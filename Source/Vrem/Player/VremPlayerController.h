// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "VremPlayerController.generated.h"


class UVremInputConfig;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class VREM_API AVremPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AVremPlayerController();
protected:
	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* PossessedPawn) override;
	virtual void OnRep_Pawn() override;
	
// ~IGenericTeamAgentInterface Begin
	virtual FGenericTeamId GetGenericTeamId() const override { return PlayerTeamId; }
	void SetGenericTeamId(const FGenericTeamId& TeamID) override { PlayerTeamId = TeamID; }
// ~IGenericTeamAgentInterface End

protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Vrem|Faction")
	FGenericTeamId PlayerTeamId = FGenericTeamId(0);
};
