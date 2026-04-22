// Fill out your copyright notice in the Description page of Project Settings.


#include "VremGameplayTags.h"

namespace FVremGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Move, "Input.Move", "Movement Input");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Jump, "Input.Jump", "Jump Input");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Look, "Input.Look", "Look Input");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_WeaponPrimary, "Input.WeaponPrimary", "Input_WeaponPrimary Input");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_ToggleADS, "Input.ToggleADS", "Toggle ADS Input");

	// states
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Aiming_ADS, "State.Aiming.ADS", "Character is in shoulder ADS mode");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Aiming_Scoped, "State.Aiming.Scoped", "Character is in scope view");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Moving, "State.Movement.Moving", "Character is moving");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_InAir, "State.Movement.InAir", "Character is in air");
}