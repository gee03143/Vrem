// Fill out your copyright notice in the Description page of Project Settings.


#include "VremInputConfig.h"
#include "InputAction.h"
#include "Vrem/VremLogChannels.h"


const UInputAction* UVremInputConfig::FindInputActionByTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid())
	{
		UE_LOG(LogVremInput, Warning,
			TEXT("FindInputActionByTag called with invalid GameplayTag in [%s]"),
			*GetNameSafe(this));
		return nullptr;
	}
	
	for (const FTaggedInputAction& Action : TaggedInputActions)
	{
		if (Action.InputTag == Tag)
		{
			return Action.InputAction;
		}
	}

	UE_LOG(LogVremInput, Warning,
	TEXT("InputAction not found for tag [%s] in InputConfig [%s] (Actions: %d)"),
	*Tag.ToString(),
	*GetNameSafe(this),
	TaggedInputActions.Num());
	
	return nullptr;
}
