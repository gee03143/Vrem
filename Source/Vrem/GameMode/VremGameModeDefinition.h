// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremGameModeDefinition.generated.h"

class UVremPawnData;
/**
 * 
 */
UCLASS(BlueprintType)
class VREM_API UVremGameModeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("GameModeDefinition", GetFName());
	}
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UVremPawnData> PawnData;
	
};
