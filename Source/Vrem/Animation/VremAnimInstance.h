// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "VremAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class VREM_API UVremAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	

public:
	void SetWeaponAnimLayer(const TSubclassOf<UAnimInstance> NewLayer);

	TSubclassOf<UAnimInstance> GetCurrentLayer() const { return CurrentLayer; }

protected:
    UPROPERTY(Transient)
    TSubclassOf<UAnimInstance> CurrentLayer;
};
