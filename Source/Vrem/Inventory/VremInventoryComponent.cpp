// Fill out your copyright notice in the Description page of Project Settings.


#include "VremInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/VremAssetManager.h"
#include "Vrem/Equipment/ItemFragment_Equipment.h"
#include "Vrem/Equipment/Weapon/ItemFragment_Ammo.h"


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

TArray<FVremInventoryEntryView> FInventoryList::CollectEntryViews() const
{
	TArray<FVremInventoryEntryView> Result;
	Result.Reserve(Entries.Num());
	for (const FInventoryEntry& Entry : Entries)
	{
		FVremInventoryEntryView& View = Result.Emplace_GetRef();
		View.ItemId = Entry.ItemId;
		View.Count = Entry.Count;
		View.Instance = Entry.ItemInstance;
	}
	return Result;
}

int32 FInventoryList::GetCountByFragmentMatch(const UItemFragment* MatchFragment) const
{
	if (IsValid(MatchFragment) == false)
	{
		return 0;
	}

	int32 ReturnValue = 0;
	for (const FInventoryEntry& Entry : Entries)
	{
		if (IsValid(Entry.ItemInstance) == false)
		{
			continue;
		}

		UItemFragment* EntryFragment = Entry.ItemInstance->FindFragmentByClass(MatchFragment->GetClass());
		if (EntryFragment && EntryFragment->Matches(MatchFragment))
		{
			ReturnValue += Entry.Count;
		}
	}

	return ReturnValue;
}

int32 FInventoryList::RemoveByFragmentMatch(const UItemFragment* MatchFragment, int32 Amount)
{
	if (IsValid(MatchFragment) == false || Amount <= 0)
	{
		return 0;
	}

	int32 Remaining = Amount;
	int32 TotalConsumed = 0;

	for (int32 i = Entries.Num() - 1; i >= 0 && Remaining > 0; --i)
	{
		FInventoryEntry& Entry = Entries[i];
		if (IsValid(Entry.ItemInstance) == false)
		{
			continue;
		}

		UItemFragment* EntryFragment = Entry.ItemInstance->FindFragmentByClass(MatchFragment->GetClass());
		if (EntryFragment == nullptr || EntryFragment->Matches(MatchFragment) == false)
		{
			continue;
		}

		const int32 ConsumeFromThis = FMath::Min(Remaining, Entry.Count);
		Entry.Count -= ConsumeFromThis;
		Remaining -= ConsumeFromThis;
		TotalConsumed += ConsumeFromThis;

		if (Entry.Count <= 0)
		{
			const UVremItemDefinition* ItemDef = Entries[i].ItemInstance->GetItemDefinition();
			Entries[i].ItemInstance->OnItemRemoved();
			Entries.RemoveAt(i);
			MarkArrayDirty();

			if (OwnerComponent)
			{
				OwnerComponent->OnItemInstanceRemoved.Broadcast(ItemDef);
			}
		}
		else
		{
			MarkItemDirty(Entry);
		}
	}

	return TotalConsumed;
}

void FInventoryList::AddEntry(const FPrimaryAssetId& ItemToAdd, int32 Amount)
{
	FInventoryEntry* FoundEntry = GetEntryFromId(ItemToAdd);
	if (FoundEntry)
	{
		FoundEntry->Count += Amount;
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
			NewEntry.Count = Amount;

			CreateInstanceForEntry(NewEntry);
		}
	}
}

void FInventoryList::RemoveEntry(const FPrimaryAssetId& ItemToRemove, int32 Amount)
{
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (Entries[i].ItemId == ItemToRemove)
		{
			Entries[i].Count -= Amount;
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

void UVremInventoryComponent::AddItemToInventory(const UVremItemDefinition* ItemToAdd, int32 Amount)
{
    check(IsValid(GetOwner()));
    check(GetOwner()->HasAuthority());

	if (IsValid(ItemToAdd) == false || Amount <= 0)
	{
		return;
	}

	InventoryItems.AddEntry(ItemToAdd->GetPrimaryAssetId(), Amount);
    OnInventoryChanged.Broadcast();
}

void UVremInventoryComponent::RemoveItemFromInventory(const UVremItemDefinition* ItemToRemove, int32 Amount)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	if (IsValid(ItemToRemove) == false || Amount <= 0)
	{
		return;
	}

	InventoryItems.RemoveEntry(ItemToRemove->GetPrimaryAssetId(), Amount);
	OnInventoryChanged.Broadcast();
}

TArray<FVremInventoryEntryView> UVremInventoryComponent::GetInventoryEntries() const
{
	return InventoryItems.CollectEntryViews();
}

int32 UVremInventoryComponent::GetItemCountByFragmentMatch(const UItemFragment* MatchFragment) const
{
	return InventoryItems.GetCountByFragmentMatch(MatchFragment);
}

int32 UVremInventoryComponent::RemoveItemsByFragmentMatch(const UItemFragment* MatchFragment, int32 Amount) 
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	return InventoryItems.RemoveByFragmentMatch(MatchFragment, Amount);
}

int32 UVremInventoryComponent::GetAmmoCount(FGameplayTag AmmoType) const
{
	UItemFragment_Ammo* Probe = NewObject<UItemFragment_Ammo>(GetTransientPackage());
	Probe->SetAmmoType(AmmoType);

	return GetItemCountByFragmentMatch(Probe);
}

int32 UVremInventoryComponent::RemoveAmmo(FGameplayTag AmmoType, int32 Amount)
{
	UItemFragment_Ammo* Probe = NewObject<UItemFragment_Ammo>(GetTransientPackage());
	Probe->SetAmmoType(AmmoType);

	// RemoveItemsByFragmentMatch 안에서 HasAuthority 체크함
	return RemoveItemsByFragmentMatch(Probe, Amount);
}

void UVremInventoryComponent::ServerAddItemToInventory_Implementation(const UVremItemDefinition* ItemToAdd, int32 Amount)
{
	AddItemToInventory(ItemToAdd, Amount);
}

void UVremInventoryComponent::ServerRemoveItemFromInventory_Implementation(const UVremItemDefinition* ItemToRemove, int32 Amount)
{
	RemoveItemFromInventory(ItemToRemove, Amount);
}

void UVremInventoryComponent::InitializeDefaultItems()
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	for (const FDefaultInventoryItem& DefaultItem : DefaultItems)
	{
		if (IsValid(DefaultItem.ItemDefinition) && DefaultItem.Count > 0)
		{
			AddItemToInventory(DefaultItem.ItemDefinition, DefaultItem.Count);
		}
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
