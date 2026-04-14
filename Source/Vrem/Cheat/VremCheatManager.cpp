// Fill out your copyright notice in the Description page of Project Settings.


#include "VremCheatManager.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/VremAssetManager.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"


void UVremCheatManager::TestAssetSyncLoad()
{
	TSoftObjectPtr<UTexture2D> TestTexture(
		FSoftObjectPath(TEXT("/Engine/EngineResources/AICON-Green.AICON-Green")));
	
	UVremAssetManager::Get().LoadAssetSync<UTexture2D>(TestTexture);
}

void UVremCheatManager::PrintEquipmentList()
{
    APlayerController* PC = GetOuterAPlayerController();
    if (PC == nullptr || PC->GetPawn() == nullptr)
    {
        UE_LOG(LogVremEquipment, Warning, TEXT("UVremCheatManager::PrintEquipmentList Failed!"));
        return;
    }

    UVremEquipmentComponent* EquipmentComponent = PC->GetPawn()->FindComponentByClass<UVremEquipmentComponent>();
    if (IsValid(EquipmentComponent))
    {
        UE_LOG(LogVremEquipment, Warning, TEXT("%s"), *EquipmentComponent->GetEquipmentListString());
    }
}
