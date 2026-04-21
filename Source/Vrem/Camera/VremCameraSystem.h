// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VremCameraMode.h"
#include "VremCameraSystem.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremCameraSystem : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVremCameraSystem();

public:	

	void SetTargetCameraMode(UVremCameraMode* InVremCameraMode) { TargetCameraMode = InVremCameraMode; }
	FVremCameraState GetBlendedCameraState();
	bool IsBlending() const;

	void AddTransientFOVKick(float FOVDelta, float RecoverSpeed = 8.f);
	void AddTransientOffset(const FVector& OffsetDelta, float RecoverSpeed = 8.f);

protected:
	TWeakObjectPtr<UVremCameraMode> TargetCameraMode;

	FVremCameraState CurrentCameraState;
		
	float TransientFOVKick = 0.f;
	float TransientFOVRecoverSpeed = 8.f;

	// camera shake용도로 만들어두긴 했는데 미사용..
	FVector TransientOffset = FVector::ZeroVector;
	float TransientOffsetRecoverSpeed = 8.f;
};