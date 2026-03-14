// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "VremGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class VREM_API AVremGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	AVremGameStateBase();

protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UVremGameModeDefManager> GameModeManager;
};
