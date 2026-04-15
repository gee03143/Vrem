#pragma once

#include "CoreMinimal.h"
#include "Vrem/Inventory/VremItemDefinition.h"
#include "ItemFragment_Equipment.generated.h"

class UVremEquipmentDefinition;

UCLASS()
class UItemFragment_Equipment : public UItemFragment
{
    GENERATED_BODY()

public:
	const UVremEquipmentDefinition* GetEquipmentDefinition() const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UVremEquipmentDefinition> EquipmentDefinition;
};