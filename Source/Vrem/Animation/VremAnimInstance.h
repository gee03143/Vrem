// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "VremAnimInstance.generated.h"

class AVremCharacter;

USTRUCT(BlueprintType)
struct FVremAnimTagBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Vrem|Animation")
    FGameplayTag Tag;

    UPROPERTY(EditAnywhere, Category = "Vrem|Animation")
    FName PropertyToEdit;
};

UCLASS()
class VREM_API UVremAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeBeginPlay() override;
    virtual void NativeUninitializeAnimation() override;

	TSubclassOf<UAnimInstance> GetCurrentLayer() const { return CurrentLayer; }

protected:
    void CacheTagBindingProperties();
    void ApplyTagChange(const FGameplayTag& Tag, bool bIsAdded);
    UFUNCTION()
    void HandleStateTagAdded(const FGameplayTag& Tag);
    UFUNCTION()
    void HandleStateTagRemoved(const FGameplayTag& Tag);

protected:
    UPROPERTY(Transient)
    TSubclassOf<UAnimInstance> CurrentLayer;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Vrem|Animation|TagBindings")
    TArray<FVremAnimTagBinding> TagBindings;

	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	float GroundDistance = -1.0f;

private:
    TWeakObjectPtr<AVremCharacter> CachedVremCharacter;
    TMap<FGameplayTag, FBoolProperty*> ResolvedTagProperties;
};
