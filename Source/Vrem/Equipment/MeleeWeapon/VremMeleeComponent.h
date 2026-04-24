// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VremMeleeComponent.generated.h"

class UVremMeleeWeaponDefinition;
struct FAttackSequence;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremMeleeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UVremMeleeComponent();

    // 외부(Character) 진입점
    void TryMeleeAttack();

protected:
    // 로컬 예측 실행 (쿨다운/콤보 로직 포함) → 서버에 요청
    void ExecuteMeleeAttack();

    UFUNCTION(Server, Reliable)
    void ServerMeleeAttack(int32 ComboIndex, FVector ViewOrigin, FVector ViewDirection);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastOnMeleeAttack(int32 ComboIndex);

    // 서버 히트 판정 (Sphere Trace)
    void PerformMeleeHitDetection(int32 ComboIndex, const FVector& ViewOrigin, const FVector& ViewDirection);

    // 쿨다운/콤보 상태
    bool IsAttacking() const;    // AttackDuration 중 (입력 무시)
    bool IsInComboWindow() const; // Duration ~ Cooldown 사이

    void OnAttackDurationFinished();  // 공격 모션 종료 시점
    void OnComboWindowFinished();     // 쿨다운 종료 → 콤보 리셋

    AController* GetInstigatorController() const;
    AActor* GetWeaponOwner() const;

protected:
    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UVremMeleeWeaponDefinition> MeleeDefinition;

private:
    int32 CurrentComboIndex = 0;           // 다음 실행할 콤보 단계
    bool bIsAttacking = false;              // AttackDuration 중인가
    bool bIsInComboWindow = false;          // Cooldown 중인가 (다음 입력 시 콤보 이어감)

    FTimerHandle AttackDurationTimer;
    FTimerHandle ComboWindowTimer;

#if WITH_AUTOMATION_WORKER
public:
    // Definition 주입
    void SetMeleeDefinition_ForTest(UVremMeleeWeaponDefinition* InDef)
    {
        MeleeDefinition = InDef;
    }

    // 상태 조회
    int32 GetCurrentComboIndex_ForTest() const { return CurrentComboIndex; }
    bool IsAttacking_ForTest() const { return bIsAttacking; }
    bool IsInComboWindow_ForTest() const { return bIsInComboWindow; }

    // 타이머 콜백 수동 호출
    void TriggerAttackDurationFinished_ForTest() { OnAttackDurationFinished(); }
    void TriggerComboWindowFinished_ForTest() { OnComboWindowFinished(); }

    // 테스트 전용: ExecuteMeleeAttack의 상태 변화 부분만 수동 시뮬레이션
    // (Controller 의존 없음, 타이머 설정 없음)
    void SimulateAttackStart_ForTest();
#endif
};
