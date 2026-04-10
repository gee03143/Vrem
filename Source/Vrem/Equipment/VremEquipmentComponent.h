// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
#include "VremEquipmentDefinition.h"
#include "VremEquipmentComponent.generated.h"

USTRUCT()
struct FEquipmentEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

	UPROPERTY()
	UVremEquipmentDefinition* EquipmentDefiniton;

	UVremEquipmentInstance* EquipmentInstance;

	UPROPERTY()
	EEquipmentState EquipmentState = EEquipmentState::Unequipped;

	UPROPERTY()
	int32 EquipmentIndex = INDEX_NONE;

	FString ToString() const
	{
		return FString::Printf(TEXT("EquipmentIndex: %d ItemId: %s"), EquipmentIndex, *EquipmentDefiniton->GetName());
	}

	void SetAndApplyEquipmentState(EEquipmentState InEquipmentState);
	void ApplyEquipmentStateToInstance();

	bool operator==(const FEquipmentEntry& Other) const
	{
		return EquipmentIndex == Other.EquipmentIndex;
	}

};

USTRUCT()
struct FEquipmentList : public FFastArraySerializer
{
    GENERATED_BODY()

	void SetOwner(UVremEquipmentComponent* InOwner);

	FEquipmentEntry* GetEntryFromIndex(const int32 InIndex)
	{
		FEquipmentEntry* FoundEntry = Entries.FindByPredicate(
			[InIndex](const FEquipmentEntry& Other)
			{
				return InIndex == Other.EquipmentIndex;
			});

		return FoundEntry;
	}

	void AddEntry(UVremEquipmentDefinition* InEquipmentDefinition, int32 InIndex);
	void RemoveEntry(int32 InIndex);
	int32 GetNumEntries() const { return Entries.Num(); }

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
	void SetCurrentWeapon(int32 InWeaponSlotIndex);
	int32 GetEquipmentItemNum() const { return EquipmentList.GetNumEntries(); }

	void TryEquipItem(UVremEquipmentDefinition* ItemToEquip, int32 InSlotIndex);
	void TryUnequipItem(int32 InSlotIndex);

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquipmentChanged, const TSubclassOf<UAnimInstance>)
	FOnEquipmentChanged OnEquipmenntAttached;
	FOnEquipmentChanged OnEquipmenntDetached;

protected:
	UFUNCTION()
	void OnRep_EquipmentList();

protected:
	UPROPERTY(EditDefaultsOnly)
	int32 CurrentWeaponSlotIndex = INDEX_NONE;

private:
	UPROPERTY(ReplicatedUsing = OnRep_EquipmentList)
	FEquipmentList EquipmentList;
};
