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

	virtual void InitGameState() override;

protected:

	UPROPERTY(Config, EditAnywhere, Category="GameMode")
	FPrimaryAssetId DefaultGameModeDefinition;	// 이 값은 일단 config에서 하드코딩 하는걸로, 한번 정하면 바뀌지 않을 값에 가까움
};
