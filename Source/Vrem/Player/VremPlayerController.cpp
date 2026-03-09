// Fill out your copyright notice in the Description page of Project Settings.


#include "VremPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Vrem/VremLogChannels.h"
#include "InputMappingContext.h"
#include "Vrem/Character/VremCharacter.h"

void AVremPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (IsValid(Subsystem))
	{
		if (IsValid(DefaultInputMappingContext))
		{
			Subsystem->AddMappingContext(DefaultInputMappingContext, 0);
		}
		else
		{
			UE_LOG(LogVremInput, Warning, TEXT("AVremPlayerController::BeginPlay DefaultInputMappingContext Is Invalid!  NetRole : [%s]"), *GetNetRoleString(this));
		}
	}
	else
	{
		if (IsLocalController())
		{
			UE_LOG(LogVremInput, Warning, TEXT("AVremPlayerController::BeginPlay UEnhancedInputLocalPlayerSubsystem Is Invalid! NetRole : [%s]"), *GetNetRoleString(this));
		}
	}
}

void AVremPlayerController::OnPossess(APawn* PossessedPawn)
{
	Super::OnPossess(PossessedPawn);
}

void AVremPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
}
