// Fill out your copyright notice in the Description page of Project Settings.


#include "VremAnimInstance.h"
#include "Vrem/Character/VremCharacter.h"
#include "Vrem/VremLogChannels.h"

void UVremAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CacheTagBindingProperties();
}

void UVremAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	APawn* PawnOwner = TryGetPawnOwner();
	if (!IsValid(PawnOwner))
	{
		UE_LOG(LogVremInput, Warning, TEXT("UVremAnimInstance::NativeBeginPlay invalid PawnOwner"));
		return;
	}

	AVremCharacter* VremChar = Cast<AVremCharacter>(PawnOwner);
	if (IsValid(VremChar))
	{
		CachedVremCharacter = VremChar;

		CachedVremCharacter->OnStateTagAdded.AddDynamic(this, &UVremAnimInstance::HandleStateTagAdded);
		CachedVremCharacter->OnStateTagRemoved.AddDynamic(this, &UVremAnimInstance::HandleStateTagRemoved);

		FGameplayTagContainer OwnedTags;
		CachedVremCharacter->GetOwnedGameplayTags(OwnedTags);
		for (const FVremAnimTagBinding& Binding : TagBindings)
		{
			ApplyTagChange(Binding.Tag, OwnedTags.HasTag(Binding.Tag));
		}
	}
	else
	{
		UE_LOG(LogVremInput, Warning, TEXT("UVremAnimInstance::NativeBeginPlay invalid VremChar"));
	}
}

void UVremAnimInstance::NativeUninitializeAnimation()
{
	if (CachedVremCharacter.Get())
	{
		CachedVremCharacter->OnStateTagAdded.RemoveDynamic(this, &UVremAnimInstance::HandleStateTagAdded);
		CachedVremCharacter->OnStateTagRemoved.RemoveDynamic(this, &UVremAnimInstance::HandleStateTagRemoved);
	}
	ResolvedTagProperties.Empty();

	Super::NativeUninitializeAnimation();
}

void UVremAnimInstance::CacheTagBindingProperties()
{
	ResolvedTagProperties.Empty();
	UClass* AnimClass = GetClass();
	for (const FVremAnimTagBinding& Binding : TagBindings)
	{
		if (!Binding.Tag.IsValid() || Binding.PropertyToEdit.IsNone())
		{
			continue;
		}
		if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(AnimClass, Binding.PropertyToEdit))
		{
			ResolvedTagProperties.Add(Binding.Tag, BoolProp);
		}
	}
}

void UVremAnimInstance::ApplyTagChange(const FGameplayTag& Tag, bool bIsAdded)
{
	if (FBoolProperty** Found = ResolvedTagProperties.Find(Tag))
	{
		(*Found)->SetPropertyValue_InContainer(this, bIsAdded);
	}
}

void UVremAnimInstance::HandleStateTagAdded(const FGameplayTag& Tag)
{
	ApplyTagChange(Tag, true);
}

void UVremAnimInstance::HandleStateTagRemoved(const FGameplayTag& Tag)
{
	ApplyTagChange(Tag, false);
}
