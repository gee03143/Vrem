// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"
#include "VremEquipmentDefinition.h"
#include "UObject/TopLevelAssetPath.h"


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

void FEquipmentList::AddEntry(const FTopLevelAssetPath& ItemToEquip)
{
	FEquipmentEntry* FoundEntry = GetEntryFromId(ItemToEquip);
	if (FoundEntry)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("FEquipmentList::AddEntry: Same Equipment Entry Trying to Add Twice"));
		return;
	}
	else
	{
		FSoftObjectPath SoftPath(ItemToEquip);
		TSoftObjectPtr<UVremEquipmentDefinition> DefPtr(SoftPath);
		UVremEquipmentDefinition* EquipmentDefinition = DefPtr.LoadSynchronous();

		if (IsValid(EquipmentDefinition))
		{
			FEquipmentEntry& NewEntry = Entries.Emplace_GetRef();
			NewEntry.EquipmentDefPath = ItemToEquip;

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
			UE_LOG(LogVremEquipment, Warning, TEXT("FEquipmentList::AddEntry: Failed To Load EquipmentDefinition, LoadedPath : %s"), *SoftPath.ToString());
		}
	}
}

void FEquipmentList::RemoveEntry(const FTopLevelAssetPath& ItemToUnequip)
{
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (Entries[i].EquipmentDefPath == ItemToUnequip)
		{
			Entries[i].EquipmentInstance->OnItemRemoved();
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
			CreateInstanceForEntry(Entry);
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

	FSoftObjectPath SoftPath(Entry.EquipmentDefPath);
	TSoftObjectPtr<UVremEquipmentDefinition> DefPtr(SoftPath);
	UVremEquipmentDefinition* EquipmentDefinition = DefPtr.LoadSynchronous();
	if (IsValid(EquipmentDefinition))
	{
		Entry.EquipmentInstance = NewObject<UVremEquipmentInstance>(OwnerComponent);
		Entry.EquipmentInstance->OnItemCreated(EquipmentDefinition);

		Entry.EquipmentInstance->RequestAttach(OwnerComponent->GetOwner());

		MarkItemDirty(Entry);
	}
	else
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("CreateInstanceForEntry Failed To Load EquipmentDefinition, LoadedPath : %s"), *SoftPath.ToString());
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

void UVremEquipmentComponent::TryEquipItem(const UVremEquipmentDefinition* ItemToEquip)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	const FSoftObjectPath SoftPath(ItemToEquip);
	EquipmentList.AddEntry(SoftPath.GetAssetPath());

	if (ItemToEquip->AnimLayerClass)
	{ 
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentComponent::TryEquipItem"));
		OnEquipmenntAttached.Broadcast(ItemToEquip);
	}
}

void UVremEquipmentComponent::TryUnequipItem(const UVremEquipmentDefinition* ItemToUnequip)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	const FSoftObjectPath SoftPath(ItemToUnequip);
	EquipmentList.RemoveEntry(SoftPath.GetAssetPath());

	if (ItemToUnequip->AnimLayerClass)
	{
		OnEquipmenntDetached.Broadcast(ItemToUnequip);
	}
}

void UVremEquipmentComponent::OnRep_EquipmentList()
{
	EquipmentList.SetOwner(this);
}
