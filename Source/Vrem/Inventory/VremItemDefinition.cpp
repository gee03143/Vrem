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
// UVremItemDefinition
// =======================================
UItemFragment* UVremItemDefinition::FindFragmentByClass(TSubclassOf<UItemFragment> FragmentClass) const
{
	for (UItemFragment* Fragment : Fragments)
	{
		if (Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}

	return nullptr;
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
			Fragment->OnItemCreated(this);
		}
	}
}

void UVremItemInstance::OnItemRemoved()
{
	for (UItemFragment* Fragment : ItemDef->Fragments)
	{
		if (Fragment)
		{
			Fragment->OnItemRemoved(this);
		}
	}
}

const UVremItemDefinition* UVremItemInstance::GetItemDefinition() const
{
	return ItemDef;
}

UItemFragment* UVremItemInstance::FindFragmentByClass(TSubclassOf<UItemFragment> FragmentClass) const
{
	return IsValid(ItemDef) ? ItemDef->FindFragmentByClass(FragmentClass) : nullptr;
}

int32 UVremItemInstance::GetStateInt(FGameplayTag Key, int32 Default) const
{
	const int32* Found = IntStats.Find(Key);
	return Found ? *Found : Default;
}

void UVremItemInstance::SetStateInt(FGameplayTag Key, int32 Value)
{
	IntStats.FindOrAdd(Key) = Value;
}

bool UVremItemInstance::HasStateInt(FGameplayTag Key) const
{
	return IntStats.Contains(Key);
}

void UVremItemInstance::RemoveStateInt(FGameplayTag Key)
{
	IntStats.Remove(Key);
}