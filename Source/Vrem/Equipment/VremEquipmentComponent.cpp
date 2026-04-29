// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentComponent.h"
#include "VremEquipmentActor.h"
#include "ItemFragment_Equipment.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"
#include "UObject/TopLevelAssetPath.h"

FString FEquipmentEntry::ToString() const
{
	const TCHAR* StateStr = TEXT("Unknown");
	switch (EquipmentState)
	{
	case EEquipmentState::OnHand:    StateStr = TEXT("OnHand"); break;
	case EEquipmentState::Holstered: StateStr = TEXT("Holstered"); break;
	case EEquipmentState::Stowed:    StateStr = TEXT("Stowed"); break;
	default: break;
	}

	return FString::Printf(TEXT("EquipmentIndex: [%d] EquipmentDef: [%s], State: [%s], Instance: [%s], EquipmentActor: [%s]"),
		EquipmentIndex,
		EquipmentDefiniton.IsValid() ? *EquipmentDefiniton->GetName() : TEXT("Invalid"),
		StateStr,
		IsValid(EquipmentInstance) ? *EquipmentInstance->GetName() : TEXT("Invalid"),
		EquipmentActor.IsValid() ? *EquipmentActor->GetName() : TEXT("Invalid"));

}

void FEquipmentEntry::SetAndApplyEquipmentState(EEquipmentState InEquipmentState)
{
	EquipmentState = InEquipmentState;
	TryApplyEquipmentStateToInstance();
}

void FEquipmentEntry::TryApplyEquipmentStateToInstance()
{
	if (IsValid(EquipmentInstance))
	{
		EquipmentInstance->SetEquipmentState(EquipmentState);
	}
}

void FEquipmentList::SetOwner(UVremEquipmentComponent* InOwner)
{
	OwnerComponent = InOwner;

	for (int32 i = PendingEquipmentIndices.Num() - 1; i >= 0; --i)
	{
		FEquipmentEntry* Entry = GetEntryFromIndex(PendingEquipmentIndices[i]);
		if (Entry)
		{
			CreateInstanceForEntry(*Entry, TEXT("SetOwner"));

			if (IsValid(Entry->EquipmentInstance))
			{
				PendingEquipmentIndices.RemoveAt(i);
			}
		}
	}
}

void FEquipmentList::AddEntry(const UVremEquipmentDefinition* InEquipmentDefinition, int32 InIndex)
{
	if (OwnerComponent.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("FEquipmentList::AddEntry: OwnerComponent is not valid"));
		return;
	}

	// server only
	check(IsValid(OwnerComponent->GetOwner()));
	check(OwnerComponent->GetOwner()->HasAuthority());

	FEquipmentEntry* FoundEntry = GetEntryFromIndex(InIndex);
	if (FoundEntry)
	{
		RemoveEntry(InIndex);
	}

	if (IsValid(InEquipmentDefinition))
	{
		FEquipmentEntry& NewEntry = Entries.Emplace_GetRef();
		NewEntry.EquipmentDefiniton = InEquipmentDefinition;
		NewEntry.EquipmentIndex = InIndex;
		NewEntry.SetAndApplyEquipmentState(EEquipmentState::Stowed);

		if (OwnerComponent.IsValid())
		{
			CreateInstanceForEntry(NewEntry, TEXT("AddEntry"));
		}
		else
		{
			PendingEquipmentIndices.AddUnique(InIndex);
		}
	}
	else
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("FEquipmentList::AddEntry: EquipmentDefinition is null"));
	}
}

void FEquipmentList::RemoveEntry(int32 InIndex)
{
	// server only
	check(IsValid(OwnerComponent->GetOwner()));
	check(OwnerComponent->GetOwner()->HasAuthority());

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (Entries[i].EquipmentIndex == InIndex)
		{
			if (IsValid(Entries[i].EquipmentInstance))
			{
				Entries[i].EquipmentInstance->Cleanup();
			}
			Entries.RemoveAt(i);

			MarkArrayDirty();
			return;
		}
	}
}

TArray<FVremEquipmentSlotView> FEquipmentList::CollectEntryViews() const
{
	TArray<FVremEquipmentSlotView> Result;
	Result.Reserve(Entries.Num());
	for (const FEquipmentEntry& Entry : Entries)
	{
		FVremEquipmentSlotView& View = Result.Emplace_GetRef();
		View.SlotIndex = Entry.EquipmentIndex;
		View.Definition = Entry.EquipmentDefiniton.Get();
		View.State = Entry.EquipmentState;
	}
	return Result;
}

void FEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FEquipmentEntry& Entry = Entries[Index];

		if (OwnerComponent.IsValid())
		{
			CreateInstanceForEntry(Entry, TEXT("PostReplicatedAdd"));
		}
		else
		{
			PendingEquipmentIndices.AddUnique(Entry.EquipmentIndex);
		}
	}
}

void FEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FEquipmentEntry& Entry = Entries[Index];

		if (OwnerComponent.IsValid())
		{
			if (Entry.EquipmentInstance)
			{
				if (Entry.EquipmentInstance->GetEquipmentActor() == nullptr && Entry.EquipmentActor.IsValid())
				{
					Entry.EquipmentInstance->BindEquipmentActor(Entry.EquipmentActor.Get());
					UE_LOG(LogVremEquipment, Warning, TEXT("PostReplicatedChange: Client - EquipmentActor resolved\nEntry : [%s]"), *Entry.ToString());
				}
				
				Entry.TryApplyEquipmentStateToInstance();
			}
			else
			{
				CreateInstanceForEntry(Entry, TEXT("PostReplicatedChange"));
			}
		}
		else
		{
			PendingEquipmentIndices.AddUnique(Entry.EquipmentIndex);
		}
	}
}

void FEquipmentList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FEquipmentEntry& Entry = Entries[Index];
		if (IsValid(Entry.EquipmentInstance))
		{
			Entry.EquipmentInstance->Cleanup();
		}
	}
}

void FEquipmentList::CreateInstanceForEntry(FEquipmentEntry& Entry, const TCHAR* Caller)
{
	if (OwnerComponent.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry OwnerComponent is nullptr"));
		return;
	}

	if (Entry.EquipmentInstance)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry EquipmentInstance is not nullptr"));
		return;
	}

	if (Entry.EquipmentDefiniton.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry Failed To Load EquipmentDefinition LoadedPath : %s"), *Entry.EquipmentDefiniton->GetName());
		return;
	}

	AActor* OwnerActor = OwnerComponent->GetOwner();
	const bool bHasAuthority = IsValid(OwnerActor) && OwnerActor->HasAuthority();

	Entry.EquipmentInstance = NewObject<UVremEquipmentInstance>(OwnerComponent.Get());
	Entry.EquipmentInstance->OnStateChanged.AddUObject(OwnerComponent.Get(), &UVremEquipmentComponent::OnInstanceStateChanged);
	Entry.EquipmentInstance->OnInstanceDestroyed.AddUObject(OwnerComponent.Get(), &UVremEquipmentComponent::OnInstanceDestroyed);
	Entry.EquipmentInstance->Initialize(Entry.EquipmentDefiniton.Get(), OwnerActor);

	if (bHasAuthority)
	{
		// ����: ���� ���� �� Entry�� ���� �� ���� ����
		Entry.EquipmentInstance->SpawnEquipmentActor();
		Entry.EquipmentActor = Entry.EquipmentInstance->GetEquipmentActor();
	}
	else
	{
		// Ŭ���̾�Ʈ: ������ ���� ���ε�
		if (Entry.EquipmentActor.IsValid())
		{
			Entry.EquipmentInstance->BindEquipmentActor(Entry.EquipmentActor.Get());
			UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry called by [%s]: Client - EquipmentActor resolved\nEntry : [%s]"), Caller, *Entry.ToString());
		}
		else
		{
			UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry called by [%s]: Client - EquipmentActor not yet resolved\nEntry : [%s]"), Caller, *Entry.ToString());
		}
	}

	Entry.SetAndApplyEquipmentState(Entry.EquipmentState);
	MarkItemDirty(Entry);

}

void FEquipmentList::TryBindEquipmentActor(AVremEquipmentActor* InActor)
{
	for (FEquipmentEntry& Entry : Entries)
	{
		if (Entry.EquipmentActor.Get() == InActor && Entry.EquipmentInstance != nullptr)
		{
			if (Entry.EquipmentInstance->GetEquipmentActor() == nullptr)
			{
				Entry.EquipmentInstance->BindEquipmentActor(InActor);
				UE_LOG(LogVremEquipment, Warning, TEXT("TryBindEquipmentActor: Client - EquipmentActor resolved Entry : [%s]"), *Entry.ToString());
				Entry.TryApplyEquipmentStateToInstance();
			}
			return;
		}
	}
}

// =======================================
// UVremEquipmentComponent
// =======================================

// Sets default values for this component's properties
UVremEquipmentComponent::UVremEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UVremEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(UVremEquipmentComponent, EquipmentList);
}

void UVremEquipmentComponent::InitializeComponent()
{
	Super::InitializeComponent();

	EquipmentList.SetOwner(this);
}

void UVremEquipmentComponent::RequestSetCurrentWeapon(int32 InSlotIndex, EEquipmentState PrevOnHandDest /*= EEquipmentState::Stowed*/)
{
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		SetCurrentWeapon(InSlotIndex, PrevOnHandDest);
	}
	else
	{
		ServerSetCurrentWeapon(InSlotIndex, PrevOnHandDest);
	}
}

void UVremEquipmentComponent::RequestEquipItemByInstance(const UVremItemInstance* ItemToEquip, int32 InSlotIndex)
{
	if (IsValid(ItemToEquip) == false)
	{
		return;
	}

	UItemFragment_Equipment* EquipmentFragment = ItemToEquip->FindFragment<UItemFragment_Equipment>();
	if (EquipmentFragment)
	{
		RequestEquipItemByDefinition(EquipmentFragment->GetEquipmentDefinition(), InSlotIndex);
	}
}

void UVremEquipmentComponent::RequestEquipItemByDefinition(const UVremEquipmentDefinition* ItemToEquip, int32 InSlotIndex)
{
	if (IsValid(ItemToEquip) == false)
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		TryEquipItem(ItemToEquip, InSlotIndex);
	}
	else
	{
		ServerTryEquipItem(ItemToEquip, InSlotIndex);
	}
}

void UVremEquipmentComponent::RequestUnequipItemBySlot(int32 InSlotIndex)
{
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		TryUnequipItem(InSlotIndex);
	}
	else
	{
		ServerTryUnequipItem(InSlotIndex);
	}
}

void UVremEquipmentComponent::RequestUnequipItemByDefinition(const UVremEquipmentDefinition* InEquipmentDefinition)
{
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		TryUnequipItem(InEquipmentDefinition);
	}
	else
	{
		ServerTryUnequipItem(EquipmentList.FindIndexByDefinition(InEquipmentDefinition));
	}
}

TArray<FVremEquipmentSlotView> UVremEquipmentComponent::GetEquipmentEntries() const
{	
	return EquipmentList.CollectEntryViews();
}

void UVremEquipmentComponent::SetCurrentWeapon(int32 InWeaponSlotIndex, EEquipmentState PrevOnHandDest)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	FEquipmentEntry* NewEntry = EquipmentList.GetEntryFromIndex(InWeaponSlotIndex);
	if (NewEntry == nullptr) 
	{	
		return;
	}
	if (NewEntry->EquipmentState == EEquipmentState::OnHand)
	{
		return;
	}

	// 이전에 손에 있던 장비가 있다면, PrevOnHandDest로 밀어낸다
	FEquipmentEntry* PrevOnHand = EquipmentList.GetEntryFromEquipmentState(EEquipmentState::OnHand);
	if (PrevOnHand && PrevOnHand != NewEntry)
	{
		// 손에 있던 장비가 홀스터로 밀리면, 원래 홀스터에 있던 장비는 Stowed로 밀려야 한다
		if (PrevOnHandDest == EEquipmentState::Holstered)
		{
			FEquipmentEntry* PrevOnHolster = EquipmentList.GetEntryFromEquipmentState(EEquipmentState::Holstered);
			if (PrevOnHolster && PrevOnHolster != NewEntry)
			{
				PrevOnHolster->SetAndApplyEquipmentState(EEquipmentState::Stowed);
				EquipmentList.MarkItemDirty(*PrevOnHolster);
			}
		}

		PrevOnHand->SetAndApplyEquipmentState(PrevOnHandDest);
		EquipmentList.MarkItemDirty(*PrevOnHand);
	}

	// 새 장비를 손에 장착한다
	NewEntry->SetAndApplyEquipmentState(EEquipmentState::OnHand);
	EquipmentList.MarkItemDirty(*NewEntry);

	OnEquipmentUpdated.Broadcast();
}

AVremEquipmentActor* UVremEquipmentComponent::GetCurrentEquipmentActor() const
{
	const FEquipmentEntry* Entry = EquipmentList.GetEntryFromEquipmentState(EEquipmentState::OnHand);
	if (Entry == nullptr)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("GetCurrentEquipmentActor: CurrentWeaponEntry is nullptr"));
		return nullptr;
	}

	return Entry->EquipmentActor.Get();
}

void UVremEquipmentComponent::TryEquipItem(const UVremEquipmentDefinition* ItemToEquip, int32 InSlotIndex)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	if (IsValid(ItemToEquip))
	{ 
		EquipmentList.AddEntry(ItemToEquip, InSlotIndex);
		OnEquipmentUpdated.Broadcast();
	}
}

void UVremEquipmentComponent::TryUnequipItem(int32 InSlotIndex)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	EquipmentList.RemoveEntry(InSlotIndex);
	OnEquipmentUpdated.Broadcast();
}

void UVremEquipmentComponent::TryUnequipItem(const UVremEquipmentDefinition* InEquipmentDefinition)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	int32 SlotIndex = EquipmentList.FindIndexByDefinition(InEquipmentDefinition);
	if (SlotIndex != INDEX_NONE)
	{
		TryUnequipItem(SlotIndex);
	}
}

void UVremEquipmentComponent::OnEquipmentActorReplicated(AVremEquipmentActor* InActor)
{
	EquipmentList.TryBindEquipmentActor(InActor);
}

void UVremEquipmentComponent::ServerTryEquipItem_Implementation(const UVremEquipmentDefinition* ItemToEquip, int32 InSlotIndex)
{
	TryEquipItem(ItemToEquip, InSlotIndex);
}

void UVremEquipmentComponent::ServerTryUnequipItem_Implementation(int32 InSlotIndex)
{
	TryUnequipItem(InSlotIndex);
}

void UVremEquipmentComponent::ServerSetCurrentWeapon_Implementation(int32 InSlotIndex, EEquipmentState PrevOnHandDest)
{
	SetCurrentWeapon(InSlotIndex, PrevOnHandDest);
}

void UVremEquipmentComponent::OnInstanceStateChanged(EEquipmentState NewState, TSubclassOf<UAnimInstance> AnimLayerClass)
{
	switch (NewState)
	{
	case EEquipmentState::OnHand:
		OnEquipmentAttached.Broadcast(AnimLayerClass);
		break;
	case EEquipmentState::Holstered:
	case EEquipmentState::Stowed:
		OnEquipmentDetached.Broadcast(AnimLayerClass);
		break;
	}
}

void UVremEquipmentComponent::OnInstanceDestroyed(TSubclassOf<UAnimInstance> AnimLayerClass)
{
	OnEquipmentDetached.Broadcast(AnimLayerClass);
}

void UVremEquipmentComponent::OnRep_EquipmentList()
{
	EquipmentList.SetOwner(this);
	OnEquipmentUpdated.Broadcast();
}

int32 UVremEquipmentComponent::GetOnHandSlotIndex() const
{
	const FEquipmentEntry* Entry = EquipmentList.GetEntryFromEquipmentState(EEquipmentState::OnHand);
	return Entry ? Entry->EquipmentIndex : INDEX_NONE;
}

int32 UVremEquipmentComponent::GetHolsteredSlotIndex() const
{
	const FEquipmentEntry* Entry = EquipmentList.GetEntryFromEquipmentState(EEquipmentState::Holstered);
	return Entry ? Entry->EquipmentIndex : INDEX_NONE;
}

EEquipmentSlotType UVremEquipmentComponent::GetOnHandSlotType() const
{
	const FEquipmentEntry* Entry = EquipmentList.GetEntryFromEquipmentState(EEquipmentState::OnHand);
	return Entry ? Entry->EquipmentDefiniton->SlotType : EEquipmentSlotType::NUM_EEquipmentSlotType;
}

#if WITH_AUTOMATION_WORKER
void UVremEquipmentComponent::SimulateReplicateFrom(const UVremEquipmentComponent* Source)
{
	EquipmentList = Source->EquipmentList;
	OnRep_EquipmentList();
}

EEquipmentState UVremEquipmentComponent::GetEquipmentStateAtSlot(int32 InSlotIndex) const
{
	const FEquipmentEntry* Entry = EquipmentList.GetEntryFromIndex(InSlotIndex);
	return Entry != nullptr ? Entry->EquipmentState : EEquipmentState::NUM_EQUIPMENTSTATE;
}

#endif