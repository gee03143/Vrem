// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VremDebugHUDWidget.generated.h"

struct FVremInventoryEntryView;
class UVremInventoryComponent;
class UVremEquipmentComponent;
class UHealthComponent;
/**
 * 
 */
UCLASS()
class VREM_API UVremDebugHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    void BindToCharacter(APawn* InPawn);
    void UnbindFromCharacter();

protected:
    // UUserWidget interface
    virtual void NativeDestruct() override;

protected:
    // ---- BP에서 구현 (BlueprintImplementableEvent) ----
    UFUNCTION(BlueprintImplementableEvent, Category = "Vrem|HUD")
    void OnInventoryRefreshed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Vrem|HUD")
    void OnEquipmentRefreshed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Vrem|HUD")
    void OnHealthUpdated(float NewHealth, float MaxHealth);

protected:
    UFUNCTION(BlueprintPure, Category = "Vrem|HUD")
    TArray<FVremInventoryEntryView> GetInventoryEntries() const;

    UFUNCTION(BlueprintPure, Category = "Vrem|HUD")
    TArray<FVremEquipmentSlotView> GetEquipmentEntries() const;

    UFUNCTION(BlueprintPure, Category = "Vrem|HUD")
    FString GetInventoryDebugString() const;

    UFUNCTION(BlueprintPure, Category = "Vrem|HUD")
    FString GetEquipmentDebugString() const;

private:
    UFUNCTION()
    void HandleInventoryChanged();
    void HandleEquipmentChanged();
    void HandleHealthChanged(float PrevHealth, float NewHealth);

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UVremInventoryComponent> CachedInventory;

    UPROPERTY(Transient)
    TWeakObjectPtr<UVremEquipmentComponent> CachedEquipment;

    UPROPERTY(Transient)
    TWeakObjectPtr<UHealthComponent> CachedHealth;
};
