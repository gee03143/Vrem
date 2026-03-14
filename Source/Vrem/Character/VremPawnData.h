// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremPawnData.generated.h"

class UVremInputConfig;
/**
 * 
 */
UCLASS()
class VREM_API UVremPawnData : public UDataAsset
{
	GENERATED_BODY()
	

public:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UVremInputConfig> InputConfig;

	// TODO : 아마 Pawn 외형에 대한 정보가 따로 들어갈듯.
};
