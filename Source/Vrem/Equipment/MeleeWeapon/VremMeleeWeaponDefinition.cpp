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
    }
}
#endif
