// Fill out your copyright notice in the Description page of Project Settings.


#include "VremItemDefinition.h"
#include "Vrem/VremLogChannels.h"


// =======================================
// UItemFragment_Dummy
// =======================================
void UItemFragment_Dummy::OnItemCreated(class UVremItemInstance* Instance)
{
	UE_LOG(LogVremInventory, Warning, TEXT("param1 : [%d] param2 : [%d]\nNetMode : %s"), Param1, Param2, *GetNetModeString(GetWorld()));
}

// =======================================
// UVremItemInstance
// =======================================
void UVremItemInstance::OnItemCreated(UVremItemDefinition* InItemDefinition)
{
	ItemDef = InItemDefinition;

	if (InItemDefinition == nullptr)
	{
		UE_LOG(LogVremInventory, Warning, TEXT("UVremItemInstance::OnItemCreated InItemDefinition is nullptr\nNetMode : %s"), *GetNetModeString(GetWorld()));
		return;
	}

	for (UItemFragment* Fragment : InItemDefinition->Fragments)
	{
		if (Fragment)
		{
			UItemFragment* NewFragment = DuplicateObject<UItemFragment>(Fragment, this);

			InstanceFragments.Add(NewFragment);
			NewFragment->OnItemCreated(this);
		}
	}
}

void UVremItemInstance::OnItemRemoved()
{
	for (UItemFragment* Fragment : InstanceFragments)
	{
		Fragment->OnItemRemoved(this);
	}
}