// Fill out your copyright notice in the Description page of Project Settings.

#include "VremDebugHUDWidget.h"
#include "Vrem/Inventory/VremInventoryComponent.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"
#include "Vrem/Character/HealthComponent.h"
#include "GameFramework/Pawn.h"

void UVremDebugHUDWidget::NativeDestruct()
{
    UnbindFromCharacter();
    Super::NativeDestruct();
}

void UVremDebugHUDWidget::BindToCharacter(APawn* InPawn)
{
    UnbindFromCharacter();

    if (!IsValid(InPawn))
    {
        return;
    }

    if (UVremInventoryComponent* InventoryComponent = InPawn->FindComponentByClass<UVremInventoryComponent>())
    {
        CachedInventory = InventoryComponent;
        InventoryComponent->OnInventoryChanged.AddDynamic(this, &UVremDebugHUDWidget::HandleInventoryChanged);
    }

    if (UVremEquipmentComponent* EquipmentComponent = InPawn->FindComponentByClass<UVremEquipmentComponent>())
    {
        CachedEquipment = EquipmentComponent;
        EquipmentComponent->OnEquipmentUpdated.AddUObject(this, &UVremDebugHUDWidget::HandleEquipmentChanged);
    }

    if (UHealthComponent* HP = InPawn->FindComponentByClass<UHealthComponent>())
    {
        CachedHealth = HP;
        HP->OnHealthChanged.AddUObject(this, &UVremDebugHUDWidget::HandleHealthChanged);
    }

    OnInventoryRefreshed();
    OnEquipmentRefreshed();
}

void UVremDebugHUDWidget::UnbindFromCharacter()
{
    if (CachedInventory.IsValid())
    {
        CachedInventory->OnInventoryChanged.RemoveAll(this);
    }
    CachedInventory.Reset();

    if (CachedEquipment.IsValid())
    {
        CachedEquipment->OnEquipmentUpdated.RemoveAll(this);
    }
    CachedEquipment.Reset();

    if (CachedHealth.IsValid())
    {
        CachedHealth->OnHealthChanged.RemoveAll(this);
    }
    CachedHealth.Reset();
}

TArray<FVremInventoryEntryView> UVremDebugHUDWidget::GetInventoryEntries() const
{
    if (CachedInventory.IsValid())
    {
        return CachedInventory->GetInventoryEntries();
    }
    return {};
}

TArray<FVremEquipmentSlotView> UVremDebugHUDWidget::GetEquipmentEntries() const
{
    if (CachedEquipment.IsValid())
    {
        return CachedEquipment->GetEquipmentEntries();
    }
    return {};
}

FString UVremDebugHUDWidget::GetInventoryDebugString() const
{
    if (CachedInventory.IsValid())
    {
        return CachedInventory->GetInventoryItemsString();
    }
    return FString();
}

FString UVremDebugHUDWidget::GetEquipmentDebugString() const
{
    if (CachedEquipment.IsValid())
    {
        return CachedEquipment->GetEquipmentListString();
    }
    return FString();
}

void UVremDebugHUDWidget::HandleInventoryChanged()
{
    OnInventoryRefreshed();
}

void UVremDebugHUDWidget::HandleEquipmentChanged()
{
    OnEquipmentRefreshed();
}

void UVremDebugHUDWidget::HandleHealthChanged(float PrevHealth, float NewHealth)
{
    OnHealthUpdated(NewHealth, CachedHealth->GetMaxHealth());
}