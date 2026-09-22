// Fill out your copyright notice in the Description page of Project Settings.


#include "VremEquipmentActor.h"
#include "VremEquipmentComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Vrem/VremLogChannels.h"

// Sets default values
AVremEquipmentActor::AVremEquipmentActor()
{
	bReplicates = true;
}

void AVremEquipmentActor::BeginPlay()
{
    Super::BeginPlay();

    // 클라이언트에서만 - 복제 도착 시 EquipmentComponent에 알림
    if (HasAuthority() == false)
    {
        AActor* OwnerActor = GetAttachParentActor();
        if (IsValid(OwnerActor))
        {
            if (UVremEquipmentComponent* Comp = OwnerActor->FindComponentByClass<UVremEquipmentComponent>())
            {
                Comp->OnEquipmentActorReplicated(this);
            }
        }
    }
}