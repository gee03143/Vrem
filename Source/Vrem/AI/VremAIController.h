// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "VremAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
struct FAIStimulus;
/**
 * 
 */
UCLASS()
class VREM_API AVremAIController : public AAIController
{
	GENERATED_BODY()

public:
	AVremAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Vrem|AI")
	TObjectPtr<UAIPerceptionComponent> VremPerceptionComponent;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Vrem|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Vrem|AI|Blackboard")
	FName TargetActorKeyName = TEXT("TargetActor");

	UPROPERTY(EditDefaultsOnly, Category = "Vrem|AI|Sight")
	float SightRadius = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Vrem|AI|Sight")
	float LoseSightRadius = 2000.f;

	// total sight angle = PeripheralVisionHalfAngleDegrees * 2
	// Left HalfDegree (PeripheralVisionHalfAngleDegrees) + Right HalfDegree (PeripheralVisionHalfAngleDegrees)
	UPROPERTY(EditDefaultsOnly, Category="Vrem|AI|Sight")
	float PeripheralVisionHalfAngleDegrees = 60.f;

	UPROPERTY(EditDefaultsOnly, Category="Vrem|AI|Sight")
	float SightMaxAge = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="Vrem|AI|Chase")
	float AcceptanceRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Category="Vrem|AI|Chase")
	float LoseInterestDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, Category="Vrem|AI|Chase")
	float RotationInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="Vrem|AI|Faction")
	uint8 TeamId = 1;
	
protected:
	UFUNCTION(BlueprintCallable, Category="Vrem|AI")
	bool HasLineOfSightTo(AActor* Other) const { return IsValid(Other) ? LineOfSightTo(Other) : false; }

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void SetTarget(AActor* NewTarget);
	void ClearTarget();

private:
	TWeakObjectPtr<AActor> CurrentTarget;
	FTimerHandle LoseInterestTimerHandle;
};
