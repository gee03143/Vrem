// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VremGameModeDefManager.generated.h"

class UVremGameModeDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameModeDefinitionLoaded, const UVremGameModeDefinition*);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremGameModeDefManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVremGameModeDefManager();

public:
	bool IsGameModeDefinitionLoaded() const { return CachedGameModeDefinition.Get() != nullptr; }
	void SetAndLoadGameModeDefinition(const FPrimaryAssetId& GameModeToLoad);

	UVremGameModeDefinition* GetGameModeDefinition() { return CachedGameModeDefinition; }

	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	void LoadGameModeDefinition();
	void OnGameModeDefinitionLoaded();

public:
	FOnGameModeDefinitionLoaded OnGameModeDefinitionLoadedDelegate;

protected:
	UPROPERTY(ReplicatedUsing=OnRep_CurrentGameModeId)
	FPrimaryAssetId CurrentGameModeId;

	UFUNCTION()
	void OnRep_CurrentGameModeId();

private:
	TObjectPtr<UVremGameModeDefinition> CachedGameModeDefinition;
};
