// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VremWeaponHandlerInterface.generated.h"

struct FRecoilProfile;

UINTERFACE(MinimalAPI)
class UVremWeaponHandler : public UInterface
{
    GENERATED_BODY()
};

class VREM_API IVremWeaponHandler
{
    GENERATED_BODY()

public:
    // 원거리 무기 발사 시 호출
    virtual void OnWeaponFired(const FRecoilProfile& RecoilProfile) = 0;

    virtual void OnMeleeAttackStarted(int32 ComboIndex) = 0;
    virtual void OnMeleeAttackFinished() = 0;
};