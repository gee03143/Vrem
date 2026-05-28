// Fill out your copyright notice in the Description page of Project Settings.


#include "VremSkillComponent.h"
#include "Vrem/VremLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// =======================================
// FSkillList
// =======================================
void FSkillList::SetOwner(UVremSkillComponent* InOwner)
{
	OwnerComponent = InOwner;
}


void FSkillList::AddEntry(const UVremSkillDefinition* InSkillDefinition, int32 InSlotIndex)
{
	if (OwnerComponent.IsValid() == false)
	{
		UE_LOG(LogVremSkill, Warning, TEXT("FSkillList::AddEntry OwnerComponent is nullptr"));
		return;
	}

	check(IsValid(OwnerComponent->GetOwner()));
	check(OwnerComponent->GetOwner()->HasAuthority());

	if (IsValid(InSkillDefinition) == false)
	{
		UE_LOG(LogVremSkill, Warning, TEXT("FSkillList::AddEntry InSkillDefinition is invalid"));
		return;
	}

	if (GetEntryFromIndex(InSlotIndex))
	{
		RemoveEntry(InSlotIndex);
	}

	FSkillEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.SkillDefinition = InSkillDefinition;
	NewEntry.SkillSlotIndex = InSlotIndex;
	NewEntry.CooldownEndTime = 0.f;
	MarkItemDirty(NewEntry);
}

void FSkillList::RemoveEntry(int32 InSlotIndex)
{
	if (OwnerComponent.IsValid() == false)
	{
		UE_LOG(LogVremSkill, Warning, TEXT("FSkillList::RemoveEntry OwnerComponent is nullptr"));
		return;
	}

	check(IsValid(OwnerComponent->GetOwner()));
	check(OwnerComponent->GetOwner()->HasAuthority());

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (Entries[i].SkillSlotIndex == InSlotIndex)
		{
			Entries.RemoveAt(i);
			MarkArrayDirty();
			return;
		}
	}
}

void FSkillList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (OwnerComponent.IsValid())
	{
		OwnerComponent->NotifySkillListChanged();
	}
}

void FSkillList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (OwnerComponent.IsValid())
	{
		OwnerComponent->NotifySkillListChanged();
	}
}

void FSkillList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	if (OwnerComponent.IsValid())
	{
		OwnerComponent->NotifySkillListChanged();
	}
}

// =======================================
// UVremSkillComponent
// =======================================
UVremSkillComponent::UVremSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UVremSkillComponent::InitializeComponent()
{
	Super::InitializeComponent();
	SkillList.SetOwner(this);
}

void UVremSkillComponent::BeginPlay()
{
    Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		for (int32 i = 0; i < DefaultGrantedSkills.Num(); ++i)
		{
			AddSkill(DefaultGrantedSkills[i], i + 1 /* Skill slot 1 == input Skill1 */);
		}
	}
}

void UVremSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UVremSkillComponent, SkillList, COND_OwnerOnly);
}

void UVremSkillComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentTargetingSlotIndex == INDEX_NONE)
	{
		return;
	}

	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(CurrentTargetingSlotIndex);
	if (Entry == nullptr)
	{
		CancelTargetingLocal();
		return;
	}

	const UVremSkillDefinition* Def = Entry->SkillDefinition.Get();
	if (IsValid(Def) == false)
	{
		return;
	}

	CurrentTargetingContext = BuildActivationContext();   // 조준 위치/방향 갱신

	const UVremSkillBehavior* CDO = Def->GetBehaviorCDO();
	if (IsValid(CDO))
	{
		CDO->UpdateTargeting(CurrentTargetingContext, Def);
	}
}

void UVremSkillComponent::RequestAddSkill(const UVremSkillDefinition* InSkillDefinition, int32 InSlotIndex)
{
	if (IsValid(GetOwner()) == false)
	{
		UE_LOG(LogVremSkill, Warning, TEXT("UVremSkillComponent::RequestAddSkill GetOwner is invalid"));
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		AddSkill(InSkillDefinition, InSlotIndex);
	}
	else
	{
		ServerAddSkill(InSkillDefinition, InSlotIndex);
	}
}

void UVremSkillComponent::RequestRemoveSkill(int32 InSlotIndex)
{
	if (IsValid(GetOwner()) == false)
	{
		UE_LOG(LogVremSkill, Warning, TEXT("UVremSkillComponent::RequestRemoveSkill GetOwner is invalid"));
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		RemoveSkill(InSlotIndex);
	}
	else
	{
		ServerRemoveSkill(InSlotIndex);
	}
}

void UVremSkillComponent::RequestCancelTargeting()
{
	CancelTargetingLocal();
}

void UVremSkillComponent::RequestActivateSkill(int32 SlotIndex)
{
	if (CurrentTargetingSlotIndex == SlotIndex)
	{
		ConfirmTargetingLocal();
		return;
	}

	if (CurrentTargetingSlotIndex != INDEX_NONE)
	{
		CancelTargetingLocal();
	}

	if (CanActivateSkill(SlotIndex) == false)
	{
		return;
	}

	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	if (Entry == nullptr)
	{
		return;
	}
	const UVremSkillDefinition* Def = Entry->SkillDefinition.Get();
	if (IsValid(Def) == false)
	{
		return;
	}

	if (Def->ActivationMode == EVremSkillActivationMode::Instant)
	{
		const FVremSkillActivationContext Context = BuildActivationContext();
		ExecuteActivateSkill(SlotIndex, Context);
	}
	else // EVremSkillActivationMode::Targeting
	{
		BeginTargetingLocal(SlotIndex);
	}
}

bool UVremSkillComponent::CanActivateSkill(int32 SlotIndex) const
{
	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	if (Entry == nullptr)
	{
		return false;
	}

	const UVremSkillDefinition* Def = Entry->SkillDefinition.Get();
	if (IsValid(Def) == false || Def->GetBehaviorCDO() == nullptr)
	{
		UE_LOG(LogVremSkill, Warning, TEXT("UVremSkillComponent::CanActivateSkill: slot %d has no valid Definition/BehaviorClass"), SlotIndex);
		return false;
	}

	return GetWorld()->GetTimeSeconds() >= Entry->CooldownEndTime;
}

float UVremSkillComponent::GetCooldownRemaining(int32 SlotIndex) const
{
	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	if (Entry == nullptr)
	{
		return 0.f;
	}

	return FMath::Max(0.f, Entry->CooldownEndTime - GetWorld()->GetTimeSeconds());
}

const UVremSkillDefinition* UVremSkillComponent::GetSkillDefinition(int32 SlotIndex) const
{
	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	return Entry != nullptr ? Entry->SkillDefinition.Get() : nullptr;
}

void UVremSkillComponent::ServerActivateSkill_Implementation(int32 SlotIndex, FVector AimLocation, FVector AimDirection)
{
	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	if (Entry == nullptr)
	{
		return;
	}

	const UVremSkillDefinition* Def = Entry->SkillDefinition.Get();
	if (IsValid(Def) == false || Def->BehaviorClass == nullptr)
	{
		return;
	}

	FVremSkillActivationContext Context;
	Context.Instigator = GetOwner();
	Context.AimLocation = AimLocation;
	Context.AimDirection = AimDirection;

	const UVremSkillBehavior* CDO = Def->GetBehaviorCDO();
	if (CDO)
	{
		CDO->Activate(Context, Def);
	}

	StartCooldownLocal(SlotIndex);
}

void UVremSkillComponent::ServerAddSkill_Implementation(const UVremSkillDefinition* SkillDefinition, int32 SlotIndex)
{
	AddSkill(SkillDefinition, SlotIndex);
}

void UVremSkillComponent::ServerRemoveSkill_Implementation(int32 SlotIndex)
{
	RemoveSkill(SlotIndex);
}

void UVremSkillComponent::AddSkill(const UVremSkillDefinition* InSkillDefinition, int32 InSlotIndex)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	SkillList.AddEntry(InSkillDefinition, InSlotIndex);
	OnSkillListChanged.Broadcast();
}

void UVremSkillComponent::RemoveSkill(int32 InSlotIndex)
{
	check(IsValid(GetOwner()));
	check(GetOwner()->HasAuthority());

	SkillList.RemoveEntry(InSlotIndex);
	OnSkillListChanged.Broadcast();
}

void UVremSkillComponent::ExecuteActivateSkill(int32 SlotIndex, const FVremSkillActivationContext& Context)
{
	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	if (Entry == nullptr)
	{
		return;
	}

	ServerActivateSkill(SlotIndex, Context.AimLocation, Context.AimDirection);

	StartCooldownLocal(SlotIndex);
	OnSkillActivated.Broadcast(SlotIndex);
}

void UVremSkillComponent::StartCooldownLocal(int32 SlotIndex)
{
	FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	if (Entry == nullptr)
	{
		return;
	}

	const UVremSkillDefinition* Def = Entry->SkillDefinition.Get();
	if (IsValid(Def) && Def->Cooldown > 0.f)
	{
		Entry->CooldownEndTime = GetWorld()->GetTimeSeconds() + Def->Cooldown;
	}
}

void UVremSkillComponent::BeginTargetingLocal(int32 SlotIndex)
{
	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	if (Entry == nullptr)
	{
		return;
	}
	const UVremSkillDefinition* Def = Entry->SkillDefinition.Get();
	if (IsValid(Def) == false || Def->BehaviorClass == nullptr)
	{
		return;
	}

	CurrentTargetingSlotIndex = SlotIndex;
	CurrentTargetingContext = BuildActivationContext();

	const UVremSkillBehavior* CDO = Def->GetBehaviorCDO();
	if (IsValid(CDO))
	{
		CDO->BeginTargeting(CurrentTargetingContext, Def);
	}

	SetComponentTickEnabled(true);
}

void UVremSkillComponent::ConfirmTargetingLocal()
{
	if (CurrentTargetingSlotIndex == INDEX_NONE)
	{
		return;
	}

	const int32 SlotIndex = CurrentTargetingSlotIndex;
	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(SlotIndex);
	if (Entry == nullptr)
	{
		CancelTargetingLocal();
		return;
	}
	const UVremSkillDefinition* Def = Entry->SkillDefinition.Get();
	if (IsValid(Def) == false)
	{
		CancelTargetingLocal();
		return;
	}

	CurrentTargetingContext = BuildActivationContext();   // 확정 시점 갱신

	const UVremSkillBehavior* CDO = Def->GetBehaviorCDO();
	if (IsValid(CDO))
	{
		CDO->EndTargeting(CurrentTargetingContext, Def, true /*bConfirmed*/);
	}

	// 상태 정리 후 실제 발동
	const FVremSkillActivationContext FinalContext = CurrentTargetingContext;
	CurrentTargetingSlotIndex = INDEX_NONE;
	SetComponentTickEnabled(false);

	ExecuteActivateSkill(SlotIndex, FinalContext);
}

void UVremSkillComponent::CancelTargetingLocal()
{
	if (CurrentTargetingSlotIndex == INDEX_NONE)
	{
		return;
	}

	const FSkillEntry* Entry = SkillList.GetEntryFromIndex(CurrentTargetingSlotIndex);
	if (Entry != nullptr)
	{
		const UVremSkillDefinition* Def = Entry->SkillDefinition.Get();
		if (IsValid(Def) && Def->BehaviorClass != nullptr)
		{
			const UVremSkillBehavior* CDO = Def->GetBehaviorCDO();
			if (IsValid(CDO))
			{
				CDO->EndTargeting(CurrentTargetingContext, Def, false /*bConfirmed*/);
			}
		}
	}

	CurrentTargetingSlotIndex = INDEX_NONE;
	SetComponentTickEnabled(false);
}

void UVremSkillComponent::OnRep_SkillList()
{
	SkillList.SetOwner(this);
	OnSkillListChanged.Broadcast();
}

FVremSkillActivationContext UVremSkillComponent::BuildActivationContext() const
{
    FVremSkillActivationContext Context;
    Context.Instigator = GetOwner();

    APlayerController* PC = Cast<APlayerController>(GetInstigatorController());
    if (IsValid(PC))
    {
        FVector ViewOrigin;
        FRotator ViewRotation;
        PC->GetPlayerViewPoint(ViewOrigin, ViewRotation);
        Context.AimLocation = ViewOrigin;
        Context.AimDirection = ViewRotation.Vector();
    }
    return Context;
}

AController* UVremSkillComponent::GetInstigatorController() const
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    return Pawn ? Pawn->GetController() : nullptr;
}

