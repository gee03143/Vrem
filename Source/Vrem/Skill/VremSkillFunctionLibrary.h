// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VremSkillBehavior.h"
#include "VremSkillFunctionLibrary.generated.h"

class AActor;
/**
 * 
 */
UCLASS()
class VREM_API UVremSkillFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	UFUNCTION(BlueprintCallable, Category = "Vrem|Skill", meta = (DeterminesOutputType = "PrjectileClass"))
	static AActor* SpawnAndLaunchProjectile(
		const FVremSkillActivationContext& Context,
		TSubclassOf<AActor> ProjectileClass,
		FVector SpawnLocation,
		FVector LaunchVelocity);

	UFUNCTION(BlueprintCallable, Category = "Vrem|Skill")
	static bool PredictGrenadeTrajectory(
		const FVremSkillActivationContext& Context,
		FVector LaunchLocation,
		FVector LaunchVelocity,
		float ProjectileRadius,
		float MaxSimTime,
		TArray<FVector>& OutPathPoints,
		FVector& OutImpactPoint,
		bool& bOutHit);
};
