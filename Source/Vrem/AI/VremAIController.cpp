// Fill out your copyright notice in the Description page of Project Settings.


#include "VremAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Vrem/VremLogChannels.h"

AVremAIController::AVremAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	VremPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("VremPerceptionComponent"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SetPerceptionComponent(*VremPerceptionComponent);
}

void AVremAIController::BeginPlay()
{
	Super::BeginPlay();

	SetGenericTeamId(FGenericTeamId(TeamId));

	if (IsValid(VremPerceptionComponent) == false || IsValid(SightConfig) == false)
	{
		UE_LOG(LogVremAI, Warning, TEXT("[%s] Perception falied to initialize: invalid component"), *GetName());
		return;
	}

	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngleDegrees;
	SightConfig->SetMaxAge(SightMaxAge);

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	VremPerceptionComponent->ConfigureSense(*SightConfig);
	VremPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	VremPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &AVremAIController::OnTargetPerceptionUpdated);
}

void AVremAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsValid(BehaviorTreeAsset))
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
	else
	{
		UE_LOG(LogVremAI, Log, TEXT("[%s] BehaviorTreeAsset is not assigned"), *GetName());
	}
}

void AVremAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (IsValid(Actor) == false)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		GetWorldTimerManager().ClearTimer(LoseInterestTimerHandle);
		SetTarget(Actor);
	}
	else if (CurrentTarget.Get() == Actor)
	{
		// 현재 타겟을 놓침 → 지연 후 추격 포기.
		GetWorldTimerManager().SetTimer(
			LoseInterestTimerHandle, this, &AVremAIController::ClearTarget,
			LoseInterestDelay, false);
	}
}

void AVremAIController::SetTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
	SetFocus(NewTarget);

	UBlackboardComponent* BBComponent = GetBlackboardComponent();
	if (IsValid(BBComponent))
	{
		BBComponent->SetValueAsObject(TargetActorKeyName, NewTarget);
	}
}

void AVremAIController::ClearTarget()
{
	CurrentTarget = nullptr;
	ClearFocus(EAIFocusPriority::Gameplay);

	UBlackboardComponent* BBComponent = GetBlackboardComponent();
	if (IsValid(BBComponent))
	{
		BBComponent->ClearValue(TargetActorKeyName);
	}
}