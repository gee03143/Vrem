// Fill out your copyright notice in the Description page of Project Settings.


#include "VremHUD.h"
#include "Engine/Canvas.h"
#include "GameplayTagAssetInterface.h"
#include "Vrem/VremGameplayTags.h"
#include "Vrem/Character/VremCharacter.h"

void AVremHUD::DrawHUD()
{
    Super::DrawHUD();

    if (ShouldDrawCrosshair())
    {
        DrawCrosshair();
    }
}

bool AVremHUD::ShouldDrawCrosshair() const
{
    APawn* Pawn = GetOwningPawn();
    if (!IsValid(Pawn)) return false;

    const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Pawn);
    if (!TagInterface) return false;

    // ADS 또는 Scoped 조준 중일 때만 크로스헤어 표시
    return TagInterface->HasMatchingGameplayTag(FVremGameplayTags::State_Aiming_ADS)
        || TagInterface->HasMatchingGameplayTag(FVremGameplayTags::State_Aiming_Scoped);
}

float AVremHUD::GetOwnerWeaponSpread() const
{
    APawn* Pawn = GetOwningPawn();
    AVremCharacter* Character = Cast<AVremCharacter>(Pawn);
    if (!IsValid(Character)) return 0.f;

    return Character->GetCurrentSpreadForUI();
}

void AVremHUD::DrawCrosshair()
{
    if (Canvas == nullptr)
    {
        return;
    }

    const float CenterX = Canvas->SizeX * 0.5f;
    const float CenterY = Canvas->SizeY * 0.5f;

    // 스프레드 값에 따라 간격을 확장
    const float Spread = GetOwnerWeaponSpread();
    const float DynamicGap = BaseGap + Spread * SpreadToPixelScale;

    // 상
    DrawLine(CenterX, CenterY - DynamicGap, CenterX, CenterY - DynamicGap - LineLength, CrosshairColor, LineThickness);
    // 하
    DrawLine(CenterX, CenterY + DynamicGap, CenterX, CenterY + DynamicGap + LineLength, CrosshairColor, LineThickness);
    // 좌
    DrawLine(CenterX - DynamicGap, CenterY, CenterX - DynamicGap - LineLength, CenterY, CrosshairColor, LineThickness);
    // 우
    DrawLine(CenterX + DynamicGap, CenterY, CenterX + DynamicGap + LineLength, CenterY, CrosshairColor, LineThickness);
}

