// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VremSkillDefinition.h"
#include "VremSkillBehavior.generated.h"


USTRUCT(BlueprintType)
struct FVremSkillActivationContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Vrem|Skill")
    TObjectPtr<AActor> Instigator = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Vrem|Skill")
    FVector AimLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Vrem|Skill")
    FVector AimDirection = FVector::ZeroVector;
}; 
/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class VREM_API UVremSkillBehavior : public UObject
{
	GENERATED_BODY()
	
public:
    // CDO 에서 호출되므로 인스턴스 상태에 의존하면 안 됨.
    UFUNCTION(BlueprintNativeEvent, Category = "Vrem|Skill")
    void Activate(const FVremSkillActivationContext& Context, const UVremSkillDefinition* Definition) const;

    virtual void Activate_Implementation(const FVremSkillActivationContext& Context, const UVremSkillDefinition* Definition) const {}
};
