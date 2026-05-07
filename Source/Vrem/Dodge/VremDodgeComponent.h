// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VremDodgeProfile.h"
#include "VremDodgeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDodgeStarted, EDodgeDirection, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeFinished);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UVremDodgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVremDodgeComponent();

public:
	UFUNCTION(BlueprintCallable, Category = "Vrem|Dodge")
	void RequestDodge(EDodgeDirection Direction);

	UFUNCTION(BlueprintPure, Category = "Vrem|Dodge")
    bool CanDodge() const;

    UFUNCTION(BlueprintPure, Category = "Vrem|Dodge")
    bool IsDodging() const { return bIsDodging; }

	UPROPERTY(BlueprintAssignable, Category = "Vrem|Dodge")
	FOnDodgeStarted OnDodgeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Vrem|Dodge")
	FOnDodgeFinished OnDodgeFinished;

protected:
	void ExecuteDodge(EDodgeDirection Direction);

	UFUNCTION(Server, Reliable)
	void ServerDodge(EDodgeDirection Direction);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastOnDodge(EDodgeDirection Direction);

	void PlayMontageLocally(const FDodgeSequence& Sequence);
	void ApplyDodgeForce(const FDodgeSequence& Sequence, EDodgeDirection Direction);
	FVector GetWorldDirection(EDodgeDirection Direction) const;

private:
	void OnDodgeDurationFinished();
	void OnCooldownFinished();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Vrem|Dodge")
	TObjectPtr<UVremDodgeProfile> DodgeProfile;

private:
	bool bIsDodging = false;
	bool bIsOnCooldown = false;
	float LastDodgeCooldown = 0.f;

	FTimerHandle DodgeDurationTimer;
	FTimerHandle CooldownTimer;
};
