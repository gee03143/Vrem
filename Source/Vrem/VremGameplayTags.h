// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace FVremGameplayTags
{
	// inputs
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Look);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_WeaponPrimary);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_ToggleADS);

	// states
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Aiming_ADS);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Aiming_Scoped);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Moving);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_InAir);
}