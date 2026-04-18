// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentComponent.h"
#include "VremEquipmentActor.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"
#include "UObject/TopLevelAssetPath.h"

FString FEquipmentEntry::ToString() const
{
	return FString::Printf(TEXT("EquipmentIndex: [%d] EquipmentDef: [%s], State: [%s], Instance: [%s], EquipmentActor: [%s]"),
		EquipmentIndex,
		*EquipmentDefiniton->GetName(),
		(EquipmentState == EEquipmentState::Equipped) ? TEXT("Equipped") : TEXT("Holstered"),
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
		UE_LOG(LogVremEquipment, Warning, TEXT("FEquipmentList::AddEntry: Failed To Load EquipmentDefinition, LoadedPath : %s"), *InEquipmentDefinition->GetName());
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
			Entries[i].EquipmentInstance->Cleanup();
			Entries.RemoveAt(i);

			MarkArrayDirty();
			return;
		}
	}
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
}

void UVremEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(UVremEquipmentComponent, EquipmentList);
	DOREPLIFETIME(UVremEquipmentComponent, CurrentWeaponSlotIndex);
}

void UVremEquipmentComponent::InitializeFromOwner()
{
	EquipmentList.SetOwner(this);
}

void UVremEquipmentComponent::SetCurrentWeapon(int32 InWeaponSlotIndex)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	FEquipmentEntry* EquipmentEntry = EquipmentList.GetEntryFromIndex(CurrentWeaponSlotIndex);
	if (EquipmentEntry != nullptr)
	{
		EquipmentEntry->SetAndApplyEquipmentState(EEquipmentState::Holstered);
		EquipmentList.MarkItemDirty(*EquipmentEntry);
	}

	CurrentWeaponSlotIndex = InWeaponSlotIndex;

	EquipmentEntry = EquipmentList.GetEntryFromIndex(CurrentWeaponSlotIndex);
	if (EquipmentEntry != nullptr)
	{
		EquipmentEntry->SetAndApplyEquipmentState(EEquipmentState::Equipped);
		EquipmentList.MarkItemDirty(*EquipmentEntry);
	}
}

AVremEquipmentActor* UVremEquipmentComponent::GetCurrentEquipmentActor() const
{
	const FEquipmentEntry* Entry = EquipmentList.GetEntryFromIndex(CurrentWeaponSlotIndex);
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
	}
}

void UVremEquipmentComponent::TryUnequipItem(int32 InSlotIndex)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	FEquipmentEntry* EquipmentEntry = EquipmentList.GetEntryFromIndex(InSlotIndex);

	EquipmentList.RemoveEntry(InSlotIndex);
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

void UVremEquipmentComponent::ServerSetCurrentWeapon_Implementation(int32 InSlotIndex)
{
	SetCurrentWeapon(InSlotIndex);
}

void UVremEquipmentComponent::OnInstanceStateChanged(EEquipmentState NewState, TSubclassOf<UAnimInstance> AnimLayerClass)
{
	switch (NewState)
	{
	case EEquipmentState::Equipped:
		OnEquipmenntAttached.Broadcast(AnimLayerClass);
		break;
	case EEquipmentState::Holstered:
		OnEquipmenntDetached.Broadcast(AnimLayerClass);
		break;
	}
}

void UVremEquipmentComponent::OnInstanceDestroyed(TSubclassOf<UAnimInstance> AnimLayerClass)
{
	OnEquipmenntDetached.Broadcast(AnimLayerClass);
}

void UVremEquipmentComponent::OnRep_EquipmentList()
{
	EquipmentList.SetOwner(this);
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