
#include "VremTestTagPawn.h"

void ATestTagPawn::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
    TagContainer.Reset();
    TagContainer.AppendTags(StateTags);
}

