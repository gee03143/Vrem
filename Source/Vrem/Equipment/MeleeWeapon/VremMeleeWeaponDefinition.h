// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremMeleeWeaponDefinition.generated.h"

USTRUCT(BlueprintType)
struct FAttackSequence
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float Damage = 30.f;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float Range = 150.f;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float TraceRadius = 40.f;

    // 공격 모션 지속 시간 (이 시간 동안 MeleeAttacking 태그 유지, 입력 무시)
    UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo")
    float AttackDuration = 0.5f;

    // 쿨다운, Combo Window (이 시간이 지나면 콤보 리셋, 다음 공격은 단계 0부터)
    UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo")
    float AttackCooldown = 0.9f;

    // TODO: 이펙트/사운드 에셋 확보 후 추가
    // UPROPERTY(EditDefaultsOnly, Category = "Effects")
    // TObjectPtr<USoundBase> SwingSound;
    // TObjectPtr<UNiagaraSystem> HitImpactEffect;
};

USTRUCT(BlueprintType)
struct FMeleeProfile
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    TArray<FAttackSequence> AttackSequences;

    inline int32 GetComboCount() const { return AttackSequences.Num(); }
};

UCLASS()
class VREM_API UVremMeleeWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditDefaultsOnly, Category = "Melee")
    FMeleeProfile MeleeProfile;

    inline int32 GetComboCount() const { return MeleeProfile.GetComboCount(); }
    const FAttackSequence* GetSequenceAt(int32 InIndex) const 
    { 
        return MeleeProfile.AttackSequences.IsValidIndex(InIndex) ? &MeleeProfile.AttackSequences[InIndex] : nullptr;
    }
};
