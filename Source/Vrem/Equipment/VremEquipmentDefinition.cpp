// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentDefinition.h"
#include "VremEquipmentActor.h"
#include "Vrem/VremLogChannels.h"
#include "Kismet/GameplayStatics.h"

// =======================================
// UVremEquipmentInstance
// =======================================

void UVremEquipmentInstance::OnItemCreated(UVremEquipmentDefinition* InEquipmentDefinition)
{
	EquipmentDefinition = InEquipmentDefinition;

	if (InEquipmentDefinition == nullptr)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::OnItemCreated InItemDefinition is nullptr\nNetMode : %s"), *GetNetModeString(GetWorld()));
		return;
	}
}

void UVremEquipmentInstance::OnItemRemoved()
{

}

void UVremEquipmentInstance::RequestAttach(AActor* ParentActor)
{
	UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::RequestAttach"));

	if (IsValid(ParentActor) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UItemFragment_Equipment::RequestAttach: ParentActor is invalid"));
		return;
	}

	if (IsValid(EquipmentDefinition) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UItemFragment_Equipment::RequestAttach: EquipmentDefinition is invalid"));
		return;
	}

	// 이미 생성된 경우 → 재사용
	if (EquipmentActor)
	{
		EquipmentActor->AttachToActor(ParentActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale, EquipmentDefinition->AttachSocketName);
		return;
	}

	if (EquipmentDefinition->EquipmentActorClass.IsNull())
	{
		// 액터 클래스가 세팅되지 않은 경우 스킵
		return;
	}

	UWorld* World = ParentActor->GetWorld();
	if (IsValid(World) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UItemFragment_Equipment::RequestAttach: World is invalid"));
		return;
	}

	if (ParentActor->HasAuthority() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UItemFragment_Equipment::RequestAttach: Trying to Spawn EquipmentActor but ParentActor not have authority"));
		return;
	}

	// SoftClass → Load
	UClass* ActorClass = EquipmentDefinition->EquipmentActorClass.LoadSynchronous();
	if (IsValid(ActorClass) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("Failed to load EquipmentActorClass"));
		return;
	}


	// spawn
	AVremEquipmentActor* SpawnedActor = World->SpawnActorDeferred<AVremEquipmentActor>(ActorClass, FTransform::Identity, ParentActor, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(SpawnedActor) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UItemFragment_Equipment::RequestAttach: SpawnedActor is invalid"));
		return;
	}

	SpawnedActor->AttachSocketName = EquipmentDefinition->AttachSocketName;
	SpawnedActor->AttachOffset = EquipmentDefinition->AttachOffset;

	// Attach
	USkeletalMeshComponent* Mesh = ParentActor->FindComponentByClass<USkeletalMeshComponent>();
	if (IsValid(Mesh))
	{
		SpawnedActor->AttachToComponent(
			Mesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			EquipmentDefinition->AttachSocketName
		);
	}

	// Offset 적용
	SpawnedActor->SetActorRelativeTransform(EquipmentDefinition->AttachOffset);

	UGameplayStatics::FinishSpawningActor(SpawnedActor, FTransform::Identity);
	EquipmentActor = SpawnedActor;
}

void UVremEquipmentInstance::RequestRemoveActor()
{
	if (IsValid(EquipmentActor))
	{
		EquipmentActor->Destroy();
		EquipmentActor = nullptr;
	}
}
