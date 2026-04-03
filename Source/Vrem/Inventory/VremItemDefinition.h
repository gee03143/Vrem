// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VremItemDefinition.generated.h"

class UVremItemInstance;

UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class UItemFragment : public UObject
{
    GENERATED_BODY()

public:
    virtual void OnItemCreated(UVremItemInstance* Instance) {}
    virtual void OnItemRemoved(UVremItemInstance* Instance) {}
};

UCLASS()
class UItemFragment_Equipment : public UItemFragment
{
    GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<AActor> WeaponActorClass;

	UPROPERTY(EditDefaultsOnly)
    TSoftClassPtr<UAnimInstance> AnimLayerClass;
};

UCLASS()
class UItemFragment_Dummy : public UItemFragment
{
    GENERATED_BODY()

public:
    virtual void OnItemCreated(UVremItemInstance* Instance) override;

protected:
	UPROPERTY(EditDefaultsOnly)
	int32 Param1;

	UPROPERTY(EditDefaultsOnly)
    int32 Param2;
};

/**
 * 
 */
UCLASS()
class VREM_API UVremItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
		
public:
    UPROPERTY(EditDefaultsOnly, Instanced)
    TArray<UItemFragment*> Fragments;
};

UCLASS()
class VREM_API UVremItemInstance : public UObject
{
	GENERATED_BODY()
	
public:
    void OnItemCreated(UVremItemDefinition* InItemDefinition);
    void OnItemRemoved();

    template<typename T>
    T* FindFragment() const;

protected:
    UPROPERTY()
    UVremItemDefinition* ItemDef;

    UPROPERTY()
    TArray<UItemFragment*> InstanceFragments;
};

template<typename T>
T* UVremItemInstance::FindFragment() const
{
	for (UItemFragment* Fragment : InstanceFragments)
	{
		if (T* Typed = Cast<T>(Fragment))
		{
			return Typed;
		}
	}
	return nullptr;
}
