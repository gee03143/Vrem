// Fill out your copyright notice in the Description page of Project Settings.


#include "VremCheatManager.h"

#include "Vrem/VremAssetManager.h"


void UVremCheatManager::TestAssetSyncLoad()
{
	TSoftObjectPtr<UTexture2D> TestTexture(
		FSoftObjectPath(TEXT("/Engine/EngineResources/AICON-Green.AICON-Green")));
	
	UVremAssetManager::Get().LoadAssetSync<UTexture2D>(TestTexture);
}
