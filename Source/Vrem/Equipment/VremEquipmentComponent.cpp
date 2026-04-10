// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"
#include "UObject/TopLevelAssetPath.h"

void FEquipmentEntry::SetAndApplyEquipmentState(EEquipmentState InEquipmentState)
{
	if (EquipmentState == InEquipmentState)
	{
		return;
	}

	EquipmentState = InEquipmentState;
	ApplyEquipmentStateToInstance();
}

void FEquipmentEntry::ApplyEquipmentStateToInstance()
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

void FEquipmentList::AddEntry(UVremEquipmentDefinition* InEquipmentDefinition, int32 InIndex)
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

		if (IsValid(OwnerComponent))
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

		if (OwnerComponent)
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

		if (OwnerComponent)
		{
			if (Entry.EquipmentInstance)
			{
				Entry.ApplyEquipmentStateToInstance();
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
	if (!OwnerComponent)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry OwnerComponent is nullptr"));
		return;
	}

	if (Entry.EquipmentInstance)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry EquipmentInstance is not nullptr"));
		return;
	}

	if (IsValid(Entry.EquipmentDefiniton))
	{
		Entry.EquipmentInstance = NewObject<UVremEquipmentInstance>(OwnerComponent);
		Entry.EquipmentInstance->Initialize(Entry.EquipmentDefiniton, OwnerComponent->GetOwner());
		Entry.SetAndApplyEquipmentState(EEquipmentState::Unequipped);

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

void UVremEquipmentComponent::TryEquipItem(UVremEquipmentDefinition* ItemToEquip, int32 InSlotIndex)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	if (IsValid(ItemToEquip))
	{ 
		EquipmentList.AddEntry(ItemToEquip, InSlotIndex);
		if (ItemToEquip->AnimLayerClass)
		{
			OnEquipmenntAttached.Broadcast(ItemToEquip->AnimLayerClass);
		}
	}
}

void UVremEquipmentComponent::TryUnequipItem(int32 InSlotIndex)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	FEquipmentEntry* EquipmentEntry = EquipmentList.GetEntryFromIndex(InSlotIndex);

	EquipmentList.RemoveEntry(InSlotIndex);

	if (EquipmentEntry != nullptr)
	{
		if (IsValid(EquipmentEntry->EquipmentDefiniton) && EquipmentEntry->EquipmentDefiniton->AnimLayerClass)
		{
			OnEquipmenntDetached.Broadcast(EquipmentEntry->EquipmentDefiniton->AnimLayerClass);
		}
	}
}

void UVremEquipmentComponent::OnRep_EquipmentList()
{
	EquipmentList.SetOwner(this);
}