// Fill out your copyright notice in the Description page of Project Settings.


#include "VremWeaponComponent.h"
#include "VremWeaponDefinition.h"
#include "Engine/DamageEvents.h"
#include "Vrem/VremLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

static TAutoConsoleVariable<int32> CVarDebugCharacterShooting(
    TEXT("vrem.DebugCharacterShooting"),
    0,
    TEXT("Enable Character Shooting Debug\n")
    TEXT("0: Off\n")
    TEXT("1: On"),
    ECVF_Cheat);

static TAutoConsoleVariable<float> CVarLogicalMuzzleX(
    TEXT("vrem.Weapon.LogicalMuzzleX"), 40.f, TEXT("Logical muzzle X offset"));
static TAutoConsoleVariable<float> CVarLogicalMuzzleY(
    TEXT("vrem.Weapon.LogicalMuzzleY"), 0.f, TEXT("Logical muzzle Y offset"));
static TAutoConsoleVariable<float> CVarLogicalMuzzleZ(
    TEXT("vrem.Weapon.LogicalMuzzleZ"), 40.f, TEXT("Logical muzzle Z offset"));

// Sets default values for this component's properties
UVremWeaponComponent::UVremWeaponComponent()
{
	SetIsReplicatedByDefault(true);
}

void UVremWeaponComponent::Fire()
{
    if (IsValid(WeaponDefinition) == false)
    {
        return;
    }

    bWantsToFire = true;

    if (CanFire())
    {
        ExecuteFire();
    }
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
    const FWeaponFireResult& FireResult = PerformHitScan(ViewOrigin, ViewDirection);
    MulticastOnFire(FireResult);
}

void UVremWeaponComponent::MulticastOnFire_Implementation(const FWeaponFireResult& FireResult)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    const FVector MuzzleLocation = GetMuzzleLocation();  // 애니메이션 반영된 실제 머즐

    // 1. 발사 사운드
    if (IsValid(WeaponDefinition->FireSound))
    {
        UGameplayStatics::PlaySoundAtLocation(this, WeaponDefinition->FireSound, MuzzleLocation);
    }

    // 2. 머즐 플래시
    if (WeaponDefinition->MuzzleFlashEffect)
    {
        UNiagaraComponent* MuzzleFlash = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this, WeaponDefinition->MuzzleFlashEffect, MuzzleLocation, FRotator::ZeroRotator, FVector(1.f), true, false);
        if (MuzzleFlash)
        {
            static const FName MuzzleFlashScaleName = FName(TEXT("Global Scale"));
            MuzzleFlash->SetVariableFloat(MuzzleFlashScaleName, 0.25f);
            MuzzleFlash->Activate();
        }
    }

    // 3. 트레일 (히트 여부와 상관없이 탄도를 따라)
    if (WeaponDefinition->BulletTrailEffect)
    {
        const FVector TrailDirection = (FireResult.HitLocation - MuzzleLocation).GetSafeNormal();
        const FRotator TrailRotation = TrailDirection.Rotation();

        UNiagaraComponent* Trail = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this, WeaponDefinition->BulletTrailEffect, MuzzleLocation, TrailRotation);
        if (Trail)
        {
            static const FName TrailStartVariableName = FName(TEXT("Start"));
            static const FName TrailTargetVariableName = FName(TEXT("Target"));

            Trail->SetVariableVec3(TrailStartVariableName, MuzzleLocation);
            Trail->SetVariableVec3(TrailTargetVariableName, FireResult.HitLocation);
        }
    }

    // 4. 피격 이펙트
    if (FireResult.bHit)
    {
        if (UNiagaraSystem* ImpactFx = WeaponDefinition->ImpactEffects.FindRef(FireResult.SurfaceType))
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this, ImpactFx, FireResult.HitLocation, FireResult.HitNormal.Rotation());
        }
    }
}

FWeaponFireResult UVremWeaponComponent::PerformHitScan(const FVector& ViewOrigin, const FVector& ViewDirection)
{
    FWeaponFireResult Result;

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
        ECC_Visibility,
        QueryParams
    );

    if (bShowDebug)
    {
        DrawDebugLine(GetWorld(), ViewOrigin, ViewTraceEnd, FColor::Blue, false, 1.f, 0, 1.f);
    }

    FVector TargetPoint = bViewHit ? ViewHit.Location : ViewTraceEnd;

    FVector MuzzleLocation = GetLogicalMuzzleLocation();
    FVector ShootDirection = (TargetPoint - MuzzleLocation).GetSafeNormal();
    FVector MuzzleTraceEnd = MuzzleLocation + ShootDirection * WeaponDefinition->Range;

    FHitResult MuzzleHit;
    bool bMuzzleHit = GetWorld()->LineTraceSingleByChannel(
        MuzzleHit,
        MuzzleLocation,
        MuzzleTraceEnd,
        ECC_Visibility,
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

        Result.bHit = true;
        Result.HitLocation = MuzzleHit.Location;
        Result.HitNormal = MuzzleHit.Normal;
        Result.SurfaceType = UGameplayStatics::GetSurfaceType(MuzzleHit);
        
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

        Result.bHit = false;
        Result.HitLocation = MuzzleTraceEnd;
    }

    return Result;
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
    APawn* Pawn = Cast<APawn>(GetWeaponOwner());
    return Pawn ? Pawn->GetController() : nullptr;
}

AActor* UVremWeaponComponent::GetWeaponOwner() const
{
    return GetOwner() ? GetOwner()->GetOwner() : nullptr;
}

FVector UVremWeaponComponent::GetMuzzleLocation() const
{
    check(GetNetMode() != NM_DedicatedServer);

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

FVector UVremWeaponComponent::GetLogicalMuzzleLocation() const
{
    check(GetOwner()->HasAuthority());

    FVector ShootingOffset(
        CVarLogicalMuzzleX.GetValueOnGameThread(),
        CVarLogicalMuzzleY.GetValueOnGameThread(),
        CVarLogicalMuzzleZ.GetValueOnGameThread()
    );

    AActor* WeaponOwner = GetWeaponOwner();

    return IsValid(WeaponOwner) ? WeaponOwner->GetActorTransform().TransformPosition(ShootingOffset) : FVector::ZeroVector;
}

