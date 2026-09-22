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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

    // server only, 권위 측 사격 승인 조건.
    // CanFire() 와 달리 bCanFire(로컬 쿨다운 타이머)를 보지 않는다 — 서버는
    // ExecuteFire() 를 거치지 않아 그 타이머를 돌리지 않으므로, 발사 간격은
    // NextAllowedFireTime 으로 직접 검증한다.
    //
    // 판정과 기한 전진은 월드 시각을 인자로 받는다. 월드 시계에서 떼어놓아야
    // 자동화 테스트가 시간을 직접 먹여 결정론적으로 검증할 수 있다.
    bool IsFireAllowedAt(float WorldTime) const;
    void AdvanceFireDeadline(float WorldTime);

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

    // server only, 권위 측이 다음 사격을 허용하는 가장 이른 월드 시각.
    // 승인할 때마다 발사 간격만큼 전진시켜 평균 발사율을 강제한다.
    // 음수는 '아직 승인한 사격이 없음'을 뜻한다.
    float NextAllowedFireTime = -1.f;

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

    // 권위 측 사격 승인 (시간을 직접 먹인다)
    bool IsFireAllowedAt_ForTest(float WorldTime) const { return IsFireAllowedAt(WorldTime); }
    void AdvanceFireDeadline_ForTest(float WorldTime) { AdvanceFireDeadline(WorldTime); }
    float GetNextAllowedFireTime_ForTest() const { return NextAllowedFireTime; }
    void SetMagazineAmmo_ForTest(int32 InAmmo) { CurrentMagazineAmmo = InAmmo; }
    void SetIsReloading_ForTest(bool bValue) { bIsReloading = bValue; }

    // 상태 조작
    void AccumulateBloom_ForTest() { AccumulateBloom(); }
    void SimulateBloomRecover_ForTest(float DeltaTime);
    void StartFireCooldown_ForTest() { StartFireCooldown(); }
    void OnFireCooldownFinished_ForTest() { OnFireCooldownFinished(); }
    void SetWantsToFire_ForTest(bool bValue) { bWantsToFire = bValue; }
#endif
};
