// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VremHUD.generated.h"


// TODO: 추후 CommonUI 기반 UMG 위젯으로 마이그레이션 예정.
// 현재는 스프레드 시각 검증을 위한 임시 구현.
UCLASS()
class VREM_API AVremHUD : public AHUD
{
	GENERATED_BODY()
	
	public:
    virtual void DrawHUD() override;

protected:
    void DrawCrosshair();

    // 현재 소유 Pawn의 무기 스프레드 값 반환 (도 단위)
    float GetOwnerWeaponSpread() const;

    bool ShouldDrawCrosshair() const;

protected:
    // TODO: 추후 DataAsset 분리 고려..
    UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
    float BaseGap = 6.f;

    UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
    float LineLength = 8.f;

    UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
    float LineThickness = 2.f;

    // 스프레드(도)를 크로스헤어 간격(픽셀)에 매핑할 때 사용하는 배율
    UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
    float SpreadToPixelScale = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
    FLinearColor CrosshairColor = FLinearColor::White;
};
