// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemFragment_Equipment.h"
#include "VremEquipmentDefinition.h"

const UVremEquipmentDefinition* UItemFragment_Equipment::GetEquipmentDefinition() const
{
	return EquipmentDefinition.Get();
}