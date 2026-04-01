// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremCameraMode.generated.h"


struct FVremCameraState
{
	float TargetFOV;
	float TargetArmLength;
	FVector TargetSocketOffset;
};

UCLASS()
class VREM_API UVremCameraMode : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	float TargetFOV;

	UPROPERTY(EditDefaultsOnly)
	float TargetArmLength;

	UPROPERTY(EditDefaultsOnly)
	FVector TargetSocketOffset;
};
