// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "VremInputConfig.generated.h"

class UInputAction;

USTRUCT()
struct FTaggedInputAction
{
    GENERATED_BODY()
    
    UPROPERTY(EditDefaultsOnly)
    const UInputAction* InputAction = nullptr;
 
    UPROPERTY(EditDefaultsOnly)
    FGameplayTag InputTag;
};

UCLASS()
class VREM_API UVremInputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
    
     UPROPERTY(EditDefaultsOnly)
     TArray<FTaggedInputAction> TaggedInputActions;
    
     const UInputAction* FindInputActionByTag(const FGameplayTag& Tag) const;
};
