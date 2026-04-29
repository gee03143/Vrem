// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "VremItemDefinition.h"
#include "VremInventoryComponent.generated.h"

class UVremItemDefinition;
class UVremItemInstance;
class UVremInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemInstanceCreated, UVremItemInstance*, ItemInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemInstanceRemoved, const UVremItemDefinition*, ItemDefinition);

USTRUCT(BlueprintType)
struct FVremInventoryEntryView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Vrem|Inventory")
    FPrimaryAssetId ItemId;

    UPROPERTY(BlueprintReadOnly, Category = "Vrem|Inventory")
    int32 Count = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Vrem|Inventory")
    UVremItemInstance* Instance = nullptr;
};

USTRUCT()
struct FInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FPrimaryAssetId ItemId;

    UPROPERTY()
    int32 Count = 0;

	UPROPERTY(NotReplicated, Transient)
	UVremItemInstance* ItemInstance = nullptr;

	FString ToString() const
	{
		return FString::Printf(TEXT("ItemId: %s, Count: %d"), *ItemId.ToString(), Count);
	}

	bool operator==(const FInventoryEntry& Other) const
	{
		return ItemId == Other.ItemId;
	}
};

USTRUCT()
struct FInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

	void SetOwner(UVremInventoryComponent* InOwner);

	FInventoryEntry* GetEntryFromId(const FPrimaryAssetId& WeaponToAdd)
	{
		FInventoryEntry* FoundEntry = Entries.FindByPredicate(
			[WeaponToAdd](const FInventoryEntry& Entry)
			{
				return Entry.ItemId == WeaponToAdd;
			});

		return FoundEntry;
	}

	TArray<FVremInventoryEntryView> CollectEntryViews() const;

	void AddEntry(const FPrimaryAssetId& ItemToAdd);
	void RemoveEntry(const FPrimaryAssetId& ItemToRemove);
	int32 GetNumEntries() const { return Entries.Num(); }

	FString ToString() const
	{
		FString Result;
		Result += FString::Printf(TEXT("Inventory Component\n"));
		Result += FString::Printf(TEXT("Num Entries - %d\n"), Entries.Num());

		for (const FInventoryEntry& Entry : Entries)
		{
			Result += FString::Printf(TEXT(" - %s\n"), *Entry.ToString());
		}

		return Result;
	}

	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	void CreateInstanceForEntry(FInventoryEntry& Entry);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParams, *this);
    }

private:
    UPROPERTY()
    TArray<FInventoryEntry> Entries;

	UVremInventoryComponent* OwnerComponent = nullptr;
	TArray<FPrimaryAssetId> PendingIdsForCreateInstance;

#if WITH_AUTOMATION_WORKER
public:
	void TestAddEntry(UVremItemDefinition* ItemDef, int32 Count);
#endif
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVremInventoryComponent();

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void AddItemToInventory(const UVremItemDefinition* ItemToAdd);
	void RemoveItemFromInventory(const UVremItemDefinition* ItemToRemove);
	FString GetInventoryItemsString() const { return InventoryItems.ToString(); }
	int32 GetInventoryItemNum() const { return InventoryItems.GetNumEntries(); }
	TArray<FVremInventoryEntryView> GetInventoryEntries() const;

	UFUNCTION(Server, Reliable)
	void ServerAddItemToInventory(const UVremItemDefinition* ItemToAdd);
	UFUNCTION(Server, Reliable)
	void ServerRemoveItemFromInventory(const UVremItemDefinition* ItemToRemove);
protected:
	void InitializeDefaultItems();

	UFUNCTION()
	void OnRep_InventoryItems();

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

	UPROPERTY(BlueprintAssignable, Category="Vrem|Inventory")
	FOnInventoryChanged OnInventoryChanged;


	UPROPERTY(BlueprintAssignable, Category="Vrem|Inventory")
	FOnItemInstanceCreated OnItemInstanceCreated;


	UPROPERTY(BlueprintAssignable, Category="Vrem|Inventory")
	FOnItemInstanceRemoved OnItemInstanceRemoved;

protected:
	UPROPERTY(EditDefaultsOnly)
	TArray<UVremItemDefinition*> DefaultItemDefinitions; 

private:
	UPROPERTY(ReplicatedUsing = OnRep_InventoryItems)
	FInventoryList InventoryItems;

#if WITH_AUTOMATION_WORKER
public:
	void TestAddItem(UVremItemDefinition* ItemDef, int32 Count = 1);
	void TestRemoveItem(const FPrimaryAssetId& ItemId);
	void SimulateReplicateFrom(const UVremInventoryComponent* Source);
#endif
};
