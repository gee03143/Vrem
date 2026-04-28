// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VREM_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	virtual void BeginDestroy() override;
	virtual void OnUnregister() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void InitializeComponent() override;

	void Cleanup();

protected:
	UFUNCTION()
	void OnTakePointDamageHandle(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent,
		FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);

	UFUNCTION()
	void OnTakeRadialDamageHandle(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo, class AController* InstigatedBy, AActor* DamageCauser);
	
	UFUNCTION()
	void OnRep_BaseHealth(float PrevHealth);
	
	void BroadcastHealthChanged(float PrevHealth);

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float /*PrevHealth*/, float /*NewHealth*/)
	FOnHealthChanged OnHealthChanged;

	DECLARE_MULTICAST_DELEGATE(FOnHealthZeroed)
	FOnHealthZeroed OnHealthZeroed;
	
protected:
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_BaseHealth)
	float Health = 100.f;

		UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_BaseHealth)
	float MaxHealth = 100.f;
};
