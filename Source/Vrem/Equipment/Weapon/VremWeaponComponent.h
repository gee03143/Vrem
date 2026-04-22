// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "VremWeaponDefinition.h"
#include "VremWeaponComponent.generated.h"

class UVremWeaponDefinition;

USTRUCT()
struct FWeaponFireResult
{
    GENERATED_BODY()

    UPROPERTY()
    FVector_NetQuantize HitLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector_NetQuantize HitNormal = FVector::ZeroVector;

    UPROPERTY()
    bool bHit = false;

    UPROPERTY()
    TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVremWeaponComponent();

protected:
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

public:
    void Fire();
    void StopFire();

    float GetCurrentSpread() const;
protected:
    void ExecuteFire();

    UFUNCTION(Server, Reliable)
    void ServerFire(FVector ViewOrigin, FVector ViewDirection);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastOnFire(const FWeaponFireResult& FireResult);

    FWeaponFireResult PerformHitScan(const FVector& ViewOrigin, const FVector& ViewDirection);

    bool CanFire() const;
    void StartFireCooldown();
    void OnFireCooldownFinished();

    AController* GetInstigatorController() const;
    AActor* GetWeaponOwner() const;

    void AccumulateBloom();
public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, const FRecoilProfile& /*RecoilProfile*/)
    FOnWeaponFired OnWeaponFired;

protected:
    // client only, 애니메이션이 반영된 총기 머즐 소켓 위치
    FVector GetMuzzleLocation() const;

    // server only, 애니메이션이 반영되지 않은 논리적 사격 판정 시작점
    FVector GetLogicalMuzzleLocation() const;


protected:
    UPROPERTY(EditDefaultsOnly)
    FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UVremWeaponDefinition> WeaponDefinition;
private:
    FTimerHandle FireCooldownTimer;
    bool bCanFire = true;
    bool bWantsToFire = false;

    float CurrentBloom = 0.f;
};
