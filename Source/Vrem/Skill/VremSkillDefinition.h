// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremSkillDefinition.generated.h"

class UTexture2D;
class UVremSkillBehavior;

UENUM(BlueprintType)
enum class EVremSkillActivationMode : uint8
{
	Instant,
	Targeting
};

/**
 * 
 */
UCLASS(BlueprintType)
class VREM_API UVremSkillDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vrem|Skill")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vrem|Skill")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0"), Category = "Vrem|Skill")
    float Cooldown = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vrem|Skill")
    TSubclassOf<UVremSkillBehavior> BehaviorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vrem|Skill")
	EVremSkillActivationMode ActivationMode = EVremSkillActivationMode::Instant;

public:
	const UVremSkillBehavior* GetBehaviorCDO() const;
};
