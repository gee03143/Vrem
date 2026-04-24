// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "Vrem/Equipment/Weapon/VremWeaponHandlerInterface.h"
#include "Vrem/Equipment/VremEquipmentDefinition.h"
#include "VremCharacter.generated.h"

class UHealthComponent;
class UVremGameModeDefinition;
class USpringArmComponent;
class UCameraComponent;
class UVremInputConfig;
class UVremCameraSystem;
class UVremCameraMode;
class UVremInventoryComponent;
class UVremEquipmentComponent;
class UVremWeaponComponent;
struct FInputActionValue;
class UVremEquipmentDefinition;
struct FRecoilProfile;

// TODO: 컴포넌트 구성은 추후 BP에서 관리하는 방향으로 리팩터링 예정.
// 현재는 개발 편의를 위해 C++에서 모든 컴포넌트를 기본 생성.
UCLASS()
class VREM_API AVremCharacter : public ACharacter, public IGameplayTagAssetInterface, public IVremWeaponHandler
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVremCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;

	virtual void Tick(float DeltaTime) override;

#pragma region input
public:	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
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

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UVremCameraMode> DefaultCameraMode;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UVremCameraMode> ADSCameraMode;
#pragma endregion

#pragma region weaponsystem
public:
	float GetCurrentSpreadForUI() const;

protected:
	void OnItemInstanceCreated(class UVremItemInstance* ItemInstance);
	void OnItemInstanceRemoved(const FPrimaryAssetId& ItemId);
	void OnEquipmentActorAttached(const TSubclassOf<UAnimInstance> InAnimLayerClass);
	void OnEquipmentActorDetached(const TSubclassOf<UAnimInstance> InAnimLayerClass);

	void HandleRangeAttackInput(bool bStart = true);
	void HandleMeleeAttackInput();

	void RequestSetCurrentWeapon(int32 InSlotIndex, EEquipmentState PrevOnHandDest = EEquipmentState::Stowed);
	// ~IVremWeaponHandler Interface Begin
	void OnWeaponFired(const FRecoilProfile& RecoilProfile);
	virtual void OnMeleeAttackStarted(int32 ComboIndex) override;
	virtual void OnMeleeAttackFinished() override;
	// ~IVremWeaponHandler Interface End
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UVremInventoryComponent> InventoryComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UVremEquipmentComponent> EquipmentComponent;
#pragma endregion weaponsystem

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UHealthComponent> HealthComponent;
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

private:
	void UpdateMovementStateTags();

	// 상태 변경은 내부나 테스트 클래스에서만

	void AddStateTag(const FGameplayTag& Tag)
	{
		StateTags.AddTag(Tag);
	}

	void RemoveStateTag(const FGameplayTag& Tag)
	{
		StateTags.RemoveTag(Tag);
	}

private:
	FGameplayTagContainer StateTags;
#pragma endregion gameplaytag

};

