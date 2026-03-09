// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VremGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class VREM_API AVremGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVremGameModeBase();
	
protected:
	virtual void BeginPlay() override;
	
};
