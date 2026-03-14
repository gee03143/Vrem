// Fill out your copyright notice in the Description page of Project Settings.


#include "VremGameModeDefManager.h"

#include "VremGameModeDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremAssetManager.h"


// Sets default values for this component's properties
UVremGameModeDefManager::UVremGameModeDefManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UVremGameModeDefManager::SetAndLoadGameModeDefinition(const FPrimaryAssetId& GameModeToLoad)
{
	if (GameModeToLoad.IsValid() == false)
	{
		UE_LOG(LogVrem, Warning, TEXT("UVremGameModeDefManager::SetAndLoadGameModeDefinition Invalid GameModeId : [%s], NetMode : [%s]"), *GameModeToLoad.ToString(), *GetNetModeString(GetWorld()));
		return;
	}
	
	CurrentGameModeId = GameModeToLoad;
	LoadGameModeDefinition();
}

void UVremGameModeDefManager::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UVremGameModeDefManager, CurrentGameModeId);
}

void UVremGameModeDefManager::LoadGameModeDefinition()
{
	check(CurrentGameModeId.IsValid());
	
	UVremAssetManager& AssetManager = UVremAssetManager::Get();

	FStreamableDelegate Delegate =
	FStreamableDelegate::CreateUObject(this, &UVremGameModeDefManager::OnGameModeDefinitionLoaded);
	
	AssetManager.LoadPrimaryAsset(CurrentGameModeId, {}, Delegate);
}

void UVremGameModeDefManager::OnGameModeDefinitionLoaded()
{
	check(CurrentGameModeId.IsValid());
	
	UVremAssetManager& AssetManager = UVremAssetManager::Get();
	UVremGameModeDefinition* LoadedDefinition = AssetManager.GetPrimaryAssetObject<UVremGameModeDefinition>(CurrentGameModeId);
	if (LoadedDefinition != nullptr)
	{
		CachedGameModeDefinition = LoadedDefinition;
		OnGameModeDefinitionLoadedDelegate.Broadcast(LoadedDefinition);
	}
	else
	{
		UE_LOG(LogVrem, Warning, TEXT("Failed To Load GameModeDefinition, CurrentGameModeId : [%s], NetMode : [%s]"), *CurrentGameModeId.ToString(), *GetNetModeString(GetWorld()));
	}
}

void UVremGameModeDefManager::OnRep_CurrentGameModeId()
{
	LoadGameModeDefinition();
}
