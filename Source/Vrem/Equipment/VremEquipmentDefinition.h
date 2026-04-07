// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Vrem/Inventory/VremItemDefinition.h"
#include "GameplayTagContainer.h"
#include "VremEquipmentDefinition.generated.h"

class UVremEquipmentDefinition;
class AVremEquipmentActor;

UCLASS()
class UItemFragment_Equipment : public UItemFragment
{
    GENERATED_BODY()

public:
	const UVremEquipmentDefinition* GetEquipmentDefinition() const { return EquipmentDefinition; }

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UVremEquipmentDefinition> EquipmentDefinition;
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
    TSoftClassPtr<UAnimInstance> AnimLayerClass;
};

UCLASS()
class VREM_API UVremEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	void OnItemCreated(UVremEquipmentDefinition* InEquipmentDefinition);
	void OnItemRemoved();

	void RequestAttach(AActor* ParentActor);
	void RequestRemoveActor();

protected:
	TObjectPtr<UVremEquipmentDefinition> EquipmentDefinition;

	UPROPERTY(Transient)
	TObjectPtr<AVremEquipmentActor> EquipmentActor;
};