// Fill out your copyright notice in the Description page of Project Settings.


#include "VremAnimInstance.h"

void UVremAnimInstance::SetWeaponAnimLayer(const TSubclassOf<UAnimInstance> NewLayer)
{
    if (CurrentLayer == NewLayer)
    {
        return;
    }

    if (CurrentLayer)
    {
        UnlinkAnimClassLayers(CurrentLayer);
    }

    if (NewLayer)
    {
        LinkAnimClassLayers(NewLayer);
    }

    CurrentLayer = NewLayer;
}