// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Vrem/Inventory/VremItemDefinition.h"
#include "GameplayTagContainer.h"
#include "ItemFragment_Ammo.generated.h"


/**
 * 
 */
UCLASS()
class VREM_API UItemFragment_Ammo : public UItemFragment
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Vrem|Ammo")
	FGameplayTag GetAmmoType() const { return AmmoType; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	FGameplayTag AmmoType;
};
