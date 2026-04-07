// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Vrem/VremLogChannels.h"

// Sets default values
AVremEquipmentActor::AVremEquipmentActor()
{
	bReplicates = true;
}

// Called when the game starts or when spawned
void AVremEquipmentActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsValid(GetOwner()) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremEquipmentActor::BeginPlay: Owner is invalid"));
		return;
	}

	// Attach
	USkeletalMeshComponent* Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	if (IsValid(Mesh) && AttachSocketName.IsNone() == false)
	{
		AttachToComponent(
			Mesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName
		);

		SetActorRelativeTransform(AttachOffset);
	}
	else
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremEquipmentActor::BeginPlay: OwnerMesh or AttachSocketName is Invalid, Failed to Attach"));
	}
}

