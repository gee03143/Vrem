// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremDodgeProfile.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right,
};

/*
* 
* RootMotionSource_ConstantForce
* - WorldDir x ForceStrength 의 force를 ForceDuration동안 적용
* - Montage에는 rootmotion off
* 
* Timing : 
* 0 -----------ForceDuration------DodgeDuration----------->DodgeDuration + Cooldown
* |					|					|								|
* | force 적용   force 종료			commit 종료					다음 회피 가능
* |									(다른 입력 무시)
*/
USTRUCT(BlueprintType)
struct FDodgeSequence
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;


	// force vector = WorldDir x ForceStrength
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0.0f))
	float ForceStrength = 4000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0.0f))
	float ForceDuration = 0.25f;

	// 회피 시작부터 여기까지 다른 입력 무시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0.0f))
	float DodgeDuration = 0.4f;

	// DodgeDuration이 끝나고 DodgeCooldown만큼 시간이 지나면 다음 회피 시작 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0.0f))
	float DodgeCooldown = 0.5f;

	// 회피 시작 후 무적 프레임 진입 시점
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "i-frame", meta = (ClampMin = 0.0f))
	float IFrameStartTime = 0.05f;

	// 회피 시작 후 무적 프레임 종료 시점
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "i-frame", meta = (ClampMin = 0.0f))
	float IFrameEndTime = 0.3f;
};

/**
 * 
 */
UCLASS(BlueprintType)
class VREM_API UVremDodgeProfile : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	FDodgeSequence ForwardSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	FDodgeSequence BackwardSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	FDodgeSequence LeftSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	FDodgeSequence RightSequence;

	const FDodgeSequence& GetDodgeSequence(EDodgeDirection Direction) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
