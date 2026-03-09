// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VremCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UVremInputConfig;
struct FInputActionValue;

UCLASS()
class VREM_API AVremCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVremCharacter();

	virtual void Tick(float DeltaTime) override;

#pragma region input
public:	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);

	void Attack_Temp(const FInputActionValue& Value);		// 임시.. 아마 무기 기능같은곳에 들어가야할거같음, 추후 무기 프라이머리 기능으로 들어갈 것
	void ToggleADS(const FInputActionValue& Value);	
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UVremInputConfig> DefaultInputConfig;		// 임시... 나중에 Character 정보를 완전히 데이터로 분리하자
#pragma endregion

#pragma region camera
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> FollowCamera;
#pragma endregion

private:
	bool bIsADS = false;		// 임시... 추후 GameplayTag로 상태 관리할 것
};
