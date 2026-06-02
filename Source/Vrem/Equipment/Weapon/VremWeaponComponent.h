// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "VremWeaponDefinition.h"
#include "VremWeaponComponent.generated.h"

class UVremWeaponDefinition;
class UVremInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMagazineChanged, int32, NewAmount, int32, MaxAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadFinished);

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
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Blueprint API
    UFUNCTION(BlueprintCallable, Category="Vrem|Weapon")
    void RequestFire();

    UFUNCTION(BlueprintCallable, Category="Vrem|Weapon")
    void RequestStopFire();

    UFUNCTION(BlueprintCallable, Category = "Vrem|Weapon")
    void RequestReload();

    UFUNCTION(BlueprintCallable, Category = "Vrem|Weapon")
    void RequestCancelReload();

    UFUNCTION(BlueprintPure, Category="Vrem|Weapon")
    int32 GetCurrentMagazineAmmo() const { return CurrentMagazineAmmo; }

    UFUNCTION(BlueprintPure, Category="Vrem|Weapon")
    int32 GetMagazineSize() const;

    UFUNCTION(BlueprintPure, Category = "Vrem|Weapon")
    bool CanReload() const;

    UFUNCTION(BlueprintPure, Category = "Vrem|Weapon")
    bool IsReloading() const { return bIsReloading; }

    UPROPERTY(BlueprintAssignable, Category="Vrem|Weapon")
    FOnMagazineChanged OnMagazineChanged;

    UPROPERTY(BlueprintAssignable, Category = "Vrem|Weapon")
    FOnReloadStarted OnReloadStarted;

    UPROPERTY(BlueprintAssignable, Category = "Vrem|Weapon")
    FOnReloadFinished OnReloadFinished;

    void Fire();
    void StopFire();

    float GetCurrentSpread() const;

    // Fire
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

private:
    FTimerHandle FireCooldownTimer;
    bool bCanFire = true;
    bool bWantsToFire = false;

    float CurrentBloom = 0.f;

    // Reload
protected:
    void ExecuteReload();          // 권위 측 본문
    void OnReloadTimerFinished();  // 완료 콜백
    void CancelReloadLocal();      // 권위 측 캔슬 본문

    UFUNCTION(Server, Reliable)
    void ServerStartReload();

    UFUNCTION(Server, Reliable)
    void ServerCancelReload();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayReloadMontage();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastCancelReloadMontage();

    UFUNCTION()
    void OnRep_IsReloading();

    UVremInventoryComponent* GetCharacterInventory() const;

private:
    UPROPERTY(ReplicatedUsing = OnRep_IsReloading)
    bool bIsReloading = false;

    FTimerHandle ReloadTimer;

protected:
    // client only, 애니메이션이 반영된 총기 머즐 소켓 위치
    FVector GetMuzzleLocation() const;

    // server only, 애니메이션이 반영되지 않은 논리적 사격 판정 시작점
    FVector GetLogicalMuzzleLocation() const;

    void PlayMontageLocally(UAnimMontage* MontageToPlay);
    void CancelMontageLocally();

    void TryPlayDryFire();

    UFUNCTION()
    void OnRep_CurrentMagazineAmmo();
protected:
    UPROPERTY(EditDefaultsOnly)
    FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UVremWeaponDefinition> WeaponDefinition;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentMagazineAmmo);
    int32 CurrentMagazineAmmo;

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
