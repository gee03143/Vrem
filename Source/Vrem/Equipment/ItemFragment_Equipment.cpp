// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemFragment_Equipment.h"
#include "VremEquipmentDefinition.h"

EEquipmentSlotType UItemFragment_Equipment::GetSlotType() const
{
	return EquipmentDefinition ? EquipmentDefinition->SlotType : EEquipmentSlotType::NUM_EEquipmentSlotType;
}

const UVremEquipmentDefinition* UItemFragment_Equipment::GetEquipmentDefinition() const
{
	return EquipmentDefinition.Get();
}