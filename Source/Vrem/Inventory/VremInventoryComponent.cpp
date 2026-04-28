// Fill out your copyright notice in the Description page of Project Settings.


#include "VremInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/VremAssetManager.h"
#include "Vrem/Equipment/ItemFragment_Equipment.h"


// =======================================
// FInventoryList
// =======================================
void FInventoryList::SetOwner(UVremInventoryComponent* InOwner)
{
	OwnerComponent = InOwner;

	for (int32 i = PendingIdsForCreateInstance.Num() - 1; i >= 0; --i)
	{
		FInventoryEntry* Entry = GetEntryFromId(PendingIdsForCreateInstance[i]);
		if (Entry)
		{ 
			if (Entry->ItemInstance == nullptr)
			{
				CreateInstanceForEntry(*Entry);
			}

			if (IsValid(Entry->ItemInstance))
			{
				PendingIdsForCreateInstance.RemoveAt(i);
			}
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
				const UVremItemDefinition* ItemDef = Entries[i].ItemInstance->GetItemDefinition();
				Entries[i].ItemInstance->OnItemRemoved();
				Entries.RemoveAt(i);
				MarkArrayDirty();

				OwnerComponent->OnItemInstanceRemoved.Broadcast(ItemDef);
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
			if (Entry.ItemInstance == nullptr)
			{
				CreateInstanceForEntry(Entry);
			}
		}
		else
		{
			PendingIdsForCreateInstance.AddUnique(Entry.ItemId);
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
			if (Entry.ItemInstance == nullptr)
			{
				CreateInstanceForEntry(Entry);
			}
		}
		else
		{
			PendingIdsForCreateInstance.AddUnique(Entry.ItemId);
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

		OwnerComponent->OnItemInstanceCreated.Broadcast(Entry.ItemInstance);

		MarkItemDirty(Entry);
	}
	else
	{
		UE_LOG(LogVremInventory, Warning, TEXT("CreateInstanceForEntry ItemDef is nullptr"));
	}
}

#if WITH_AUTOMATION_WORKER
void FInventoryList::TestAddEntry(UVremItemDefinition* ItemDef, int32 Count)
{
	FInventoryEntry& NewEntry = Entries.Emplace_GetRef();
	NewEntry.ItemId = ItemDef->GetPrimaryAssetId();
	NewEntry.Count = Count;
	NewEntry.ItemInstance = NewObject<UVremItemInstance>(OwnerComponent);
	NewEntry.ItemInstance->OnItemCreated(ItemDef);

	OwnerComponent->OnItemInstanceCreated.Broadcast(NewEntry.ItemInstance);
	MarkItemDirty(NewEntry);
}
#endif

// =======================================
// UVremInventoryComponent
// =======================================
UVremInventoryComponent::UVremInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UVremInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();

	InventoryItems.SetOwner(this);
}

void UVremInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

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

void UVremInventoryComponent::ServerAddItemToInventory_Implementation(const UVremItemDefinition* ItemToAdd)
{
	AddItemToInventory(ItemToAdd);
}

void UVremInventoryComponent::ServerRemoveItemFromInventory_Implementation(const UVremItemDefinition* ItemToRemove)
{
	RemoveItemFromInventory(ItemToRemove);
}

void UVremInventoryComponent::InitializeDefaultItems()
{
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

#if WITH_AUTOMATION_WORKER
void UVremInventoryComponent::TestAddItem(UVremItemDefinition* ItemDef, int32 Count)
{
	if (ItemDef == nullptr)
	{
		return;
	}

	FInventoryEntry* FoundEntry = InventoryItems.GetEntryFromId(ItemDef->GetPrimaryAssetId());
	if (FoundEntry)
	{
		FoundEntry->Count += Count;
	}
	else
	{
		InventoryItems.TestAddEntry(ItemDef, Count);
	}

	OnInventoryChanged.Broadcast();
}

void UVremInventoryComponent::TestRemoveItem(const FPrimaryAssetId& ItemId)
{
	InventoryItems.RemoveEntry(ItemId);
	OnInventoryChanged.Broadcast();
}

void UVremInventoryComponent::SimulateReplicateFrom(const UVremInventoryComponent* Source)
{
	InventoryItems = Source->InventoryItems;
	OnRep_InventoryItems();
}

#endif // WITH_AUTOMATION_WORKER
