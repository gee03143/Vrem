// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VremPlayerController.generated.h"


class UVremInputConfig;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class VREM_API AVremPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* PossessedPawn) override;
	virtual void OnRep_Pawn() override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;
};
