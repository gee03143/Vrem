// Fill out your copyright notice in the Description page of Project Settings.


#include "VremInventoryComponent.h"
#include "Vrem/Inventory/VremItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/VremAssetManager.h"

// temp...
#include "Vrem/Equipment/VremEquipmentDefinition.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"


// =======================================
// FInventoryList
// =======================================
void FInventoryList::SetOwner(UVremInventoryComponent* InOwner)
{
	OwnerComponent = InOwner;

	for (int32 i = PendingEntriesForCreateInstance.Num() - 1; i >= 0; --i)
	{
		FInventoryEntry& Entry = PendingEntriesForCreateInstance[i];
		CreateInstanceForEntry(Entry);

		if (IsValid(Entry.ItemInstance))
		{
			PendingEntriesForCreateInstance.RemoveAt(i);
		}
	}
}


void FInventoryList::AddEntry(const FPrimaryAssetId& ItemToAdd)
{
	FInventoryEntry* FoundEntry = GetEntryFromId(ItemToAdd);
	if (FoundEntry)
	{
		FoundEntry->Count++;
		MarkItemDirty(*FoundEntry);
	}
	else
	{
		UVremAssetManager& Manager = UVremAssetManager::Get();
		UVremItemDefinition* ItemDef = Manager.GetItemDefinition(ItemToAdd);

		if (ItemDef && IsValid(OwnerComponent))
		{
			FInventoryEntry& NewEntry = Entries.Emplace_GetRef();
			NewEntry.ItemId = ItemToAdd;
			NewEntry.Count = 1;

			CreateInstanceForEntry(NewEntry);
		}
	}
}

void FInventoryList::RemoveEntry(const FPrimaryAssetId& ItemToRemove)
{
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (Entries[i].ItemId == ItemToRemove)
		{
			Entries[i].Count--;
			if (Entries[i].Count <= 0)
			{
				Entries[i].ItemInstance->OnItemRemoved();
				Entries.RemoveAt(i);
				MarkArrayDirty();
			}
			else
			{
				MarkItemDirty(Entries[i]);
			}
			return;
		}
	}
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FInventoryEntry& Entry = Entries[Index];

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

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FInventoryEntry& Entry = Entries[Index];

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

void FInventoryList::CreateInstanceForEntry(FInventoryEntry& Entry)
{
	if (!OwnerComponent)
	{
		UE_LOG(LogVremInventory, Warning, TEXT("CreateInstanceForEntry OwnerComponent is nullptr"));
		return;
	}

	if (Entry.ItemInstance)
	{
		UE_LOG(LogVremInventory, Warning, TEXT("CreateInstanceForEntry ItemInstance is not nullptr"));
		return;
	}

	UVremAssetManager& Manager = UVremAssetManager::Get();
	UVremItemDefinition* ItemDef = Manager.GetItemDefinition(Entry.ItemId);

	if (ItemDef)
	{
		Entry.ItemInstance = NewObject<UVremItemInstance>(OwnerComponent);
		Entry.ItemInstance->OnItemCreated(ItemDef);

		// temp code...
		{

			if (OwnerComponent->GetOwner()->HasAuthority())
			{
				UItemFragment_Equipment* EquipmentFragment = Entry.ItemInstance->FindFragment<UItemFragment_Equipment>();
				if (IsValid(EquipmentFragment))
				{
					UVremEquipmentComponent* EquipmentComponent = OwnerComponent->GetOwner()->GetComponentByClass<UVremEquipmentComponent>();
					EquipmentComponent->TryEquipItem(EquipmentFragment->GetEquipmentDefinition());
				}
				else
				{
					UE_LOG(LogVremInventory, Warning, TEXT("CreateInstanceForEntry cannot find EquipmentFragment"));
				}
			}
		}

		MarkItemDirty(Entry);
	}
	else
	{
		UE_LOG(LogVremInventory, Warning, TEXT("CreateInstanceForEntry ItemDef is nullptr"));
	}
}

// =======================================
// UVremInventoryComponent
// =======================================
UVremInventoryComponent::UVremInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

void UVremInventoryComponent::InitializeFromOwner()
{
	InventoryItems.SetOwner(this);
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		InitializeDefaultItems();
	}
}

void UVremInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ 
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UVremInventoryComponent, InventoryItems, COND_OwnerOnly);
}

void UVremInventoryComponent::AddItemToInventory(const UVremItemDefinition* ItemToAdd)
{
    check(IsValid(GetOwner()));
    check(GetOwner()->HasAuthority());

	InventoryItems.AddEntry(ItemToAdd->GetPrimaryAssetId());
    OnInventoryChanged.Broadcast();
}

void UVremInventoryComponent::RemoveItemFromInventory(const UVremItemDefinition* ItemToRemove)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	InventoryItems.RemoveEntry(ItemToRemove->GetPrimaryAssetId());
	OnInventoryChanged.Broadcast();
}

void UVremInventoryComponent::InitializeDefaultItems()
{
	UE_LOG(LogVremInventory, Warning, TEXT("UVremInventoryComponent::InitializeDefaultItems"));

	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	for (const UVremItemDefinition* Def : DefaultItemDefinitions)
	{
		AddItemToInventory(Def);
	}
}

void UVremInventoryComponent::OnRep_InventoryItems()
{
	InventoryItems.SetOwner(this);

	OnInventoryChanged.Broadcast();
}