// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Vrem/Inventory/VremItemDefinition.h"
#include "GameplayTagContainer.h"
#include "VremEquipmentDefinition.generated.h"

class UVremEquipmentDefinition;
class AVremEquipmentActor;

UENUM()
enum class EEquipmentState : uint8
{
	OnHand,      // 손에 들고 사용 중
	Holstered,   // 홀스터에 있음 (즉시 꺼낼 수 있는 준비 상태)
	Stowed,      // 보관 중 (2차 저장)
	NUM_EQUIPMENTSTATE
};

UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
    Ranged  UMETA(DisplayName = "Ranged"),
    Melee   UMETA(DisplayName = "Melee"),
	NUM_EEquipmentSlotType UMETA(Hidden)
};

UCLASS()
class VREM_API UVremEquipmentDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<AActor> EquipmentActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attach")
	FName AttachSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Attach")
	FTransform AttachOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Attach")
	FName HolsterSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Attach")
	FTransform HolsterOffset;

    UPROPERTY(EditDefaultsOnly, Category = "Socket|Stowed")
    FName StowedSocketName;

    UPROPERTY(EditDefaultsOnly, Category = "Socket|Stowed")
    FTransform StowedOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TSubclassOf<UAnimInstance> AnimLayerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Slot")
    EEquipmentSlotType SlotType = EEquipmentSlotType::Ranged;
};

UCLASS()
class VREM_API UVremEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const UVremEquipmentDefinition* InEquipmentDefinition, AActor* InParentActor);
	void Cleanup();

	virtual void BeginDestroy() override;

	void SetEquipmentState(EEquipmentState InEquipmentState);
	EEquipmentState GetEquipmentState() const { return EquipmentState; }

	void BindEquipmentActor(AVremEquipmentActor* InActor);
	AVremEquipmentActor* GetEquipmentActor() const { return EquipmentActor; }

	void SpawnEquipmentActor();
protected:
	void AttachToSocket(const FName& SocketName, const FTransform& Offset);
	void ApplyEquipmentState();

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEquipmentInstanceStateChanged, EEquipmentState /*NewState*/, TSubclassOf<UAnimInstance> /*AnimLayerClass*/)
	FOnEquipmentInstanceStateChanged OnStateChanged;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquipmentInstanceDestroyed, TSubclassOf<UAnimInstance> /*AnimLayerClass*/)
	FOnEquipmentInstanceDestroyed OnInstanceDestroyed;

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<const UVremEquipmentDefinition> EquipmentDefinition;

	UPROPERTY(Transient)
	TObjectPtr<AVremEquipmentActor> EquipmentActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ParentActor;

	EEquipmentState EquipmentState;
};