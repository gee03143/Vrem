// Fill out your copyright notice in the Description page of Project Settings.


#include "VremAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
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

void AVremAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* Target = CurrentTarget.Get();
	APawn* ControlledPawn = GetPawn();
	if (IsValid(Target) == false || IsValid(ControlledPawn) == false)
	{
		return;
	}

	const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation());
	if (Distance > AcceptanceRadius)
	{
		// MoveToActor 는 goal actor 를 추적하며 PathFollowingComponent 가 자동 repath.
		// 이미 이동 중이면 재발급하지 않음 (이동 중 player 가 움직여도 PFC 가 따라감).
		if (GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			const EPathFollowingRequestResult::Type Result = MoveToActor(Target, AcceptanceRadius);

#if WITH_EDITOR
			FString ResultString;
			switch (Result)
			{
			case EPathFollowingRequestResult::AlreadyAtGoal:
				ResultString = TEXT("AlreadyAtGoal");
				break;
			case EPathFollowingRequestResult::RequestSuccessful:
				ResultString = TEXT("RequestSuccessful");
				break;
			case EPathFollowingRequestResult::Failed:
				ResultString = TEXT("Failed");
				break;
			default:
				break;
			}
			UE_LOG(LogVremAI, Log, TEXT("MoveToActor dist=%.0f result=%s"), Distance, *ResultString);
#endif
		}
	}
	else
	{
		StopMovement();
		FaceTarget(DeltaTime);
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
		StartChasing(Actor);
	}
	else if (CurrentTarget.Get() == Actor)
	{
		// 현재 타겟을 놓침 → 지연 후 추격 포기.
		GetWorldTimerManager().SetTimer(
			LoseInterestTimerHandle, this, &AVremAIController::StopChasing,
			LoseInterestDelay, false);
	}
}

void AVremAIController::StartChasing(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
	SetFocus(NewTarget);
	UE_LOG(LogVremAI, Log, TEXT("[%s] chase start: Target is [%s]"), *GetName(), *GetNameSafe(NewTarget));
}

void AVremAIController::StopChasing()
{
	UE_LOG(LogVremAI, Log, TEXT("[%s] stop chase"), *GetName());
	ClearFocus(EAIFocusPriority::Gameplay);
	CurrentTarget = nullptr;
	StopMovement();
}

void AVremAIController::FaceTarget(float DeltaTime)
{
	APawn* ControlledPawn = GetPawn();
	AActor* Target = CurrentTarget.Get();
	if (IsValid(ControlledPawn) == false || IsValid(Target) == false)
	{
		return;
	}

	FVector Direction = Target->GetActorLocation() - ControlledPawn->GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRot = ControlledPawn->GetActorRotation();
	const FRotator TargetRot(0.f, Direction.Rotation().Yaw, 0.f);
	const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, RotationInterpSpeed);
	ControlledPawn->SetActorRotation(NewRot);
}
