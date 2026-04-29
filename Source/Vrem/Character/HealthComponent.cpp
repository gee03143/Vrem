// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
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

void UHealthComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (IsValid(GetOwner()))
	{
		GetOwner()->OnTakePointDamage.AddDynamic(this, &ThisClass::OnTakePointDamageHandle);
		GetOwner()->OnTakeRadialDamage.AddDynamic(this, &ThisClass::OnTakeRadialDamageHandle);
	}
}

void UHealthComponent::Cleanup()
{
	if (IsValid(GetOwner()))
	{
		GetOwner()->OnTakePointDamage.RemoveAll(this);
		GetOwner()->OnTakeRadialDamage.RemoveAll(this);
	}
}

void UHealthComponent::OnTakePointDamageHandle(AActor* DamagedActor, float Damage, class AController* InstigatedBy,
	FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection,
	const class UDamageType* DamageType, AActor* DamageCauser)
{
	if (GetOwner()->HasAuthority() == false)
	{
		return;
	}

	// TODO: Implement Taking Point Damage logic
	// ex) headshot
	const float PrevHealth = Health;
	Health = FMath::Max(0.f, Health - Damage);

	BroadcastHealthChanged(PrevHealth);
}

void UHealthComponent::OnTakeRadialDamageHandle(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	FVector Origin, const FHitResult& HitInfo, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (GetOwner()->HasAuthority() == false)
	{
		return;
	}

	// TODO: Implement Taking Radial Damage logic
	// ex) distance falloff
	const float PrevHealth = Health;
	Health = FMath::Max(0.f, Health - Damage);

	BroadcastHealthChanged(PrevHealth);
}

void UHealthComponent::OnRep_BaseHealth(float PrevHealth)
{
	BroadcastHealthChanged(PrevHealth);
}

void UHealthComponent::BroadcastHealthChanged(float PrevHealth)
{
	OnHealthChanged.Broadcast(PrevHealth, Health);

	if (Health <= 0 && PrevHealth > 0.f)
	{
		OnHealthZeroed.Broadcast();
	}
}

