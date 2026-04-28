// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "Vrem/Equipment/Weapon/VremWeaponHandlerInterface.h"
#include "VremCharacter.generated.h"

class UVremGameModeDefinition;
class USpringArmComponent;
class UCameraComponent;
class UVremInputConfig;
class UVremCameraSystem;
struct FInputActionValue;
struct FRecoilProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStateTagChangedDelegate, const FGameplayTag&, Tag);

UCLASS()
class VREM_API AVremCharacter : public ACharacter, public IGameplayTagAssetInterface, public IVremWeaponHandler
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVremCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaTime) override;

#pragma region input
public:	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintImplementableEvent, Category="Vrem|Input")
	void OnAttackPressed();

	UFUNCTION(BlueprintImplementableEvent, Category="Vrem|Input")
	void OnAttackReleased();

	UFUNCTION(BlueprintImplementableEvent, Category="Vrem|Input")
	void OnToggleADSPressed();
protected:
	void OnInputConfigLoaded(const UVremGameModeDefinition* InGameModeDefinition);
	
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);

	void Attack_Temp(const FInputActionValue& Value);		// 임시.. 아마 무기 기능같은곳에 들어가야할거같음, 추후 무기 프라이머리 기능으로 들어갈 것
	void StopAttack_Temp(const FInputActionValue& Value);		// 임시.. 아마 무기 기능같은곳에 들어가야할거같음, 추후 무기 프라이머리 기능으로 들어갈 것
	void ToggleADS(const FInputActionValue& Value);	

	void TryBindInputByInputConfig();
protected:
	TSoftObjectPtr<UVremInputConfig> CurrentInputConfig;		// 임시... 나중에 Character 정보를 완전히 데이터로 분리하자
#pragma endregion
	
#pragma region camera
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> FollowCamera;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UVremCameraSystem> CameraSystem;
#pragma endregion

#pragma region weaponsystem
public:
	float GetCurrentSpreadForUI() const;

protected:
	// ~IVremWeaponHandler Interface Begin
	void OnWeaponFired(const FRecoilProfile& RecoilProfile);
	virtual void OnMeleeAttackStarted(int32 ComboIndex) override;
	virtual void OnMeleeAttackFinished() override;
	// ~IVremWeaponHandler Interface End
#pragma endregion weaponsystem
private:
	FDelegateHandle GameModeDefinitionLoadedHandle;

#pragma region gameplaytag
public:
	// IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer.Reset();
		TagContainer.AppendTags(StateTags);
	}

	bool HasStateTag(const FGameplayTag& Tag) const
	{
		return StateTags.HasTag(Tag);
	}

public:
	UPROPERTY(BlueprintAssignable, Category="Vrem|Tag")
    FStateTagChangedDelegate OnStateTagAdded;

    UPROPERTY(BlueprintAssignable, Category="Vrem|Tag")
    FStateTagChangedDelegate OnStateTagRemoved;

public:
	UFUNCTION(BlueprintCallable, Category="Vrem|Combat")
	void EnterMeleeAttackingState();

	UFUNCTION(BlueprintCallable, Category="Vrem|Combat")
	void ExitMeleeAttackingState();

	UFUNCTION(BlueprintCallable, Category="Vrem|Aiming")
	void EnterADSState();

	UFUNCTION(BlueprintCallable, Category="Vrem|Aiming")
	void ExitADSState();

	UFUNCTION(BlueprintCallable, Category="Vrem|Aiming")
	void EnterScopedState();

	UFUNCTION(BlueprintCallable, Category="Vrem|Aiming")
	void ExitScopedState();

private:
	void UpdateMovementStateTags();

	// 상태 변경은 내부나 테스트 클래스에서만
	void AddStateTag(const FGameplayTag& Tag);
	void RemoveStateTag(const FGameplayTag& Tag);
private:
	FGameplayTagContainer StateTags;
#pragma endregion gameplaytag

};

