// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
#include "VremEquipmentComponent.generated.h"

class UVremEquipmentInstance;
class UVremEquipmentDefinition;

USTRUCT()
struct FEquipmentEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FTopLevelAssetPath EquipmentDefPath;

	UVremEquipmentInstance* EquipmentInstance;

	FString ToString() const
	{
		return FString::Printf(TEXT("ItemId: %s"), *EquipmentDefPath.ToString());
	}

	bool operator==(const FEquipmentEntry& Other) const
	{
		return EquipmentDefPath == Other.EquipmentDefPath;
	}
};

USTRUCT()
struct FEquipmentList : public FFastArraySerializer
{
    GENERATED_BODY()

	void SetOwner(UVremEquipmentComponent* InOwner);

	FEquipmentEntry* GetEntryFromId(const FTopLevelAssetPath& InEquipmentDefPath)
	{
		FEquipmentEntry* FoundEntry = Entries.FindByPredicate(
			[InEquipmentDefPath](const FEquipmentEntry& Other)
			{
				return InEquipmentDefPath == Other.EquipmentDefPath;
			});

		return FoundEntry;
	}

	void AddEntry(const FTopLevelAssetPath& ItemToEquip);
	void RemoveEntry(const FTopLevelAssetPath& ItemToUnequip);

	FString ToString() const
	{
		FString Result;
		Result += FString::Printf(TEXT("Equipment Component\n"));
		Result += FString::Printf(TEXT("Num Entries - %d\n"), Entries.Num());

		for (const FEquipmentEntry& Entry : Entries)
		{
			Result += FString::Printf(TEXT(" - %s\n"), *Entry.ToString());
		}

		return Result;
	}

	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	void CreateInstanceForEntry(FEquipmentEntry& Entry);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FastArrayDeltaSerialize<FEquipmentEntry, FEquipmentList>(Entries, DeltaParams, *this);
    }

private:
    UPROPERTY()
    TArray<FEquipmentEntry> Entries;

	UVremEquipmentComponent* OwnerComponent = nullptr;
	TArray<FEquipmentEntry> PendingEntriesForCreateInstance;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVremEquipmentComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeFromOwner();
public:
	void TryEquipItem(const UVremEquipmentDefinition* ItemToEquip);
	void TryUnequipItem(const UVremEquipmentDefinition* ItemToUnequip);

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquipmentChanged, const UVremEquipmentDefinition*)
	FOnEquipmentChanged OnEquipmenntAttached;
	FOnEquipmentChanged OnEquipmenntDetached;

protected:
	UFUNCTION()
	void OnRep_EquipmentList();

private:
	UPROPERTY(ReplicatedUsing = OnRep_EquipmentList)
	FEquipmentList EquipmentList;
};
