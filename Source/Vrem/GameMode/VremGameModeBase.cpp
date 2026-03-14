// Fill out your copyright notice in the Description page of Project Settings.


#include "VremGameModeBase.h"

#include "VremGameModeDefManager.h"
#include "VremGameStateBase.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/Character/VremCharacter.h"
#include "Vrem/Player/VremPlayerController.h"

AVremGameModeBase::AVremGameModeBase()
{
	GameStateClass = AVremGameStateBase::StaticClass();
	DefaultPawnClass = AVremCharacter::StaticClass();
	PlayerControllerClass = AVremPlayerController::StaticClass();
}

void AVremGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogVrem, Log, TEXT("[%s] GameMode BeginPlay"), *GetNetModeString(GetWorld()));
}

void AVremGameModeBase::InitGameState()
{
	Super::InitGameState();

	if (DefaultGameModeDefinition.IsValid())
	{
		if (IsValid(GameState))
		{
			UVremGameModeDefManager* GameModeManager = GameState->GetComponentByClass<UVremGameModeDefManager>();
			if (IsValid(GameModeManager))
			{
				GameModeManager->SetAndLoadGameModeDefinition(DefaultGameModeDefinition);
			}
		}
		else
		{
			UE_LOG(LogVrem, Warning, TEXT("AVremGameModeBase::InitGameState() GameState Invalid!"));
		}
	}
	else
	{
		UE_LOG(LogVrem, Warning, TEXT("AVremGameModeBase::InitGameState() DefaultGameModeDefinition Is Not Set!"));
	}
}


