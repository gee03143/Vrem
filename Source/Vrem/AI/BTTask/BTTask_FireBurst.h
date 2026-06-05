// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_FireBurst.generated.h"

/**
 * 
 */
UCLASS()
class VREM_API UBTTask_FireBurst : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_FireBurst();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	UPROPERTY(EditAnywhere, Category = "FireBurst")
	float FireDuration = 2.f;

	UPROPERTY(EditAnywhere, Category = "FireBurst")
	FBlackboardKeySelector TargetActorKey;

private:
	void StartFire(UBehaviorTreeComponent& OwnerComp);
	void StopFire(UBehaviorTreeComponent& OwnerComp);

	float ElapsedTime = 0.f;
};
