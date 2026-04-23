// Fill out your copyright notice in the Description page of Project Settings.


#include "VremMeleeComponent.h"
#include "VremMeleeWeaponDefinition.h"
#include "Vrem/VremLogChannels.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "../Weapon/VremWeaponHandlerInterface.h"

// 디버그 CVar (무기 시스템과 네임스페이스 일관성 유지)
static TAutoConsoleVariable<int32> CVarDebugMeleeAttack(
    TEXT("vrem.DebugMeleeAttack"),
    0,
    TEXT("Enable Melee Attack Debug\n0: Off\n1: On"),
    ECVF_Cheat);

UVremMeleeComponent::UVremMeleeComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}

AActor* UVremMeleeComponent::GetWeaponOwner() const
{
    return GetOwner() ? GetOwner()->GetOwner() : nullptr;
}

AController* UVremMeleeComponent::GetInstigatorController() const
{
    APawn* Pawn = Cast<APawn>(GetWeaponOwner());
    return Pawn ? Pawn->GetController() : nullptr;
}

bool UVremMeleeComponent::IsAttacking() const
{
    return bIsAttacking;
}

bool UVremMeleeComponent::IsInComboWindow() const
{
    return bIsInComboWindow;
}

void UVremMeleeComponent::TryMeleeAttack()
{
    if (IsValid(MeleeDefinition) == false)
    {
        UE_LOG(LogVremWeapon, Warning, TEXT("TryMeleeAttack: MeleeDefinition is null"));
        return;
    }

    if (MeleeDefinition->GetComboCount() == 0)
    {
        UE_LOG(LogVremWeapon, Warning, TEXT("TryMeleeAttack: No attack sequences defined"));
        return;
    }

    // 공격 모션 중이면 무시
    if (bIsAttacking)
    {
        return;
    }

    ExecuteMeleeAttack();
}

void UVremMeleeComponent::ExecuteMeleeAttack()
{
    APlayerController* PC = Cast<APlayerController>(GetInstigatorController());
    if (IsValid(PC) == false)
    {
        UE_LOG(LogVremWeapon, Warning, TEXT("ExecuteMeleeAttack: Controller is null"));
        return;
    }

    if (MeleeDefinition->GetSequenceAt(CurrentComboIndex) == nullptr)
    {
        CurrentComboIndex = 0;
    }

    const int32 SequenceIndex = CurrentComboIndex;
    const FAttackSequence* Sequence = MeleeDefinition->GetSequenceAt(SequenceIndex);

    // 뷰포인트 계산 (히트 판정 기준)
    FVector ViewOrigin;
    FRotator ViewRotation;
    PC->GetPlayerViewPoint(ViewOrigin, ViewRotation);
    const FVector ViewDirection = ViewRotation.Vector();

    // 서버에 판정 위임
    ServerMeleeAttack(SequenceIndex, ViewOrigin, ViewDirection);

    // 로컬 상태 업데이트
    bIsAttacking = true;
    bIsInComboWindow = false;

    if (IVremWeaponHandler* Handler = Cast<IVremWeaponHandler>(GetWeaponOwner()))
    {
        Handler->OnMeleeAttackStarted(SequenceIndex);
    }

    // 타이머들 세팅
    FTimerDelegate DurationDelegate;
    DurationDelegate.BindUObject(this, &UVremMeleeComponent::OnAttackDurationFinished);
    GetWorld()->GetTimerManager().SetTimer(
        AttackDurationTimer, DurationDelegate,
        Sequence->AttackDuration, false);

    FTimerDelegate ComboDelegate;
    ComboDelegate.BindUObject(this, &UVremMeleeComponent::OnComboWindowFinished);
    GetWorld()->GetTimerManager().SetTimer(
        ComboWindowTimer, ComboDelegate,
        Sequence->AttackCooldown, false);

    // 콤보 인덱스 증가 (마지막이면 리셋)
    CurrentComboIndex = (CurrentComboIndex + 1) % MeleeDefinition->GetComboCount();
}

void UVremMeleeComponent::OnAttackDurationFinished()
{
    bIsAttacking = false;
    bIsInComboWindow = true;  // 이제 콤보 윈도우 진입

    if (IVremWeaponHandler* Handler = Cast<IVremWeaponHandler>(GetWeaponOwner()))
    {
        Handler->OnMeleeAttackFinished();
    }
}

void UVremMeleeComponent::OnComboWindowFinished()
{
    bIsInComboWindow = false;
    CurrentComboIndex = 0;  // 콤보 리셋
}

void UVremMeleeComponent::ServerMeleeAttack_Implementation(int32 ComboIndex, FVector ViewOrigin, FVector ViewDirection)
{
    PerformMeleeHitDetection(ComboIndex, ViewOrigin, ViewDirection);
    MulticastOnMeleeAttack(ComboIndex);
}

void UVremMeleeComponent::PerformMeleeHitDetection(int32 ComboIndex, const FVector& ViewOrigin, const FVector& ViewDirection)
{
    if (IsValid(MeleeDefinition) == false) return;

    const FAttackSequence* Sequence = MeleeDefinition->GetSequenceAt(ComboIndex);
    if (Sequence == nullptr) return;

    const bool bShowDebug = CVarDebugMeleeAttack.GetValueOnGameThread() > 0;

    FVector TraceEnd = ViewOrigin + ViewDirection * Sequence->Range;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());              // EquipmentActor
    QueryParams.AddIgnoredActor(GetOwner()->GetOwner());  // Character

    FHitResult HitResult;
    const bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        ViewOrigin,
        TraceEnd,
        FQuat::Identity,
        ECC_Visibility,
        FCollisionShape::MakeSphere(Sequence->TraceRadius),
        QueryParams
    );

    if (bShowDebug)
    {
        DrawDebugCapsule(
            GetWorld(),
            (ViewOrigin + TraceEnd) * 0.5f,
            (TraceEnd - ViewOrigin).Size() * 0.5f,
            Sequence->TraceRadius,
            FRotationMatrix::MakeFromZ(ViewDirection).ToQuat(),
            bHit ? FColor::Green : FColor::Red,
            false, 1.f, 0, 1.f);
    }

    if (bHit && IsValid(HitResult.GetActor()))
    {
        FPointDamageEvent DamageEvent;
        DamageEvent.HitInfo = HitResult;
        DamageEvent.ShotDirection = ViewDirection;

        HitResult.GetActor()->TakeDamage(
            Sequence->Damage,
            DamageEvent,
            GetInstigatorController(),
            GetOwner());

        UE_LOG(LogVremWeapon, Log, TEXT("Melee hit: %s (ComboIndex=%d, Damage=%.1f)"),
            *HitResult.GetActor()->GetName(), ComboIndex, Sequence->Damage);
    }
}

void UVremMeleeComponent::MulticastOnMeleeAttack_Implementation(int32 ComboIndex)
{
    // TODO: 이펙트/사운드 재생 (모든 클라이언트)
    // const FAttackSequence* Sequence = MeleeDefinition->GetSequenceAt(ComboIndex);
    // if (Sequence && Sequence->AttackMontage) → PlayMontage
    // MeleeDefinition->SwingSound, HitImpactEffect 스폰

    UE_LOG(LogVremWeapon, Log, TEXT("MulticastOnMeleeAttack ComboIndex=%d"), ComboIndex);
}