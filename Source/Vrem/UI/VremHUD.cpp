// Fill out your copyright notice in the Description page of Project Settings.


#include "VremHUD.h"
#include "Engine/Canvas.h"
#include "GameplayTagAssetInterface.h"
#include "Vrem/VremGameplayTags.h"
#include "Vrem/Character/VremCharacter.h"
#include "VremDebugHUDWidget.h"

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

void AVremHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetOwningPlayerController();
    if (!IsValid(PC))
    {
        return;
    }

    // 폰 변경 콜백 구독 — 추후 점유 변경 시 재바인딩
    PC->OnPossessedPawnChanged.AddDynamic(this, &AVremHUD::HandlePossessedPawnChanged);

    // 위젯 생성 + viewport attach
    if (DebugHUDWidgetClass)
    {
        DebugHUDWidget = CreateWidget<UVremDebugHUDWidget>(PC, DebugHUDWidgetClass);
        if (DebugHUDWidget)
        {
            DebugHUDWidget->AddToViewport(0);
        }
    }

    // 이미 폰을 점유 중이면 즉시 바인딩
    if (APawn* CurrentPawn = PC->GetPawn())
    {
        HandlePossessedPawnChanged(nullptr, CurrentPawn);
    }
}

void AVremHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (APlayerController* PC = GetOwningPlayerController())
    {
        PC->OnPossessedPawnChanged.RemoveDynamic(this, &AVremHUD::HandlePossessedPawnChanged);
    }

    if (DebugHUDWidget)
    {
        DebugHUDWidget->UnbindFromCharacter();
        DebugHUDWidget->RemoveFromParent();
        DebugHUDWidget = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AVremHUD::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    if (!DebugHUDWidget)
    {
        return;
    }

    if (OldPawn)
    {
        DebugHUDWidget->UnbindFromCharacter();
    }

    if (DebugHUDWidget)
    {
        DebugHUDWidget->BindToCharacter(NewPawn);
    }
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

