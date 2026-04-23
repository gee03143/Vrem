// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentDefinition.h"
#include "VremEquipmentActor.h"
#include "Vrem/VremLogChannels.h"
#include "Kismet/GameplayStatics.h"

// =======================================
// UVremEquipmentInstance
// =======================================

void UVremEquipmentInstance::Initialize(const UVremEquipmentDefinition* InEquipmentDefinition, AActor* InParentActor)
{
	EquipmentDefinition = InEquipmentDefinition;
	ParentActor = InParentActor;

	if (InEquipmentDefinition == nullptr)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::OnItemCreated InItemDefinition is nullptr\nNetMode : %s"), *GetNetModeString(GetWorld()));
		return;
	}
}

void UVremEquipmentInstance::Cleanup()
{
	// EquipmentActor는 서버 복제이므로, 서버에서만 제거
	if (IsValid(EquipmentActor) && ParentActor.IsValid() && ParentActor->HasAuthority())
	{
		EquipmentActor->Destroy();
	}

	if (EquipmentDefinition.IsValid())
	{
		OnInstanceDestroyed.Broadcast(EquipmentDefinition->AnimLayerClass);
	}

	EquipmentActor = nullptr;
	EquipmentDefinition = nullptr;
	ParentActor = nullptr;
}

void UVremEquipmentInstance::BeginDestroy()
{
	Cleanup();

	Super::BeginDestroy();
}

void UVremEquipmentInstance::SetEquipmentState(EEquipmentState InEquipmentState)
{
	if (ParentActor.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::SetEquipmentState: ParentActor is invalid"));
		return;
	}

	EquipmentState = InEquipmentState;

	ApplyEquipmentState();
}

void UVremEquipmentInstance::BindEquipmentActor(AVremEquipmentActor* InActor)
{
	// client only
	check(ParentActor->HasAuthority() == false);

	if (IsValid(InActor))
	{
		EquipmentActor = InActor;
	}
	else
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::BindEquipmentActor: InActor is null  NetMode : %s"), *GetNetModeString(GetWorld()));
	}
}

void UVremEquipmentInstance::SpawnEquipmentActor()
{
	// server only
	check(ParentActor->HasAuthority());

	if (ParentActor.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::SpawnEquipmentActor: ParentActor is invalid"));
		return;
	}

	if (EquipmentDefinition->EquipmentActorClass.IsNull())
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::SpawnEquipmentActor: EquipmentDefinition EquipmentActorClass is not set"));
		return;
	}

	if (EquipmentDefinition.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::SpawnEquipmentActor: EquipmentDefinition is invalid"));
		return;
	}

	if (IsValid(EquipmentActor))
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::SpawnEquipmentActor: EquipmentActor already exist"));
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

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = ParentActor.Get();

	// spawn
	AVremEquipmentActor* SpawnedActor = World->SpawnActor<AVremEquipmentActor>(ActorClass, FTransform::Identity, SpawnParams);
	if (IsValid(SpawnedActor) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: SpawnedActor is invalid"));
		return;
	}

	EquipmentActor = SpawnedActor;
}

void UVremEquipmentInstance::AttachToSocket(const FName& SocketName, const FTransform& Offset)
{
	if (ParentActor.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: ParentActor is invalid"));
		return;
	}

	if (IsValid(EquipmentActor) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: EquipmentActor is invalid"));
		return;
	}

	if (EquipmentActor->GetAttachParentSocketName() == SocketName &&
		EquipmentActor->GetRootComponent()->GetRelativeTransform().Equals(Offset))
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::AttachToSocket: Already attached to same socket nothing happened"));
		return;
	}

	// Attach
	USkeletalMeshComponent* Mesh = ParentActor->FindComponentByClass<USkeletalMeshComponent>();
	if (IsValid(Mesh))
	{
		EquipmentActor->AttachToComponent(
			Mesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SocketName
		);
	}

	// Offset 적용
	EquipmentActor->SetActorRelativeTransform(Offset);
}


void UVremEquipmentInstance::ApplyEquipmentState()
{
	if (EquipmentDefinition.IsValid() == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("UVremEquipmentInstance::ApplyEquipmentState EquipmentDefinition not set NetMode : %s"), *GetNetModeString(GetWorld()));
		return;
	}

	switch (EquipmentState)
	{
	case EEquipmentState::OnHand:
		AttachToSocket(EquipmentDefinition->AttachSocketName, EquipmentDefinition->AttachOffset);
		OnStateChanged.Broadcast(EquipmentState, EquipmentDefinition->AnimLayerClass);
		break;

	case EEquipmentState::Holstered:
		AttachToSocket(EquipmentDefinition->HolsterSocketName, EquipmentDefinition->HolsterOffset);
		OnStateChanged.Broadcast(EquipmentState, EquipmentDefinition->AnimLayerClass);
		break;
	case EEquipmentState::Stowed:
		AttachToSocket(EquipmentDefinition->StowedSocketName, EquipmentDefinition->StowedOffset);
		OnStateChanged.Broadcast(EquipmentState, EquipmentDefinition->AnimLayerClass);
		break;
	default:
		checkNoEntry();
	}
}