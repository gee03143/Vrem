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
	// Blueprint API
    UFUNCTION(BlueprintCallable, Category="Vrem|Weapon")
    void RequestFire();

    UFUNCTION(BlueprintCallable, Category="Vrem|Weapon")
    void RequestStopFire();

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

#if WITH_AUTOMATION_WORKER
public:
    // Definition 주입
    void SetWeaponDefinition_ForTest(UVremWeaponDefinition* InDef)
    {
        WeaponDefinition = InDef;
    }

    // 상태 조회
    float GetCurrentBloom_ForTest() const { return CurrentBloom; }
    bool GetCanFire_ForTest() const { return bCanFire; }
    bool GetWantsToFire_ForTest() const { return bWantsToFire; }

    // 상태 조작
    void AccumulateBloom_ForTest() { AccumulateBloom(); }
    void SimulateBloomRecover_ForTest(float DeltaTime);
    void StartFireCooldown_ForTest() { StartFireCooldown(); }
    void OnFireCooldownFinished_ForTest() { OnFireCooldownFinished(); }
    void SetWantsToFire_ForTest(bool bValue) { bWantsToFire = bValue; }
#endif
};
