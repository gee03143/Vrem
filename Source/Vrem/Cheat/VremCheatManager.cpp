// Fill out your copyright notice in the Description page of Project Settings.


#include "VremCheatManager.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/VremAssetManager.h"
#include "Vrem/Inventory/VremInventoryComponent.h"
#include "Vrem/Inventory/VremItemDefinition.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"
#include "Vrem/Equipment/VremEquipmentDefinition.h"
#include "GameplayTagAssetInterface.h"

#define REGISTER_CHEAT_CMD(Name, Help, Func) \
    ConsoleManager.RegisterConsoleCommand(TEXT(Name), TEXT(Help), \
        FConsoleCommandWithArgsDelegate::CreateUObject(this, &UVremCheatManager::Func)); \
    RegisteredCommands.Add(TEXT(Name))

// VremCheatManager.cpp
void UVremCheatManager::InitCheatManager()
{
    Super::InitCheatManager();

    IConsoleManager& ConsoleManager = IConsoleManager::Get();

    REGISTER_CHEAT_CMD("Vrem.Inventory.AddItem", "Add Item to Inventory", AddItem_Command);
    REGISTER_CHEAT_CMD("Vrem.Inventory.RemoveItem", "Remove Item from Inventory", RemoveItem_Command);
    REGISTER_CHEAT_CMD("Vrem.Inventory.Print", "Print Inventory Items", PrintInventoryList_Command);

    REGISTER_CHEAT_CMD("Vrem.Equipment.SetCurrentWeapon", "Set current weapon by slot index", TestSetCurrentWeapon_Command);
    REGISTER_CHEAT_CMD("Vrem.Equipment.Equip", "Equip item by slot index and asset name", EquipItem_Command);
    REGISTER_CHEAT_CMD("Vrem.Equipment.Unequip", "Unequip item by slot index", UnequipItem_Command);
    REGISTER_CHEAT_CMD("Vrem.Equipment.Print", "Print equipment list", PrintEquipmentList_Command);

    REGISTER_CHEAT_CMD("Vrem.State.Print", "Print character state tags", PrintStateTags_Command);
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

void UVremCheatManager::TestAddItem(const FString& ItemPath)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (PC == nullptr || PC->GetPawn() == nullptr)
    {
        return;
    }

    UVremInventoryComponent* InventoryComponent = PC->GetPawn()->FindComponentByClass<UVremInventoryComponent>();
    if (IsValid(InventoryComponent) == false)
    {
        UE_LOG(LogVremInventory, Warning, TEXT("UVremCheatManager::TestAddItem Failed!"));
        return;
    }

    const FString& ItemDefinitionPath =
        FString::Printf(TEXT("/Game/Weapons/%s.%s"), *ItemPath, *ItemPath);

    const UVremItemDefinition* Def = LoadObject<UVremItemDefinition>(nullptr, *ItemDefinitionPath);
    if (IsValid(Def) == false)
    {
        UE_LOG(LogVremInventory, Warning, TEXT("TestAddItem: Failed to load %s"), *ItemDefinitionPath);
        return;
    }

    InventoryComponent->ServerAddItemToInventory(Def);
}

void UVremCheatManager::TestRemoveItem(const FString& ItemPath)
{
    APlayerController* PC = GetOuterAPlayerController();
    if (PC == nullptr || PC->GetPawn() == nullptr)
    {
        return;
    }

    UVremInventoryComponent* InventoryComponent = PC->GetPawn()->FindComponentByClass<UVremInventoryComponent>();
    if (IsValid(InventoryComponent) == false)
    {
        UE_LOG(LogVremInventory, Warning, TEXT("UVremCheatManager::TestRemoveItem Failed!"));
        return;
    }

    const FString& ItemDefinitionPath =
        FString::Printf(TEXT("/Game/Weapons/%s.%s"), *ItemPath, *ItemPath);

    const UVremItemDefinition* Def = LoadObject<UVremItemDefinition>(nullptr, *ItemDefinitionPath);
    if (IsValid(Def) == false)
    {
        UE_LOG(LogVremInventory, Warning, TEXT("TestAddItem: Failed to load %s"), *ItemDefinitionPath);
        return;
    }

    InventoryComponent->ServerRemoveItemFromInventory(Def);
}

void UVremCheatManager::PrintInventoryList()
{
    APlayerController* PC = GetOuterAPlayerController();
    if (PC == nullptr || PC->GetPawn() == nullptr)
    {
        UE_LOG(LogVremInventory, Warning, TEXT("UVremCheatManager::PrintInventoryList Failed!"));
        return;
    }

    UVremInventoryComponent* InventoryComponent = PC->GetPawn()->FindComponentByClass<UVremInventoryComponent>();
    if (IsValid(InventoryComponent))
    {
        UE_LOG(LogVremInventory, Warning, TEXT("%s"), *InventoryComponent->GetInventoryItemsString());
    }
}

void UVremCheatManager::AddItem_Command(const TArray<FString>& Args)
{
    if (Args.Num() < 1)
    {
        UE_LOG(LogVremInventory, Warning, TEXT("Usage: Vrem.Inventory.AddItem <AssetName>"));
        return;
    }

    const int32 SlotIndex = FCString::Atoi(*Args[0]);
    TestAddItem(Args[0]);
}

void UVremCheatManager::RemoveItem_Command(const TArray<FString>& Args)
{
    if (Args.Num() < 1)
    {
        UE_LOG(LogVremInventory, Warning, TEXT("Usage: Vrem.Inventory.AddItem <AssetName>"));
        return;
    }

    const int32 SlotIndex = FCString::Atoi(*Args[0]);
    TestRemoveItem(Args[0]);
}

void UVremCheatManager::PrintInventoryList_Command(const TArray<FString>& Args)
{
    PrintInventoryList();
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

void UVremCheatManager::PrintCharacterStates()
{
    APlayerController* PC = GetOuterAPlayerController();
    if (!PC || !PC->GetPawn()) return;

    IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(PC->GetPawn());
    if (!TagInterface) return;

    FGameplayTagContainer Container;
    TagInterface->GetOwnedGameplayTags(Container);

    UE_LOG(LogVrem, Log, TEXT("State Tags: %s"), *Container.ToStringSimple());
}

void UVremCheatManager::PrintStateTags_Command(const TArray<FString>& Args)
{
    PrintCharacterStates();
}

#undef REGISTER_CHEAT_CMD