// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Vrem/Inventory/VremItemDefinition.h"
#include "VremInventoryComponentTest.generated.h"

class UVremItemInstance;
class UVremItemDefinition;

// Test-only listener for dynamic multicast delegates on UVremInventoryComponent.
// UCLASS cannot live inside #if WITH_AUTOMATION_WORKER (UHT requirement),
// so it is declared unconditionally. Bodies are gated in the .cpp/usage sites.
UCLASS()
class UVremInventoryTestListener : public UObject
{
    GENERATED_BODY()

public:
    bool bCreatedFired = false;
    UVremItemInstance* LastCreatedInstance = nullptr;

    bool bRemovedFired = false;
    FString LastRemovedDef;

    bool bChangedFired = false;
    int32 ChangedCount = 0;

    UFUNCTION()
    void HandleItemInstanceCreated(UVremItemInstance* ItemInstance)
    {
        bCreatedFired = true;
        LastCreatedInstance = ItemInstance;
    }

    UFUNCTION()
    void HandleItemInstanceRemoved(const UVremItemDefinition* ItemDef)
    {
        bRemovedFired = true;
        LastRemovedDef = ItemDef->GetName();
    }

    UFUNCTION()
    void HandleInventoryChanged()
    {
        bChangedFired = true;
        ++ChangedCount;
    }
};
