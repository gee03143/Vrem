// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"
#include "UObject/TopLevelAssetPath.h"

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
	UE_LOG(LogVremEquipment, Warning, TEXT("FEquipmentList::SetOwner"));
	OwnerComponent = InOwner;

	for (int32 i = PendingEntriesForCreateInstance.Num() - 1; i >= 0; --i)
	{
		FEquipmentEntry& Entry = PendingEntriesForCreateInstance[i];
		CreateInstanceForEntry(Entry);

		if (IsValid(Entry.EquipmentInstance))
		{
			PendingEntriesForCreateInstance.RemoveAt(i);
		}
	}
}

void FEquipmentList::AddEntry(const UVremEquipmentDefinition* InEquipmentDefinition, int32 InIndex)
{
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
			CreateInstanceForEntry(NewEntry);
		}
		else
		{
			PendingEntriesForCreateInstance.AddUnique(NewEntry);
		}
	}
	else
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("FEquipmentList::AddEntry: Failed To Load EquipmentDefinition, LoadedPath : %s"), *InEquipmentDefinition->GetName());
	}
}

void FEquipmentList::RemoveEntry(int32 InIndex)
{
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
			CreateInstanceForEntry(Entry);
		}
		else
		{
			PendingEntriesForCreateInstance.AddUnique(Entry);
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
				UE_LOG(LogVremEquipment, Warning, TEXT("FEquipmentList::PostReplicatedChange TryApplyEquipmentStateToInstance"));
				Entry.TryApplyEquipmentStateToInstance();
			}
			else
			{
				CreateInstanceForEntry(Entry);
			}
		}
		else
		{
			PendingEntriesForCreateInstance.AddUnique(Entry);
		}
	}
}

void FEquipmentList::CreateInstanceForEntry(FEquipmentEntry& Entry)
{
	if (OwnerComponent.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry OwnerComponent is nullptr"));
		return;
	}

	UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::CreateInstanceForEntry  NetMode : %s"), *GetNetModeString(OwnerComponent->GetWorld()));

	if (Entry.EquipmentInstance)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry EquipmentInstance is not nullptr"));
		return;
	}

	if (Entry.EquipmentDefiniton.IsValid())
	{
		Entry.EquipmentInstance = NewObject<UVremEquipmentInstance>(OwnerComponent.Get());
		Entry.EquipmentInstance->Initialize(Entry.EquipmentDefiniton.Get(), OwnerComponent->GetOwner());
		Entry.SetAndApplyEquipmentState(Entry.EquipmentState);

		MarkItemDirty(Entry);
	}
	else
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry Failed To Load EquipmentDefinition LoadedPath : %s"), *Entry.EquipmentDefiniton->GetName());
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
}

void UVremEquipmentComponent::InitializeFromOwner()
{
	EquipmentList.SetOwner(this);
}

void UVremEquipmentComponent::SetCurrentWeapon(int32 InWeaponSlotIndex)
{
	FEquipmentEntry* EquipmentEntry = EquipmentList.GetEntryFromIndex(CurrentWeaponSlotIndex);
	if (EquipmentEntry != nullptr)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentComponent::SetCurrentWeapon"));
		EquipmentEntry->SetAndApplyEquipmentState(EEquipmentState::Holstered);
		EquipmentList.MarkItemDirty(*EquipmentEntry);
	}

	CurrentWeaponSlotIndex = InWeaponSlotIndex;

	EquipmentEntry = EquipmentList.GetEntryFromIndex(CurrentWeaponSlotIndex);
	if (EquipmentEntry != nullptr)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentComponent::SetCurrentWeapon"));
		EquipmentEntry->SetAndApplyEquipmentState(EEquipmentState::Equipped);
		EquipmentList.MarkItemDirty(*EquipmentEntry);
	}
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

void UVremEquipmentComponent::OnRep_EquipmentList()
{
	EquipmentList.SetOwner(this);
}