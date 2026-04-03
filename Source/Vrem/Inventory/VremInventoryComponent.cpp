// Fill out your copyright notice in the Description page of Project Settings.


#include "VremInventoryComponent.h"
#include "Vrem/Weapon/VremWeaponDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Vrem/VremLogChannels.h"

// Sets default values for this component's properties
UVremInventoryComponent::UVremInventoryComponent()
{
}

void UVremInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{ 
		InitializeDefaultItems();
	}
}

void UVremInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ 
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UVremInventoryComponent, InventoryItems, COND_OwnerOnly);
}

void UVremInventoryComponent::AddWeaponToInventory(const FPrimaryAssetId& WeaponToAdd)
{
    check(IsValid(GetOwner()));
    check(GetOwner()->HasAuthority());

	InventoryItems.AddEntry(WeaponToAdd);
    OnInventoryChanged.Broadcast();

	UE_LOG(LogVremInventory, Warning, TEXT("%s\nNetMode : %s"), *InventoryItems.ToString(), *GetNetModeString(GetWorld()));
}

void UVremInventoryComponent::RemoveWeaponFromInventory(const FPrimaryAssetId& WeaponToRemove)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	InventoryItems.RemoveEntry(WeaponToRemove);
	OnInventoryChanged.Broadcast();

	UE_LOG(LogVremInventory, Warning, TEXT("%s\nNetMode : %s"), *InventoryItems.ToString(), *GetNetModeString(GetWorld()));
}

void UVremInventoryComponent::InitializeDefaultItems()
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	for (const UVremWeaponDefinition* Def : DefaultWeaponDefinitions)
	{
		FPrimaryAssetId Id = Def->GetPrimaryAssetId();
		AddWeaponToInventory(Id);
	}
}

void UVremInventoryComponent::OnRep_InventoryItems()
{
	OnInventoryChanged.Broadcast();

	UE_LOG(LogVremInventory, Warning, TEXT("%s\nNetMode : %s"), *InventoryItems.ToString(), *GetNetModeString(GetWorld()));
}