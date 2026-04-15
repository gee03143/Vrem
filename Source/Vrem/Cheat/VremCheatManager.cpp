// Fill out your copyright notice in the Description page of Project Settings.


#include "VremCheatManager.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/VremAssetManager.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"
#include "Vrem/Equipment/VremEquipmentDefinition.h"

#define REGISTER_CHEAT_CMD(Name, Help, Func) \
    ConsoleManager.RegisterConsoleCommand(TEXT(Name), TEXT(Help), \
        FConsoleCommandWithArgsDelegate::CreateUObject(this, &UVremCheatManager::Func)); \
    RegisteredCommands.Add(TEXT(Name))

// VremCheatManager.cpp
void UVremCheatManager::InitCheatManager()
{
    Super::InitCheatManager();

    IConsoleManager& ConsoleManager = IConsoleManager::Get();

    REGISTER_CHEAT_CMD("Vrem.Equipment.SetCurrentWeapon", "Set current weapon by slot index", TestSetCurrentWeapon_Command);
    REGISTER_CHEAT_CMD("Vrem.Equipment.Equip", "Equip item by slot index and asset name", EquipItem_Command);
    REGISTER_CHEAT_CMD("Vrem.Equipment.Unequip", "Unequip item by slot index", UnequipItem_Command);
    REGISTER_CHEAT_CMD("Vrem.Equipment.Print", "Print equipment list", PrintEquipmentList_Command);
}

void UVremCheatManager::BeginDestroy()
{
    IConsoleManager& ConsoleManager = IConsoleManager::Get();
    for (const FString& Cmd : RegisteredCommands)
    {
        ConsoleManager.UnregisterConsoleObject(*Cmd);
    }
    RegisteredCommands.Empty();

    Super::BeginDestroy();
}

void UVremCheatManager::TestAssetSyncLoad()
{
	TSoftObjectPtr<UTexture2D> TestTexture(
		FSoftObjectPath(TEXT("/Engine/EngineResources/AICON-Green.AICON-Green")));
	
	UVremAssetManager::Get().LoadAssetSync<UTexture2D>(TestTexture);
}

void UVremCheatManager::TestEquipItem(int32 SlotIndex, const FString& EquipmentDefinitionName)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (PC == nullptr || PC->GetPawn() == nullptr)
    {
        return;
    }

    UVremEquipmentComponent* EquipmentComponent = PC->GetPawn()->FindComponentByClass<UVremEquipmentComponent>();
    if (IsValid(EquipmentComponent) == false)
    {
        return;
    }

    const FString& EquipmentDefinitionPath = 
        FString::Printf(TEXT("/Game/Weapons/EquipmentDefinition/%s.%s"), *EquipmentDefinitionName, *EquipmentDefinitionName);

    UVremEquipmentDefinition* Def = LoadObject<UVremEquipmentDefinition>(nullptr, *EquipmentDefinitionPath);
    if (IsValid(Def) == false)
    {
        UE_LOG(LogVremEquipment, Warning, TEXT("EquipItem: Failed to load %s"), *EquipmentDefinitionPath);
        return;
    }

    EquipmentComponent->ServerTryEquipItem(Def, SlotIndex);
}

void UVremCheatManager::TestUnequipItem(int32 SlotIndex)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (PC == nullptr || PC->GetPawn() == nullptr)
    {
        return;
    }

    UVremEquipmentComponent* EquipmentComponent = PC->GetPawn()->FindComponentByClass<UVremEquipmentComponent>();
    if (IsValid(EquipmentComponent))
    {
        EquipmentComponent->ServerTryUnequipItem(SlotIndex);
    }
}

void UVremCheatManager::TestSetCurrentWeapon(int32 SlotIndex)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (PC == nullptr || PC->GetPawn() == nullptr)
    {
        return;
    }

    UVremEquipmentComponent* EquipmentComponent = PC->GetPawn()->FindComponentByClass<UVremEquipmentComponent>();
    if (IsValid(EquipmentComponent))
    {
        EquipmentComponent->ServerSetCurrentWeapon(SlotIndex);
    }
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

void UVremCheatManager::TestSetCurrentWeapon_Command(const TArray<FString>& Args)
{
    if (Args.Num() < 1)
    {
        UE_LOG(LogVremEquipment, Warning, TEXT("Usage: Vrem.Equipment.SetCurrentWeapon <SlotIndex>"));
        return;
    }

    const int32 SlotIndex = FCString::Atoi(*Args[0]);
    TestSetCurrentWeapon(SlotIndex);
}

void UVremCheatManager::EquipItem_Command(const TArray<FString>& Args)
{
    if (Args.Num() < 2)
    {
        UE_LOG(LogVremEquipment, Warning, TEXT("Usage: Vrem.Equipment.Equip <SlotIndex> <AssetName>"));
        return;
    }

    const int32 SlotIndex = FCString::Atoi(*Args[0]);
    TestEquipItem(SlotIndex, Args[1]);
}

void UVremCheatManager::UnequipItem_Command(const TArray<FString>& Args)
{
    if (Args.Num() < 1)
    {
        UE_LOG(LogVremEquipment, Warning, TEXT("Usage: Vrem.Equipment.Unequip <SlotIndex>"));
        return;
    }

    const int32 SlotIndex = FCString::Atoi(*Args[0]);
    TestUnequipItem(SlotIndex);
}

void UVremCheatManager::PrintEquipmentList_Command(const TArray<FString>& Args)
{
    PrintEquipmentList();
}

#undef REGISTER_CHEAT_CMD