// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VremWeaponComponent.generated.h"

class UVremWeaponDefinition;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVremWeaponComponent();

public:
    void Fire();
    void StopFire();

protected:
    void ExecuteFire();

    UFUNCTION(Server, Reliable)
    void ServerFire(FVector ViewOrigin, FVector ViewDirection);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastOnFire();

    void PerformHitScan(const FVector& ViewOrigin, const FVector& ViewDirection);

    bool CanFire() const;
    void StartFireCooldown();
    void OnFireCooldownFinished();

    AController* GetInstigatorController() const;

protected:
    FVector GetMuzzleLocation() const;

    UPROPERTY(EditDefaultsOnly)
    FName MuzzleSocketName = TEXT("Muzzle");

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UVremWeaponDefinition> WeaponDefinition;
private:
    FTimerHandle FireCooldownTimer;
    bool bCanFire = true;
    bool bWantsToFire = false;
};
