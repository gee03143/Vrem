// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremWeaponDefinition.generated.h"

class UAnimInstance;
/**
 * 
 */
UCLASS()
class VREM_API UVremWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<AActor> WeaponActorClass;

	UPROPERTY(EditDefaultsOnly)
    TSoftClassPtr<UAnimInstance> AnimLayerClass;
};
