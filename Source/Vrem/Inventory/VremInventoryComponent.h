// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "VremInventoryComponent.generated.h"

class UVremItemDefinition;
class UVremItemInstance;
class UVremInventoryComponent;

USTRUCT()
struct FInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FPrimaryAssetId ItemId;

    UPROPERTY()
    int32 Count;

	UVremItemInstance* ItemInstance;

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

	void AddEntry(const FPrimaryAssetId& ItemToAdd);
	void RemoveEntry(const FPrimaryAssetId& ItemToRemove);

	FString ToString() const
	{
		FString Result;
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
	TArray<FInventoryEntry> PendingEntriesForCreateInstance;
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

	void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void AddItemToInventory(const UVremItemDefinition* ItemToAdd);
	void RemoveItemFromInventory(const UVremItemDefinition* ItemToRemove);

protected:
	void InitializeDefaultItems();

	UFUNCTION()
	void OnRep_InventoryItems();

protected:
	DECLARE_MULTICAST_DELEGATE(FOnInventoryChanged);
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(EditDefaultsOnly)
	TArray<UVremItemDefinition*> DefaultItemDefinitions; 

private:
	UPROPERTY(ReplicatedUsing = OnRep_InventoryItems)
	FInventoryList InventoryItems;
};
