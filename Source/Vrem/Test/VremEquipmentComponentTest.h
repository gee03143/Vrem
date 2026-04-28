// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Animation/AnimInstance.h"
#include "VremEquipmentComponentTest.generated.h"

class AVremEquipmentActor;

UCLASS()
class UVremEquipmentTestListener : public UObject
{
    GENERATED_BODY()
public:
    int32 AttachedCount = 0;
    int32 DetachedCount = 0;
    AVremEquipmentActor* LastRangedActor = nullptr;
    AVremEquipmentActor* LastMeleeActor = nullptr;

    UFUNCTION() void HandleAttached(TSubclassOf<UAnimInstance> AnimLayerClass) { ++AttachedCount; }
    UFUNCTION() void HandleDetached(TSubclassOf<UAnimInstance> AnimLayerClass) { ++DetachedCount; }
    UFUNCTION() void HandleRangedChanged(AVremEquipmentActor* NewActor) { LastRangedActor = NewActor; }
    UFUNCTION() void HandleMeleeChanged(AVremEquipmentActor* NewActor) { LastMeleeActor = NewActor; }
};