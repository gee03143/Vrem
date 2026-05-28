// Fill out your copyright notice in the Description page of Project Settings.


#include "VremSkillFunctionLibrary.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Kismet/GameplayStatics.h"

AActor* UVremSkillFunctionLibrary::SpawnAndLaunchProjectile(
	const FVremSkillActivationContext& Context, 
	TSubclassOf<AActor> ProjectileClass, 
	FVector SpawnLocation, 
	FVector LaunchVelocity)
{
    if (IsValid(Context.Instigator) == false || ProjectileClass == nullptr)
    {
        return nullptr;
    }

    if (Context.Instigator->HasAuthority() == false)
    {
        return nullptr;
    }

    UWorld* World = Context.Instigator->GetWorld();
    if (IsValid(World) == false)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = Context.Instigator;
    Params.Instigator = Cast<APawn>(Context.Instigator);
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    const FRotator SpawnRotation = LaunchVelocity.Rotation();
    AActor* Grenade = World->SpawnActor<AActor>(
        ProjectileClass, SpawnLocation, SpawnRotation, Params);

    if (Grenade)
    {
        // TODO : Grenade -> Launch
        //Grenade->SetLaunchVelocity(LaunchVelocity);
    }
    return Grenade;
}

bool UVremSkillFunctionLibrary::PredictGrenadeTrajectory(
	const FVremSkillActivationContext& Context, FVector LaunchLocation, 
	FVector LaunchVelocity, 
	float ProjectileRadius, 
	float MaxSimTime, 
	TArray<FVector>& OutPathPoints,
	FVector& OutImpactPoint, 
	bool& bOutHit)
{
    OutPathPoints.Reset();
    OutImpactPoint = FVector::ZeroVector;
    bOutHit = false;

    if (IsValid(Context.Instigator) == false)
    {
        return false;
    }

    UWorld* World = Context.Instigator->GetWorld();
    if (IsValid(World) == false)
    {
        return false;
    }

    // FPredictProjectilePathParams 생성자 (반지름/시작/속도/시간/채널/무시액터)
    FPredictProjectilePathParams PredictParams(
        ProjectileRadius,
        LaunchLocation,
        LaunchVelocity,
        MaxSimTime > 0.f ? MaxSimTime : 2.f,
        ECC_Visibility,
        Context.Instigator);
    PredictParams.SimFrequency = 15.f;

    FPredictProjectilePathResult PredictResult;
    const bool bResult = UGameplayStatics::PredictProjectilePath(World, PredictParams, PredictResult);

    OutPathPoints.Reserve(PredictResult.PathData.Num());
    for (const FPredictProjectilePathPointData& Point : PredictResult.PathData)
    {
        OutPathPoints.Add(Point.Location);
    }

    bOutHit = PredictResult.HitResult.bBlockingHit;
    OutImpactPoint = bOutHit ? PredictResult.HitResult.ImpactPoint : PredictResult.LastTraceDestination.Location;

    return bResult;
}
