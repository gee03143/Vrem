// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagAssetInterface.h"
#include "VremTestTagPawn.generated.h"

UCLASS()
class ATestTagPawn : public APawn, public IGameplayTagAssetInterface
{
    GENERATED_BODY()

public:
    virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

    FGameplayTagContainer StateTags;
};

