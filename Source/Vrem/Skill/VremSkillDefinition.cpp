// Fill out your copyright notice in the Description page of Project Settings.


#include "VremSkillDefinition.h"
#include "VremSkillBehavior.h"

const UVremSkillBehavior* UVremSkillDefinition::GetBehaviorCDO() const
{
	return BehaviorClass != nullptr ? BehaviorClass->GetDefaultObject<UVremSkillBehavior>() : nullptr;
}
