// Fill out your copyright notice in the Description page of Project Settings.


#include "VremWeaponComponent.h"
#include "Engine/DamageEvents.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/VremGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameplayTagAssetInterface.h"
#include "VremWeaponHandlerInterface.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/Inventory/VremInventoryComponent.h"

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
    PrimaryComponentTick.bCanEverTick = true;
}

void UVremWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (IsValid(Owner) == false || Owner->HasAuthority() == false || IsValid(WeaponDefinition) == false)
    {
        return;
    }

    int32 LoadedAmount = 0;
    UVremInventoryComponent* Inventory = GetCharacterInventory();
    if (IsValid(Inventory))
    {
        LoadedAmount = Inventory->RemoveAmmo(WeaponDefinition->RequiredAmmoType, WeaponDefinition->MagazineSize);
    }

    CurrentMagazineAmmo = LoadedAmount;
    OnMagazineChanged.Broadcast(CurrentMagazineAmmo, WeaponDefinition->MagazineSize);
}

void UVremWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (EndPlayReason == EEndPlayReason::Destroyed)
    {
        AActor* Owner = GetOwner();
        if (IsValid(Owner) && Owner->HasAuthority() && IsValid(WeaponDefinition) && CurrentMagazineAmmo > 0)
        {
            // TODO: GameplayTag(RequiredAmmoType) -> AmmoItemDefinition Mapping DataTable will be implemented.
            //       until then we connect itemdefinition manually.
            //       after implementation, remove this field and replace with runtime DataTable lookup.
            UVremInventoryComponent* Inventory = GetCharacterInventory();
            if (IsValid(Inventory) && IsValid(WeaponDefinition->AmmoItemDefinition))
            {
                Inventory->AddItemToInventory(WeaponDefinition->AmmoItemDefinition, CurrentMagazineAmmo);
                CurrentMagazineAmmo = 0;
            }
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UVremWeaponComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (IsValid(WeaponDefinition) == false)
    {
        return;
    }

    if (CurrentBloom > 0.f)
    {
        CurrentBloom = FMath::Max(0.f, CurrentBloom - WeaponDefinition->SpreadProfile.BloomRecoverSpeed * DeltaTime);
    }
}

void UVremWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UVremWeaponComponent, CurrentMagazineAmmo, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UVremWeaponComponent, bIsReloading, COND_OwnerOnly);
}

void UVremWeaponComponent::RequestFire()
{
    Fire();
}

void UVremWeaponComponent::RequestStopFire()
{
    StopFire();
}

void UVremWeaponComponent::RequestReload()
{
    if (CanReload() == false)
    {
        return;
    }

    APawn* Pawn = Cast<APawn>(GetWeaponOwner());
    if (IsValid(Pawn) && Pawn->IsLocallyControlled() && IsValid(WeaponDefinition->ReloadMontage))
    {
        PlayMontageLocally(WeaponDefinition->ReloadMontage);
    }

    AActor* Owner = GetWeaponOwner();
    if (IsValid(Owner) && Owner->HasAuthority())
    {
        ExecuteReload();
    }
    else
    {
        bIsReloading = true; // client prediction (will be corrected when OnRep)
        ServerStartReload();
    }
}

void UVremWeaponComponent::RequestCancelReload()
{
    if (bIsReloading == false)
    {
        return;
    }

    // 자기 클라 즉시 몽타주 중단
    APawn* Pawn = Cast<APawn>(GetWeaponOwner());
    if (IsValid(Pawn) && Pawn->IsLocallyControlled())
    {
        CancelMontageLocally();
    }

    AActor* Owner = GetWeaponOwner();
    if (IsValid(Owner) && Owner->HasAuthority())
    {
        CancelReloadLocal();
    }
    else
    {
        bIsReloading = false; // client prediction (will be corrected when OnRep)
        ServerCancelReload();
    }
}

int32 UVremWeaponComponent::GetMagazineSize() const
{
    return IsValid(WeaponDefinition) ? WeaponDefinition->MagazineSize : 0;
}

bool UVremWeaponComponent::CanReload() const
{
    if (bIsReloading)
    {
        return false;
    }
    if (IsValid(WeaponDefinition) == false)
    {
        return false;
    }
    if (CurrentMagazineAmmo >= WeaponDefinition->MagazineSize)
    {
        return false;   // 가득
    }

    UVremInventoryComponent* Inv = GetCharacterInventory();
    if (IsValid(Inv) == false)
    {
        return false;
    }
    if (Inv->GetAmmoCount(WeaponDefinition->RequiredAmmoType) <= 0)
    {
        return false;   // 예비탄 0
    }
    return true;
}

void UVremWeaponComponent::Fire()
{
    if (IsValid(WeaponDefinition) == false)
    {
        return;
    }

    bWantsToFire = true;

    if (bIsReloading && CurrentMagazineAmmo > 0)
    {
        RequestCancelReload();
    }

    if (CanFire())
    {
        ExecuteFire();
    }
    else if (bCanFire && CurrentMagazineAmmo <= 0)
    {
        TryPlayDryFire();
        StartFireCooldown();
    }
    return;
}

void UVremWeaponComponent::StopFire()
{
    bWantsToFire = false;
}

void UVremWeaponComponent::ExecuteFire()
{
	AController* Controller = GetInstigatorController();
	if (IsValid(Controller) == false)
	{
        UE_LOG(LogVremWeapon, Warning, TEXT("UVremWeaponComponent::ExecuteFire Controller is nullptr"));
		return;
	}

	FVector ViewOrigin;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewOrigin, ViewRotation);

    // 스프레드 적용
    const float SpreadDegrees = GetCurrentSpread();
    const FVector ShootDirection = FMath::VRandCone(
        ViewRotation.Vector(),
        FMath::DegreesToRadians(SpreadDegrees)
    );

    APawn* WeaponOwner = Cast<APawn>(GetWeaponOwner());
    if (IsValid(WeaponOwner))
    {
        if (WeaponOwner->IsLocallyControlled() && IsValid(WeaponDefinition))
        {
            PlayMontageLocally(WeaponDefinition->FireMontage);
        }

        if (WeaponOwner->GetLocalRole() == ROLE_AutonomousProxy)
        {
            CurrentMagazineAmmo = FMath::Max(0, CurrentMagazineAmmo - 1);
            OnMagazineChanged.Broadcast(CurrentMagazineAmmo, GetMagazineSize());
        }
    }

	ServerFire(ViewOrigin, ShootDirection);
	StartFireCooldown();

    AccumulateBloom();

    if (IsValid(WeaponDefinition))
    {
        if (IVremWeaponHandler* Handler = Cast<IVremWeaponHandler>(GetWeaponOwner()))
        {
            Handler->OnWeaponFired(WeaponDefinition->RecoilProfile);
        }
    }
}

void UVremWeaponComponent::ServerFire_Implementation(FVector ViewOrigin, FVector ViewDirection)
{
    CurrentMagazineAmmo = FMath::Max(0, CurrentMagazineAmmo - 1);
    OnMagazineChanged.Broadcast(CurrentMagazineAmmo, GetMagazineSize());

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

    // 5. 몽타주
    APawn* WeaponOwner = Cast<APawn>(GetWeaponOwner());
    if (IsValid(WeaponOwner) && WeaponOwner->IsLocallyControlled() == false)
    {
        if (IsValid(WeaponDefinition))
        {
            PlayMontageLocally(WeaponDefinition->FireMontage);
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
    return bIsReloading == false && bCanFire && IsValid(WeaponDefinition) && CurrentMagazineAmmo > 0;
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
        if (CanFire())
        {
            ExecuteFire();
        }
        else
        {
            TryPlayDryFire();
            bWantsToFire = false;
        }
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

float UVremWeaponComponent::GetCurrentSpread() const
{
    if (WeaponDefinition == nullptr)
    {
        return 0.f;
    }

    AActor* Owner = GetWeaponOwner();
    IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Owner);
    if (TagInterface == nullptr)
    {
        return 0.f;
    }

    FGameplayTagContainer OwnerTags;
    TagInterface->GetOwnedGameplayTags(OwnerTags);

    if (OwnerTags.HasTag(FVremGameplayTags::State_Aiming_Scoped))
    {
        return 0.f;
    }

    const FSpreadProfile& Profile = WeaponDefinition->SpreadProfile;
    float Spread = Profile.BaseSpread + CurrentBloom;
    if (OwnerTags.HasTag(FVremGameplayTags::State_Movement_Moving))
    {
        Spread *= Profile.MovingSpreadMultiplier;
    }

    if (OwnerTags.HasTag(FVremGameplayTags::State_Movement_InAir))
    {
        Spread *= Profile.InAirSpreadMultiplier;
    }

    return Spread;
}

void UVremWeaponComponent::AccumulateBloom()
{
    if (IsValid(WeaponDefinition) == false)
    {
        return;
    }

    const FSpreadProfile& Profile = WeaponDefinition->SpreadProfile;
    CurrentBloom = FMath::Min(Profile.MaxBloom, CurrentBloom + Profile.BloomPerShot);
}

void UVremWeaponComponent::ExecuteReload()
{
    check(IsValid(GetOwner()));
    check(GetOwner()->HasAuthority());

    if (CanReload() == false)
    {
        return;
    }

    bIsReloading = true;

    GetWorld()->GetTimerManager().SetTimer(
        ReloadTimer, this, &UVremWeaponComponent::OnReloadTimerFinished,
        WeaponDefinition->ReloadTime, false);

    MulticastPlayReloadMontage();
}

void UVremWeaponComponent::OnReloadTimerFinished()
{
    check(IsValid(GetOwner()));
    check(GetOwner()->HasAuthority());

    if (bIsReloading == false)
    {
        return;
    }

    UVremInventoryComponent* InventoryComponent = GetCharacterInventory();
    if (IsValid(InventoryComponent) && IsValid(WeaponDefinition))
    {
        const int32 AmountNeeded = WeaponDefinition->MagazineSize - CurrentMagazineAmmo;
        const int32 Consumed = InventoryComponent->RemoveAmmo(WeaponDefinition->RequiredAmmoType, AmountNeeded);
        CurrentMagazineAmmo += Consumed;
        OnMagazineChanged.Broadcast(CurrentMagazineAmmo, GetMagazineSize());
    }

    bIsReloading = false;
}

void UVremWeaponComponent::CancelReloadLocal()
{
    check(IsValid(GetOwner()));
    check(GetOwner()->HasAuthority());

    if (bIsReloading == false)
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
    bIsReloading = false;

    MulticastCancelReloadMontage();
}

void UVremWeaponComponent::ServerStartReload_Implementation()
{
    ExecuteReload();
}

void UVremWeaponComponent::ServerCancelReload_Implementation()
{
    CancelReloadLocal();
}

void UVremWeaponComponent::MulticastPlayReloadMontage_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }
    if (IsValid(WeaponDefinition) == false || IsValid(WeaponDefinition->ReloadMontage) == false)
    {
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetWeaponOwner());
    if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
    {
        return;   // 자기 클라는 RequestReload 에서 이미 재생
    }

    PlayMontageLocally(WeaponDefinition->ReloadMontage);
}

void UVremWeaponComponent::MulticastCancelReloadMontage_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetWeaponOwner());
    if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
    {
        return;   // 자기 클라는 RequestCancelReload 에서 이미 중단
    }

    CancelMontageLocally();
}

void UVremWeaponComponent::OnRep_IsReloading()
{
    if (bIsReloading)
    {
        OnReloadStarted.Broadcast();
    }
    else
    {
        OnReloadFinished.Broadcast();
    }
}

UVremInventoryComponent* UVremWeaponComponent::GetCharacterInventory() const
{
    ACharacter* Character = Cast<ACharacter>(GetWeaponOwner());
    return IsValid(Character) ? Character->FindComponentByClass<UVremInventoryComponent>() : nullptr;
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

void UVremWeaponComponent::PlayMontageLocally(UAnimMontage* MontageToPlay)
{
    if (IsValid(WeaponDefinition) == false)
    {
        UE_LOG(LogVremWeapon, Warning, TEXT("PlayMontageLocally: WeaponDefinition is invalid"));
        return;
    }


    ACharacter* WeaponOwner = Cast<ACharacter>(GetWeaponOwner());
    if (IsValid(WeaponOwner) == false)
    {
        UE_LOG(LogVremWeapon, Warning, TEXT("PlayMontageLocally: WeaponOwner is Invalid or not character"));
        return;
    }

    WeaponOwner->PlayAnimMontage(MontageToPlay);
}

void UVremWeaponComponent::CancelMontageLocally()
{
    ACharacter* WeaponOwner = Cast<ACharacter>(GetWeaponOwner());
    if (IsValid(WeaponOwner) == false)
    {
        UE_LOG(LogVremWeapon, Warning, TEXT("PlayMontageLocally: WeaponOwner is Invalid or not character"));
        return;
    }

    WeaponOwner->StopAnimMontage();
}

void UVremWeaponComponent::TryPlayDryFire()
{
    if (IsValid(WeaponDefinition) == false || IsValid(WeaponDefinition->DryFireMontage) == false)
    {
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetWeaponOwner());
    if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
    {
        PlayMontageLocally(WeaponDefinition->DryFireMontage);
    }
}

void UVremWeaponComponent::OnRep_CurrentMagazineAmmo()
{
    OnMagazineChanged.Broadcast(CurrentMagazineAmmo, GetMagazineSize());
}

#if WITH_AUTOMATION_WORKER
void UVremWeaponComponent::SimulateBloomRecover_ForTest(float DeltaTime)
{
    if (WeaponDefinition && CurrentBloom > 0.f)
    {
        CurrentBloom = FMath::Max(0.f, CurrentBloom - WeaponDefinition->SpreadProfile.BloomRecoverSpeed * DeltaTime);
    }
}
#endif