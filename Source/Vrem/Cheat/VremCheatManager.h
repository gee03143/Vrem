// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "VremCheatManager.generated.h"

/**
 * 
 */
UCLASS()
class VREM_API UVremCheatManager : public UCheatManager
{
	GENERATED_BODY()

public: 
	void InitCheatManager() override;
	void BeginDestroy() override;

public:
	UFUNCTION(exec)
	void TestAssetSyncLoad();
	

#pragma region InventorySystem
public:
	void TestAddItem(const FString& ItemPath);
	void TestRemoveItem(const FString& ItemPath);

	void PrintInventoryList();
private:
	void AddItem_Command(const TArray<FString>& Args);
	void RemoveItem_Command(const TArray<FString>& Args);
	void PrintInventoryList_Command(const TArray<FString>& Args);
	
#pragma endregion InventorySystem

#pragma region EquipmentSystem
public:
	void TestEquipItem(int32 SlotIndex, const FString& EquipmentDefinitionPath);
	void TestUnequipItem(int32 SlotIndex);
	void TestSetCurrentWeapon(int32 SlotIndex);
	void PrintEquipmentList();

private:
	void TestSetCurrentWeapon_Command(const TArray<FString>& Args);
	void EquipItem_Command(const TArray<FString>& Args);
	void UnequipItem_Command(const TArray<FString>& Args);
	void PrintEquipmentList_Command(const TArray<FString>& Args);

#pragma endregion EquipmentSystem

private:
	TArray<FString> RegisteredCommands;
};
