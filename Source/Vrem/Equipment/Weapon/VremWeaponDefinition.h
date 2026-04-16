// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremWeaponDefinition.generated.h"

UENUM()
enum class EWeaponFireMode : uint8
{
    SemiAuto,
    FullAuto,
};

UCLASS()
class VREM_API UVremWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditDefaultsOnly, Category = "Firing")
    EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

    UPROPERTY(EditDefaultsOnly, Category = "Firing")
    float FireRate = 600.f;  // 분당 발사 수 (RPM)

    UPROPERTY(EditDefaultsOnly, Category = "Firing")
    float Range = 10000.f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float BaseDamage = 20.f;

    // 발사 간격 (초) 계산
    float GetFireInterval() const { return 60.f / FireRate; }
};
