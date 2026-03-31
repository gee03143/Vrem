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
	const FVremCameraState& GetBlendedCameraState();
	bool IsBlending() const;

protected:
	TWeakObjectPtr<UVremCameraMode> TargetCameraMode;

	FVremCameraState CurrentCameraState;
		
};