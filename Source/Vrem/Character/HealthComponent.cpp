// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	SetIsReplicatedByDefault(true);

}

void UHealthComponent::BeginDestroy()
{
	Cleanup();
	
	Super::BeginDestroy();
}

void UHealthComponent::OnUnregister()
{
	Cleanup();
	
	Super::OnUnregister();
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
}

void UHealthComponent::InitializeFromOwner()
{
	if (IsValid(GetOwner()))
	{
		GetOwner()->OnTakeAnyDamage.AddDynamic(this, &ThisClass::OnTakeAnyDamageHandle);
		GetOwner()->OnTakePointDamage.AddDynamic(this, &ThisClass::OnTakePointDamageHandle);
		GetOwner()->OnTakeRadialDamage.AddDynamic(this, &ThisClass::OnTakeRadialDamageHandle);
	}
}

void UHealthComponent::Cleanup()
{
	if (IsValid(GetOwner()))
	{
		GetOwner()->OnTakeAnyDamage.RemoveAll(this);
		GetOwner()->OnTakePointDamage.RemoveAll(this);
		GetOwner()->OnTakeRadialDamage.RemoveAll(this);
	}
}

void UHealthComponent::OnTakeAnyDamageHandle(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
                                             class AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogVrem, Warning, TEXT("UHealthComponent::OnTakeAnyDamageHandle"));
	Health -= Damage;
}

void UHealthComponent::OnTakePointDamageHandle(AActor* DamagedActor, float Damage, class AController* InstigatedBy,
	FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection,
	const class UDamageType* DamageType, AActor* DamageCauser)
{
	UE_LOG(LogVrem, Warning, TEXT("UHealthComponent::OnTakePointDamageHandle"));
	Health -= Damage;
}

void UHealthComponent::OnTakeRadialDamageHandle(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	FVector Origin, const FHitResult& HitInfo, class AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogVrem, Warning, TEXT("UHealthComponent::OnTakeRadialDamageHandle"));
	Health -= Damage;
}

void UHealthComponent::OnRep_BaseHealth(float PrevHealth)
{
	OnHealthChanged.Broadcast(PrevHealth, Health);

	if (Health <= 0)
	{
		OnHealthZeroed.Broadcast();
	}
}

