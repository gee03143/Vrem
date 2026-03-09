// Fill out your copyright notice in the Description page of Project Settings.


#include "VremAssetManager.h"

UVremAssetManager& UVremAssetManager::Get()
{
	if (UVremAssetManager* VremAssetManager = Cast<UVremAssetManager>(GEngine->AssetManager))
	{
		return *VremAssetManager;
	}
	
	UE_LOG(LogVremAssetManager, Fatal, TEXT("Cannot use AssetManager if no AssetManagerClassName is defined! or AssetManagerClass is not UVremAssetManager"));
	return *NewObject<UVremAssetManager>(); // never calls this
}

