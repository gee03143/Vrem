// Fill out your copyright notice in the Description page of Project Settings.


#include "VremCameraSystem.h"

// Sets default values for this component's properties
UVremCameraSystem::UVremCameraSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVremCameraSystem::RequestSetCameraMode(UVremCameraMode* InVremCameraMode)
{
	SetTargetCameraMode(InVremCameraMode);
}

FVremCameraState UVremCameraSystem::GetBlendedCameraState()
{
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	// .. 일단 무조건 타깃 카메라 상태로 이동하도록 만들어보자
	if (TargetCameraMode.IsValid())
	{ 
		CurrentCameraState.TargetArmLength =
			FMath::FInterpTo(
				CurrentCameraState.TargetArmLength,
				TargetCameraMode->TargetArmLength,
				DeltaTime,
				10.f
			);

		CurrentCameraState.TargetFOV =
			FMath::FInterpTo(
				CurrentCameraState.TargetFOV,
				TargetCameraMode->TargetFOV,
				DeltaTime,
				10.f
			);

		CurrentCameraState.TargetSocketOffset =
			FMath::VInterpTo(
				CurrentCameraState.TargetSocketOffset,
				TargetCameraMode->TargetSocketOffset,
				DeltaTime,
				10.f
			);
	}

	TransientFOVKick = FMath::FInterpTo(TransientFOVKick, 0.f, DeltaTime, TransientFOVRecoverSpeed);
	TransientOffset = FMath::VInterpTo(TransientOffset, FVector::ZeroVector, DeltaTime, TransientOffsetRecoverSpeed);

	FVremCameraState Result = CurrentCameraState;
	Result.TargetFOV += TransientFOVKick;
	Result.TargetSocketOffset += TransientOffset;
	return Result;
}

bool UVremCameraSystem::IsBlending() const
{
	if (TargetCameraMode.IsValid())
	{
		bool bModeBlending = FMath::Abs(CurrentCameraState.TargetArmLength - TargetCameraMode->TargetArmLength) > SMALL_NUMBER ||
			FMath::Abs(CurrentCameraState.TargetFOV - TargetCameraMode->TargetFOV) > SMALL_NUMBER;

		if (bModeBlending)
		{
			return true;
		}
	}

	if (FMath::Abs(TransientFOVKick) > SMALL_NUMBER)
	{
		return true;
	}

	if (TransientOffset.IsNearlyZero() == false)
	{
		return true;
	}

	return false;
}

void UVremCameraSystem::AddTransientFOVKick(float FOVDelta, float RecoverSpeed /*= 8.f*/)
{
	TransientFOVKick += FOVDelta;
	TransientFOVRecoverSpeed = RecoverSpeed;
}

void UVremCameraSystem::AddTransientOffset(const FVector& OffsetDelta, float RecoverSpeed /*= 8.f*/)
{
	// camera shake용도로 만들어두긴 했는데 미사용..
	TransientOffset += OffsetDelta;
	TransientOffsetRecoverSpeed = RecoverSpeed;
}
