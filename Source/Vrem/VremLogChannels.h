// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

namespace VremDebugKey
{
	constexpr int32 DebugKey_InputMove = 100;
	constexpr int32 DebugKey_InputJump = 101;
	constexpr int32 DebugKey_InputLook = 102;
	constexpr int32 DebugKey_InputWeaponPrimary = 103;
}
	
DECLARE_LOG_CATEGORY_EXTERN(LogVrem, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVremTemp, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVremInput, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVremAssetManager, Log, All);

inline FString GetNetModeString(const UWorld* WorldContext)
{
	if (IsValid(WorldContext) == false)
	{
		return TEXT("Invalid WorldContext");
	}

	switch (WorldContext->GetNetMode())
	{
	case NM_Standalone:
		return TEXT("Standalone");
	case NM_Client:
		return TEXT("Client");
	case NM_ListenServer:
		return TEXT("ListenServer");
	case NM_DedicatedServer:
		return TEXT("DedicatedServer");
	default:
		return TEXT("Unknown");
	}
}

inline FString GetNetRoleString(const AActor* Actor)
{
	if (IsValid(Actor) == false)
	{
		return TEXT("Invalid Actor");
	}

	switch (Actor->GetLocalRole())
	{
	case ROLE_Authority:
		return TEXT("Authority");
	case ROLE_AutonomousProxy:
		return TEXT("AutonomousProxy");
	case ROLE_SimulatedProxy:
		return TEXT("SimulatedProxy");
	default:
		return TEXT("None");
	}
}