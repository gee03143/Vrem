// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremMeleeWeaponDefinition.generated.h"

class UAnimMontage;

/*
AttackMontage:  [─ swing ─][── return to idle ──]
                            ↑
                     여기서부터 캔슬 가능 (=다음 시퀀스 진행)

				0 ─── CancelTime ─── AttackDuration ─→
				│        │                │
				│ commit 구간      cancel 구간
				│ 입력 무시        입력 = 다음 콤보
				│
				│ ← 연타해도 commit 동안엔 무시됨
				│   다음 시퀀스로 의도치 않게 안 넘어감
*/

USTRUCT(BlueprintType)
struct FAttackSequence
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (ClampMin = "0.0"))
    TObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
    float Damage = 30.f;

    UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.0"))
    float Range = 150.f;

    UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.0"))
    float TraceRadius = 40.f;

    // 공격 몽타주의 전체 길이 (Swing + Return to Idle)
    UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo", meta = (ClampMin = "0.0"))
    float AttackDuration = 0.5f;

    // 공격 시작 후 다음 콤보로 캔슬 가능한 시점
    // Swing 종료/ Return to Idle 시작 지점
    // 이 시점 이전의 입력은 무시
    // 이후 입력은 Return to Idle을 캔슬하고 다음 시퀸스로 이행
    UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo", meta=(ClampMin="0.0"))
    float CancelTime = 0.4f;

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

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
