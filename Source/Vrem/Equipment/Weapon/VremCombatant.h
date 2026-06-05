#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VremCombatant.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UVremCombatant : public UInterface
{
	GENERATED_BODY()
};

class VREM_API IVremCombatant
{
	GENERATED_BODY()

public:
	virtual void StartPrimaryFire() = 0;
	virtual void StopPrimaryFire() = 0;
};