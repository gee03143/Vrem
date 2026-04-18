// Fill out your copyright notice in the Description page of Project Settings.


#include "VremWeaponComponent.h"
#include "VremWeaponDefinition.h"
#include "Engine/DamageEvents.h"
#include "Vrem/VremLogChannels.h"

static TAutoConsoleVariable<int32> CVarDebugCharacterShooting(
    TEXT("vrem.DebugCharacterShooting"),
    0,
    TEXT("Enable Character Shooting Debug\n")
    TEXT("0: Off\n")
    TEXT("1: On"),
    ECVF_Cheat);

// Sets default values for this component's properties
UVremWeaponComponent::UVremWeaponComponent()
{
	SetIsReplicatedByDefault(true);
}

void UVremWeaponComponent::Fire()
{
    if (IsValid(WeaponDefinition) == false || CanFire() == false)
    {
        return;
    }

    bWantsToFire = true;
    ExecuteFire();
    return;
}

void UVremWeaponComponent::StopFire()
{
    bWantsToFire = false;
}

void UVremWeaponComponent::ExecuteFire()
{
	APlayerController* PC = Cast<APlayerController>(GetInstigatorController());
	if (IsValid(PC) == false)
	{
        UE_LOG(LogVremWeapon, Warning, TEXT("UVremWeaponComponent::ExecuteFire Controller is nullptr"));
		return;
	}

	FVector ViewOrigin;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewOrigin, ViewRotation);

	ServerFire(ViewOrigin, ViewRotation.Vector());
	StartFireCooldown();
}

void UVremWeaponComponent::ServerFire_Implementation(FVector ViewOrigin, FVector ViewDirection)
{
    PerformHitScan(ViewOrigin, ViewDirection);
    MulticastOnFire();
}

void UVremWeaponComponent::MulticastOnFire_Implementation()
{
    //sound, muzzle flash, effects
}

void UVremWeaponComponent::PerformHitScan(const FVector& ViewOrigin, const FVector& ViewDirection)
{
    FVector ViewTraceEnd = ViewOrigin + ViewDirection * WeaponDefinition->Range;
    const bool bShowDebug = CVarDebugCharacterShooting.GetValueOnGameThread() > 0;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());                // EquipmentActor
    QueryParams.AddIgnoredActor(GetOwner()->GetOwner());    // Character

    FHitResult ViewHit;
    bool bViewHit = GetWorld()->LineTraceSingleByChannel(
        ViewHit,
        ViewOrigin,
        ViewTraceEnd,
        ECC_Pawn,
        QueryParams
    );

    if (bShowDebug)
    {
        DrawDebugLine(GetWorld(), ViewOrigin, ViewTraceEnd, FColor::Blue, false, 1.f, 0, 1.f);
    }

    FVector TargetPoint = bViewHit ? ViewHit.Location : ViewTraceEnd;

    FVector MuzzleLocation = GetMuzzleLocation();
    FVector ShootDirection = (TargetPoint - MuzzleLocation).GetSafeNormal();
    FVector MuzzleTraceEnd = MuzzleLocation + ShootDirection * WeaponDefinition->Range;

    FHitResult MuzzleHit;
    bool bMuzzleHit = GetWorld()->LineTraceSingleByChannel(
        MuzzleHit,
        MuzzleLocation,
        MuzzleTraceEnd,
        ECC_Pawn,
        QueryParams
    );

    if (bMuzzleHit)
    {
        if (bShowDebug)
        {
            DrawDebugPoint(GetWorld(), MuzzleLocation, 10.f, FColor::Black, false, 1.f);
            DrawDebugLine(GetWorld(), MuzzleLocation, MuzzleHit.Location, FColor::Green, false, 1.f, 0, 1.f);
            DrawDebugPoint(GetWorld(), MuzzleHit.Location, 10.f, FColor::Red, false, 1.f);
        }
        
        AActor* HitActor = Cast<AActor>(MuzzleHit.GetActor());
        if (IsValid(HitActor))
        {
            FPointDamageEvent DamageEvent;
            DamageEvent.HitInfo = MuzzleHit;
            DamageEvent.ShotDirection = ShootDirection;
            
            HitActor->TakeDamage(10.f, DamageEvent, GetInstigatorController(), GetOwner());
        }
    }
    else
    {
        if (bShowDebug)
        {
            DrawDebugLine(GetWorld(), MuzzleLocation, MuzzleTraceEnd, FColor::Red, false, 1.f, 0, 1.f);
        }
    }
}

bool UVremWeaponComponent::CanFire() const
{
    return bCanFire && IsValid(WeaponDefinition);
}

void UVremWeaponComponent::StartFireCooldown()
{
    bCanFire = false;

    FTimerDelegate TimerDelegate;
    TimerDelegate.BindUObject(this, &UVremWeaponComponent::OnFireCooldownFinished);

    GetWorld()->GetTimerManager().SetTimer(
        FireCooldownTimer,
        TimerDelegate,
        WeaponDefinition->GetFireInterval(),
        false
    );
}

void UVremWeaponComponent::OnFireCooldownFinished()
{
    bCanFire = true;

    if (bWantsToFire && WeaponDefinition->FireMode == EWeaponFireMode::FullAuto)
    {
        ExecuteFire();
    }
}

AController* UVremWeaponComponent::GetInstigatorController() const
{
    APawn* Pawn = Cast<APawn>(GetOwner()->GetOwner());
    return Pawn ? Pawn->GetController() : nullptr;
}

FVector UVremWeaponComponent::GetMuzzleLocation() const
{
	AActor* WeaponActor = GetOwner();
	if (IsValid(WeaponActor) == false)
	{
		UE_LOG(LogVremWeapon, Warning, TEXT("WeaponComponent::GetMuzzleLocation WeaponActor is Invalid"));
		return FVector::ZeroVector;
	}

    TArray<UMeshComponent*> MeshComponents;
    WeaponActor->GetComponents<UMeshComponent>(MeshComponents);

    for (UMeshComponent* Mesh : MeshComponents)
    {
        if (Mesh->DoesSocketExist(MuzzleSocketName))
        {
            return Mesh->GetSocketLocation(MuzzleSocketName);
        }
    }

    UE_LOG(LogVremWeapon, Warning, TEXT("WeaponComponent::GetMuzzleLocation fallback!"));
	return GetOwner()->GetActorLocation();
}

