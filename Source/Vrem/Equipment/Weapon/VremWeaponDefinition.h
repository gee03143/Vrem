// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremWeaponDefinition.generated.h"

class UNiagaraSystem;

UENUM()
enum class EWeaponFireMode : uint8
{
    SemiAuto,
    FullAuto,
};

USTRUCT(BlueprintType)
struct FRecoilProfile
{
    GENERATED_BODY()

    // 발당 수직 반동 (도 단위, 양수 = 조준선이 위로 올라감)
    UPROPERTY(EditDefaultsOnly, Category = "Recoil")
    float VerticalKick = 1.5f;

    // 발당 수평 반동 (도 단위, ±범위로 랜덤)
    UPROPERTY(EditDefaultsOnly, Category = "Recoil")
    float HorizontalKick = 0.5f;

    // 수직 반동 랜덤 편차 (VerticalKick 기준 ±범위)
    UPROPERTY(EditDefaultsOnly, Category = "Recoil")
    float VerticalVariance = 0.3f;

    // 카메라 FOV 펀치 (음수 = 순간 줌인, 양수 = 줌아웃)
    UPROPERTY(EditDefaultsOnly, Category = "Recoil|Camera")
    float FOVKick = -2.0f;

    // FOV 복귀 속도 (초당 보간 속도)
    UPROPERTY(EditDefaultsOnly, Category = "Recoil|Camera")
    float FOVRecoverSpeed = 8.0f;
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

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> BulletTrailEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<UNiagaraSystem>> ImpactEffects;

    UPROPERTY(EditDefaultsOnly, Category = "Recoil")
    FRecoilProfile RecoilProfile;

    // 발사 간격 (초) 계산
    float GetFireInterval() const { return 60.f / FireRate; }
};
