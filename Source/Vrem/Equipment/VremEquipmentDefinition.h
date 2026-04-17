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
	Equipped,
	Holstered,
	NUM_EQUIPMENTSTATE
};

UCLASS()
class VREM_API UVremEquipmentDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<AActor> EquipmentActorClass;

	UPROPERTY(EditDefaultsOnly)
	FName AttachSocketName;

	UPROPERTY(EditDefaultsOnly)
	FTransform AttachOffset;

	UPROPERTY(EditDefaultsOnly)
	FName HolsterSocketName;

	UPROPERTY(EditDefaultsOnly)
	FTransform HolsterOffset;

	UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UAnimInstance> AnimLayerClass;
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