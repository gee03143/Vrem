// Fill out your copyright notice in the Description page of Project Settings.


#include "VremGameStateBase.h"

#include "VremGameModeDefManager.h"


AVremGameStateBase::AVremGameStateBase()
{
	GameModeManager = CreateDefaultSubobject<UVremGameModeDefManager>(TEXT("GameModeManager"));
}