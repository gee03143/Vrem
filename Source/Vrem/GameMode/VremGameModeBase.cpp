// Fill out your copyright notice in the Description page of Project Settings.


#include "VremGameModeBase.h"

#include "Vrem/VremLogChannels.h"
#include "Vrem/Character/VremCharacter.h"
#include "Vrem/Player/VremPlayerController.h"

AVremGameModeBase::AVremGameModeBase()
{
	DefaultPawnClass = AVremCharacter::StaticClass();
	PlayerControllerClass = AVremPlayerController::StaticClass();
}

void AVremGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogVrem, Log, TEXT("[%s] GameMode BeginPlay"), *GetNetModeString(GetWorld()));
}


