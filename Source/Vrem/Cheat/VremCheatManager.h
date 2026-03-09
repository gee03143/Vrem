// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "VremCheatManager.generated.h"

/**
 * 
 */
UCLASS()
class VREM_API UVremCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(exec)
	void TestAssetSyncLoad();
	
};
