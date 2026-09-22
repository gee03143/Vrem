// Fill out your copyright notice in the Description page of Project Settings.


#include "VremMeleeWeaponDefinition.h"
#include "UObject/UnrealType.h"


#if WITH_EDITOR
void UVremMeleeWeaponDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    for (FAttackSequence& Seq : MeleeProfile.AttackSequences)
    {
        // CancelTime 은 AttackDuration 보다 작아야 의미 있음
        Seq.CancelTime = FMath::Min(Seq.CancelTime, Seq.AttackDuration);

        // HitTime 은 AttackDuration 보다 작아야 의미 있음
        Seq.HitTime = FMath::Min(Seq.HitTime, Seq.AttackDuration);

        // 일단 CancelTime < HitTime인 경우를 허용은 해 둠
        // 가급적 HitTime < CancelTime < AttackDuration이 되도록 구성, HitTime이 CancelTime보다 크면, 데미지 판정 전에 캔슬이 가능함
    }
}
#endif
