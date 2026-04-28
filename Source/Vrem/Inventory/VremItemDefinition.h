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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vrem|Item", meta=(DeterminesOutputType="FragmentClass"))
	UItemFragment* FindFragmentByClass(TSubclassOf<UItemFragment> FragmentClass) const;

	template<typename T>
	T* FindFragment() const;
};

template<typename T>
T* UVremItemDefinition::FindFragment() const
{
	for (UItemFragment* Fragment : Fragments)
	{
		if (T* Typed = Cast<T>(Fragment))
		{
			return Typed;
		}
	}
	return nullptr;
}

UCLASS(BlueprintType)
class VREM_API UVremItemInstance : public UObject
{
	GENERATED_BODY()
	
public:
    void OnItemCreated(UVremItemDefinition* InItemDefinition);
    void OnItemRemoved();

    template<typename T>
    T* FindFragment() const;

	const UVremItemDefinition* GetItemDefinition() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vrem|Item", meta=(DeterminesOutputType="FragmentClass"))
	UItemFragment* FindFragmentByClass(TSubclassOf<UItemFragment> FragmentClass) const;
protected:
    UPROPERTY()
    UVremItemDefinition* ItemDef;
};

template<typename T>
T* UVremItemInstance::FindFragment() const
{
	return IsValid(ItemDef) ? ItemDef->FindFragment<T>() : nullptr;
}
