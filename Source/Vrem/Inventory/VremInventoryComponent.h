// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "VremInventoryComponent.generated.h"

class UVremWeaponDefinition;

USTRUCT()
struct FInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FPrimaryAssetId ItemId;

    UPROPERTY()
    int32 Count;

	FString ToString() const
	{
		return FString::Printf(TEXT("ItemId: %s, Count: %d"), *ItemId.ToString(), Count);
	}
};

USTRUCT()
struct FInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FInventoryEntry> Entries;

	FInventoryEntry* GetEntryFromId(const FPrimaryAssetId& WeaponToAdd)
	{
		FInventoryEntry* FoundEntry = Entries.FindByPredicate(
			[WeaponToAdd](const FInventoryEntry& Entry)
			{
				return Entry.ItemId == WeaponToAdd;
			});

		return FoundEntry;
	}

	void AddEntry(const FPrimaryAssetId& WeaponToAdd)
	{
		FInventoryEntry* FoundEntry = GetEntryFromId(WeaponToAdd);
		if (FoundEntry)
		{
			FoundEntry->Count++;
			MarkItemDirty(*FoundEntry);
		}
		else
		{
			FInventoryEntry& NewEntry = Entries.Emplace_GetRef();
			NewEntry.ItemId = WeaponToAdd;
			NewEntry.Count = 1;

			MarkItemDirty(NewEntry);
		}
	}

	void RemoveEntry(const FPrimaryAssetId& WeaponToRemove)
	{
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			if (Entries[i].ItemId == WeaponToRemove)
			{
				Entries[i].Count--;
				if (Entries[i].Count <= 0)
				{
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

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParams, *this);
    }
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
	void AddWeaponToInventory(const FPrimaryAssetId& WeaponToAdd);
	void RemoveWeaponFromInventory(const FPrimaryAssetId& WeaponToRemove);

protected:
	void InitializeDefaultItems();

	UFUNCTION()
	void OnRep_InventoryItems();

protected:
	DECLARE_MULTICAST_DELEGATE(FOnInventoryChanged);
	FOnInventoryChanged OnInventoryChanged;


	UPROPERTY(EditDefaultsOnly)
	TArray<UVremWeaponDefinition*> DefaultWeaponDefinitions; 

private:
	UPROPERTY(ReplicatedUsing = OnRep_InventoryItems)
	FInventoryList InventoryItems;
};
