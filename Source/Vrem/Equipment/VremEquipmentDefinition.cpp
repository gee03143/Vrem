// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentDefinition.h"
#include "VremEquipmentActor.h"
#include "Vrem/VremLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "VremEquipmentComponent.h"

// =======================================
// UVremEquipmentInstance
// =======================================

void UVremEquipmentInstance::Initialize(const UVremEquipmentDefinition* InEquipmentDefinition, AActor* InParentActor)
{
	UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::Initialize  NetMode : %s"), *GetNetModeString(GetWorld()));
	EquipmentDefinition = InEquipmentDefinition;
	ParentActor = InParentActor;

	if (InEquipmentDefinition == nullptr)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::OnItemCreated InItemDefinition is nullptr\nNetMode : %s"), *GetNetModeString(GetWorld()));
		return;
	}

	ApplyEquipmentState();
}

void UVremEquipmentInstance::Cleanup()
{
	EquipmentDefinition = nullptr;
	ParentActor = nullptr;
	RemoveEquipmentActor();
}

void UVremEquipmentInstance::BeginDestroy()
{
	Super::BeginDestroy();

	Cleanup();
}

void UVremEquipmentInstance::SetEquipmentState(EEquipmentState InEquipmentState)
{
	if (ParentActor.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::SetEquipmentState: ParentActor is invalid"));
		return;
	}

	if (EquipmentState == InEquipmentState)
	{
		return;
	}

	EquipmentState = InEquipmentState;

	ApplyEquipmentState();

}

void UVremEquipmentInstance::ApplyEquipmentState()
{
	if (EquipmentDefinition.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::ApplyEquipmentState EquipmentDefinition not set NetMode : %s"), *GetNetModeString(GetWorld()));
		return;
	}

	UVremEquipmentComponent* EquipmentComponent = ParentActor->GetComponentByClass<UVremEquipmentComponent>();
	switch (EquipmentState)
	{
	case EEquipmentState::Equipped:
		AttachToSocket(EquipmentDefinition->AttachSocketName, EquipmentDefinition->AttachOffset);
		if (IsValid(EquipmentComponent))
		{
			EquipmentComponent->OnEquipmenntAttached.Broadcast(EquipmentDefinition->AnimLayerClass);
		}
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::ApplyEquipmentState EquipmentState : Equipped  NetMode : %s"), *GetNetModeString(GetWorld()));
		break;

	case EEquipmentState::Holstered:
		AttachToSocket(EquipmentDefinition->HolsterSocketName, EquipmentDefinition->HolsterOffset);
		if (IsValid(EquipmentComponent))
		{
			EquipmentComponent->OnEquipmenntDetached.Broadcast(EquipmentDefinition->AnimLayerClass);
		}
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::ApplyEquipmentState EquipmentState : Holstered  NetMode : %s"), *GetNetModeString(GetWorld()));
		break;

	case EEquipmentState::Unequipped:
		RemoveEquipmentActor();
		if (IsValid(EquipmentComponent))
		{
			EquipmentComponent->OnEquipmenntDetached.Broadcast(EquipmentDefinition->AnimLayerClass);
		}
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::ApplyEquipmentState EquipmentState : Unequipped  NetMode : %s"), *GetNetModeString(GetWorld()));
		break;
	}
}

void UVremEquipmentInstance::RemoveEquipmentActor()
{
	if (IsValid(EquipmentActor))
	{
		EquipmentActor->Destroy();
		EquipmentActor = nullptr;
	}
}

void UVremEquipmentInstance::AttachToSocket(const FName& SocketName, const FTransform& Offset)
{
	if (ParentActor.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: ParentActor is invalid"));
		return;
	}

	if (ParentActor->HasAuthority() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: Trying to Spawn EquipmentActor but ParentActor not have authority"));
		return;
	}

	if (EquipmentDefinition.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: EquipmentDefinition is invalid"));
		return;
	}

	// 이미 생성된 경우 → 재사용
	if (IsValid(EquipmentActor))
	{
		EquipmentActor->AttachToActor(ParentActor.Get(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		return;
	}

	if (EquipmentDefinition->EquipmentActorClass.IsNull())
	{
		// 액터 클래스가 세팅되지 않은 경우 스킵
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: EquipmentDefinition EquipmentActorClass is not set"));
		return;
	}

	UWorld* World = ParentActor->GetWorld();
	if (IsValid(World) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: World is invalid"));
		return;
	}

	// SoftClass → Load
	UClass* ActorClass = EquipmentDefinition->EquipmentActorClass.LoadSynchronous();
	if (IsValid(ActorClass) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: Failed to load EquipmentActorClass"));
		return;
	}

	// spawn
	AVremEquipmentActor* SpawnedActor = World->SpawnActor<AVremEquipmentActor>(ActorClass, FTransform::Identity);
	if (IsValid(SpawnedActor) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: SpawnedActor is invalid"));
		return;
	}

	// Attach
	USkeletalMeshComponent* Mesh = ParentActor->FindComponentByClass<USkeletalMeshComponent>();
	if (IsValid(Mesh))
	{
		SpawnedActor->AttachToComponent(
			Mesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SocketName
		);
	}

	// Offset 적용
	SpawnedActor->SetActorRelativeTransform(Offset);

	EquipmentActor = SpawnedActor;
}
