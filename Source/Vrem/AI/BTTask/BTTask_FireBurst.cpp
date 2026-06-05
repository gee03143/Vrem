// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FireBurst.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Vrem/Equipment/Weapon/VremCombatant.h"

UBTTask_FireBurst::UBTTask_FireBurst()
{
	NodeName = TEXT("Fire Burst");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	bCreateNodeInstance = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FireBurst, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_FireBurst::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ElapsedTime = 0.f;
	StartFire(OwnerComp);
	return EBTNodeResult::InProgress;
}

void UBTTask_FireBurst::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	ElapsedTime += DeltaSeconds;

	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;

	const bool bHasLOS = (IsValid(AICon) && IsValid(Target)) ? AICon->LineOfSightTo(Target) : false;

	if (ElapsedTime >= FireDuration || bHasLOS == false)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_FireBurst::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

void UBTTask_FireBurst::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	StopFire(OwnerComp);
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_FireBurst::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BB = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BB);
	}
}

void UBTTask_FireBurst::StartFire(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (IsValid(AICon))
	{
		APawn* Pawn = AICon->GetPawn();
		if (IsValid(Pawn))
		{
			if (IVremCombatant* Handler = Cast<IVremCombatant>(Pawn))
			{
				Handler->StartPrimaryFire();
			}
		}
	}
}

void UBTTask_FireBurst::StopFire(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (IsValid(AICon))
	{
		APawn* Pawn = AICon->GetPawn();
		if (IsValid(Pawn))
		{
			if (IVremCombatant* Handler = Cast<IVremCombatant>(Pawn))
			{
				Handler->StopPrimaryFire();
			}
		}
	}
}