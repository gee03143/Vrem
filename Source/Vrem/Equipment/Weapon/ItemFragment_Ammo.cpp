// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemFragment_Ammo.h"

bool UItemFragment_Ammo::Matches(const UItemFragment* Other) const
{
	if (Super::Matches(Other) == false)
	{
		return false;
	}

	const UItemFragment_Ammo* OtherAmmo = static_cast<const UItemFragment_Ammo*>(Other);
	return AmmoType == OtherAmmo->AmmoType;
}
