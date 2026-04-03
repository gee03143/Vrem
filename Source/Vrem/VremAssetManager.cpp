// Fill out your copyright notice in the Description page of Project Settings.


#include "VremAssetManager.h"
#include "Inventory/VremItemDefinition.h"

UVremAssetManager& UVremAssetManager::Get()
{
	if (UVremAssetManager* VremAssetManager = Cast<UVremAssetManager>(GEngine->AssetManager))
	{
		return *VremAssetManager;
	}
	
	UE_LOG(LogVremAssetManager, Fatal, TEXT("Cannot use AssetManager if no AssetManagerClassName is defined! or AssetManagerClass is not UVremAssetManager"));
	return *NewObject<UVremAssetManager>(); // never calls this
}

UVremItemDefinition* UVremAssetManager::GetItemDefinition(const FPrimaryAssetId& Id)
{
	UVremItemDefinition* ItemDef = Cast<UVremItemDefinition>(GetPrimaryAssetObject(Id));
	if (ItemDef)
	{
		return ItemDef;
	}

	FSoftObjectPath Path = GetPrimaryAssetPath(Id);
	UObject* LoadedObj = Path.TryLoad();
	if (LoadedObj)
	{
		return Cast<UVremItemDefinition>(LoadedObj);
	}

	UE_LOG(LogVremAssetManager, Warning, TEXT("Failed to load ItemDefinition %s"), *Id.ToString());
	return nullptr;
}

