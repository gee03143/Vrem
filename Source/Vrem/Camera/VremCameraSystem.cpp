// Fill out your copyright notice in the Description page of Project Settings.


#include "VremCameraSystem.h"

// Sets default values for this component's properties
UVremCameraSystem::UVremCameraSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const FVremCameraState& UVremCameraSystem::GetBlendedCameraState()
{
	// .. 일단 무조건 타깃 카메라 상태로 이동하도록 만들어보자
	CurrentCameraState.TargetArmLength =
		FMath::FInterpTo(
			CurrentCameraState.TargetArmLength,
			TargetCameraMode->TargetArmLength,
			GetWorld()->GetDeltaSeconds(),
			10.f
		);

	CurrentCameraState.TargetFOV =
		FMath::FInterpTo(
			CurrentCameraState.TargetFOV,
			TargetCameraMode->TargetFOV,
			GetWorld()->GetDeltaSeconds(),
			10.f
		);

	return CurrentCameraState;
}

bool UVremCameraSystem::IsBlending() const
{
	if (TargetCameraMode.IsValid() == false)
	{
		return false;
	}

	return FMath::Abs(CurrentCameraState.TargetArmLength - TargetCameraMode->TargetArmLength) > SMALL_NUMBER ||
		FMath::Abs(CurrentCameraState.TargetFOV - TargetCameraMode->TargetFOV) > SMALL_NUMBER;
}