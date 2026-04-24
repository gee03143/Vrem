// Fill out your copyright notice in the Description page of Project Settings.


#include "VremCharacter.h"

#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "HealthComponent.h"
#include "VremPawnData.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "Vrem/VremGameplayTags.h"
#include "Vrem/VremLogChannels.h"
#include "Vrem/GameMode/VremGameModeDefinition.h"
#include "Vrem/GameMode/VremGameModeDefManager.h"
#include "Vrem/Camera/VremCameraSystem.h"
#include "Vrem/Camera/VremCameraMode.h"
#include "Vrem/Input/VremInputConfig.h"
#include "Vrem/Inventory/VremInventoryComponent.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"
#include "Vrem/Equipment/VremEquipmentDefinition.h"
#include "Vrem/Equipment/ItemFragment_Equipment.h"
#include "Vrem/Equipment/VremEquipmentActor.h"
#include "Vrem/Equipment/Weapon/VremWeaponComponent.h"
#include "Vrem/Equipment/MeleeWeapon/VremMeleeComponent.h"
#include "Vrem/Animation/VremAnimInstance.h"
#include "Vrem/VremAssetManager.h"
#include "Components/CapsuleComponent.h"

static TAutoConsoleVariable<int32> CVarDebugCharacterInput(
	TEXT("vrem.DebugCharacterInput"),
	0,
	TEXT("Enable Character Input Debug\n")
	TEXT("0: Off\n")
	TEXT("1: On"),
	ECVF_Cheat);

// Sets default values
AVremCharacter::AVremCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);

	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);

	FollowCamera->bUsePawnControlRotation = false;

	CameraSystem = CreateDefaultSubobject<UVremCameraSystem>(TEXT("CameraSystem"));

	InventoryComponent = CreateDefaultSubobject<UVremInventoryComponent>(TEXT("InventoryComponent"));

	EquipmentComponent = CreateDefaultSubobject<UVremEquipmentComponent>(TEXT("EquipmentComponent"));

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void AVremCharacter::BeginPlay()
{
	Super::BeginPlay();

	AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();
	if (IsValid(GS))
	{
		UVremGameModeDefManager* GameModeManager = GS->GetComponentByClass<UVremGameModeDefManager>();
		if (IsValid(GameModeManager))
		{
			if (GameModeManager->IsGameModeDefinitionLoaded())
			{
				OnInputConfigLoaded(GameModeManager->GetGameModeDefinition());
			}
			else
			{
				GameModeDefinitionLoadedHandle = GameModeManager->OnGameModeDefinitionLoadedDelegate.AddUObject(this, &ThisClass::OnInputConfigLoaded);
			}
		}
	}

	if (IsValid(CameraSystem) && IsValid(DefaultCameraMode))
	{
		CameraSystem->SetTargetCameraMode(DefaultCameraMode);
	}

	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->RegisterComponentWithWorld(GetWorld());
		EquipmentComponent->InitializeFromOwner();
	}

	if (IsValid(InventoryComponent))
	{
		EquipmentComponent->RegisterComponentWithWorld(GetWorld());
		InventoryComponent->InitializeFromOwner();
	}
	
	if (IsValid(HealthComponent))
	{
		EquipmentComponent->RegisterComponentWithWorld(GetWorld());
		HealthComponent->InitializeFromOwner();
	}
	
	UE_LOG(LogTemp, Warning, TEXT(
		"Capsule Collision: Enabled=%d, Visibility=%d"),
		(int32)GetCapsuleComponent()->GetCollisionEnabled(),
		(int32)GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Visibility)
	);
}

void AVremCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GameModeDefinitionLoadedHandle.IsValid())
	{
		GameModeDefinitionLoadedHandle.Reset();
	}
	
	Super::EndPlay(EndPlayReason);
}

void AVremCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsValid(InventoryComponent) && HasAuthority())
	{
		InventoryComponent->OnItemInstanceCreated.AddUObject(this, &ThisClass::OnItemInstanceCreated);
		InventoryComponent->OnItemInstanceRemoved.AddUObject(this, &ThisClass::OnItemInstanceRemoved);
	}
	
	if (IsValid(EquipmentComponent) && GetNetMode() != NM_DedicatedServer)
	{
		EquipmentComponent->OnEquipmenntAttached.AddUObject(this, &ThisClass::OnEquipmentActorAttached);
		EquipmentComponent->OnEquipmenntDetached.AddUObject(this, &ThisClass::OnEquipmentActorDetached);
	}
}

void AVremCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{ 
		if (CameraSystem->IsBlending())
		{
			const FVremCameraState& BlendedCameraState = CameraSystem->GetBlendedCameraState();

			if (IsValid(FollowCamera))
			{
				FollowCamera->SetFieldOfView(BlendedCameraState.TargetFOV);
			}

			if (IsValid(SpringArm))
			{
				SpringArm->TargetArmLength = BlendedCameraState.TargetArmLength;
				SpringArm->SocketOffset = BlendedCameraState.TargetSocketOffset;
			}
		}
		else
		{
			CameraSystem->SetTargetCameraMode(nullptr);
		}
	}

	UpdateMovementStateTags();
}

inline void AVremCharacter::OnInputConfigLoaded(const UVremGameModeDefinition* InGameModeDefinition)
{
	CurrentInputConfig = InGameModeDefinition->PawnData->InputConfig;	// TODO: 이부분은 개선 여지가 있을 것 같다, Character가 GameModeDefinition 전체를 알 필요는 없다.
	
	TryBindInputByInputConfig();
}

// Called to bind functionality to input
void AVremCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	TryBindInputByInputConfig();
}

void AVremCharacter::Move(const FInputActionValue& Value)
{
	FVector2D InputValue = Value.Get<FVector2D>();

	if (IsValid(Controller) == false)
	{
		UE_LOG(LogVremInput, Warning, TEXT("AVremCharacter::InputMove controller is nullptr"));
		return;
	}

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (CVarDebugCharacterInput.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(
			VremDebugKey::DebugKey_InputMove,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("Move Input X: %.2f Y: %.2f"),
			InputValue.X,
			InputValue.Y));
	}
	
	AddMovementInput(ForwardDirection, InputValue.Y);
	AddMovementInput(RightDirection, InputValue.X);
	
}

void AVremCharacter::Look(const FInputActionValue& Value)
{
	FVector2D InputValue = Value.Get<FVector2D>();

	if (IsValid(Controller) == false)
	{
		UE_LOG(LogVremInput, Warning, TEXT("AVremCharacter::Look controller is nullptr"));
		return;
	}
	
	if (CVarDebugCharacterInput.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(
			VremDebugKey::DebugKey_InputLook,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("Look Input X: %.2f Y: %.2f"),
			InputValue.X,
			InputValue.Y));
	}
	
	AddControllerYawInput(InputValue.X);
	AddControllerPitchInput(InputValue.Y);
}

void AVremCharacter::StartJump(const FInputActionValue& Value)
{
	if (CVarDebugCharacterInput.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(
			VremDebugKey::DebugKey_InputJump,
			1.f,
			FColor::Yellow,
			TEXT("Jump Start"));
	}
	
	Jump();
}

void AVremCharacter::StopJump(const FInputActionValue& Value)
{
	if (CVarDebugCharacterInput.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(
			VremDebugKey::DebugKey_InputJump,
			1.f,
			FColor::Yellow,
			TEXT("Jump Stop"));
	}
	
	StopJumping();
}

void AVremCharacter::Attack_Temp(const FInputActionValue& Value)
{
	const bool bIsAiming =
		HasStateTag(FVremGameplayTags::State_Aiming_ADS) ||
		HasStateTag(FVremGameplayTags::State_Aiming_Scoped);

	if (bIsAiming)
	{
		HandleRangeAttackInput(true /*bStart*/);
	}
	else
	{
		HandleMeleeAttackInput();
	}
}

void AVremCharacter::StopAttack_Temp(const FInputActionValue& Value)
{
	const bool bIsAiming =
		HasStateTag(FVremGameplayTags::State_Aiming_ADS) ||
		HasStateTag(FVremGameplayTags::State_Aiming_Scoped);

	if (bIsAiming)
	{
		HandleRangeAttackInput(false /*bStart*/);
	}
}

void AVremCharacter::ToggleADS(const FInputActionValue& Value)
{
	// 근접 공격 모션 중엔 ADS 전환 차단
	if (HasStateTag(FVremGameplayTags::State_Combat_MeleeAttacking))
	{
		return;
	}

	// OnHand가 근접이면 원거리로 먼저 복귀 시도
	if (IsValid(EquipmentComponent) && EquipmentComponent->GetOnHandSlotType() == EEquipmentSlotType::Melee)
	{
		const int32 HolsteredIndex = EquipmentComponent->GetHolsteredSlotIndex();
		if (HolsteredIndex == INDEX_NONE)
		{
			// 복귀할 원거리 무기 없음 → ADS 진입 불가
			UE_LOG(LogVremEquipment, Log, TEXT("ToggleADS: No holstered ranged weapon to return to"));
			return;
		}
		RequestSetCurrentWeapon(HolsteredIndex, EEquipmentState::Holstered);
	}

	if (HasStateTag(FVremGameplayTags::State_Aiming_ADS))
	{
		RemoveStateTag(FVremGameplayTags::State_Aiming_ADS);
		HandleRangeAttackInput(false);

		if (IsValid(CameraSystem))
		{
			CameraSystem->SetTargetCameraMode(DefaultCameraMode);
		}
	}
	else
	{
		AddStateTag(FVremGameplayTags::State_Aiming_ADS);

		if (IsValid(CameraSystem))
		{
			CameraSystem->SetTargetCameraMode(ADSCameraMode);
		}
	}
}

void AVremCharacter::TryBindInputByInputConfig()
{
	if (CurrentInputConfig.IsValid() == false)
	{
		UE_LOG(LogVremInput, Warning, TEXT("AVremCharacter::SetupPlayerInputComponent InputConfig Is Invalid! NetRole : [%s]"), *GetNetRoleString(this));
		return;
	}

	if (IsValid(InputComponent) == false)
	{
		UE_LOG(LogVremInput, Warning, TEXT("AVremCharacter::SetupPlayerInputComponent InputComponent Is Invalid! NetRole : [%s]"), *GetNetRoleString(this));
		return;
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EIC))
	{
		const UInputAction* MoveAction = CurrentInputConfig->FindInputActionByTag(FVremGameplayTags::Input_Move);
		if (MoveAction != nullptr)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVremCharacter::Move);
		}

		const UInputAction* JumpAction = CurrentInputConfig->FindInputActionByTag(FVremGameplayTags::Input_Jump);
		if (JumpAction != nullptr)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AVremCharacter::StartJump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AVremCharacter::StopJump);
		}

		const UInputAction* TurnAction = CurrentInputConfig->FindInputActionByTag(FVremGameplayTags::Input_Look);
		if (TurnAction != nullptr)
		{
			EIC->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AVremCharacter::Look);
		}

		const UInputAction* AttackAction = CurrentInputConfig->FindInputActionByTag(FVremGameplayTags::Input_WeaponPrimary);
		if (AttackAction != nullptr)
		{
			EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AVremCharacter::Attack_Temp);
			EIC->BindAction(AttackAction, ETriggerEvent::Completed, this, &AVremCharacter::StopAttack_Temp);
		}

		const UInputAction* ToggleADSAction = CurrentInputConfig->FindInputActionByTag(FVremGameplayTags::Input_ToggleADS);
		if (ToggleADSAction != nullptr)
		{
			EIC->BindAction(ToggleADSAction, ETriggerEvent::Completed, this, &AVremCharacter::ToggleADS);
		}
	}
	else
	{
		UE_LOG(LogVremInput, Warning, TEXT("AVremCharacter::SetupPlayerInputComponent EnhancedInputComponent Is Invalid! NetRole : [%s]"), *GetNetRoleString(this));
	}
}

float AVremCharacter::GetCurrentSpreadForUI() const
{
	AVremEquipmentActor* OnHandActor = EquipmentComponent->GetCurrentEquipmentActor();
	if (IsValid(OnHandActor) == false)
	{
		return 0.f;
	}

	UVremWeaponComponent* Weapon = OnHandActor->FindComponentByClass<UVremWeaponComponent>();
	if (IsValid(Weapon))
	{
		return Weapon->GetCurrentSpread();
	}
	return 0.f;
}

void AVremCharacter::OnItemInstanceCreated(UVremItemInstance* ItemInstance)
{
	// server only
	check(HasAuthority());

	if (IsValid(EquipmentComponent) == false)
	{
		return;
	}


	const bool bHandEmpty = EquipmentComponent->GetOnHandSlotIndex() == INDEX_NONE;
	UItemFragment_Equipment* EquipmentFragment = ItemInstance->FindFragment<UItemFragment_Equipment>();
	if (EquipmentFragment)
	{
		 const int32 EquipmentIndex = EquipmentComponent->GetEquipmentItemNum() + 1;
		EquipmentComponent->TryEquipItem(EquipmentFragment->GetEquipmentDefinition(), EquipmentIndex); 

		if (bHandEmpty)
		{
			EquipmentComponent->SetCurrentWeapon(1);
		}
		else
		{
			const bool bHolsterEmpty = EquipmentComponent->GetHolsteredSlotIndex() == INDEX_NONE;
			const EEquipmentSlotType OnHandEquipmentSlot = EquipmentComponent->GetOnHandSlotType();
			if (bHolsterEmpty && EquipmentFragment->GetEquipmentDefinition()->SlotType != OnHandEquipmentSlot)
			{
				EquipmentComponent->SetCurrentWeapon(EquipmentIndex, EEquipmentState::Holstered);
				
			}
		}
	}
}

void AVremCharacter::OnItemInstanceRemoved(const FPrimaryAssetId& ItemId)
{
	UVremAssetManager& Manager = UVremAssetManager::Get();
	UVremItemDefinition* ItemDef = Manager.GetItemDefinition(ItemId);
	if (IsValid(ItemDef) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnItemInstanceRemoved Failed to get ItemDefinition"));
		return;
	}

	UItemFragment_Equipment* EquipmentFragment = ItemDef->FindFragment<UItemFragment_Equipment>();
	if (IsValid(EquipmentFragment) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnItemInstanceRemoved Item Has no ItemFragment_Equipment"));
		return;
	}

	const UVremEquipmentDefinition* EquipDef = EquipmentFragment->GetEquipmentDefinition();
	if (IsValid(EquipDef))
	{
		EquipmentComponent->TryUnequipItem(EquipDef);
	}
}

void AVremCharacter::OnEquipmentActorAttached(const TSubclassOf<UAnimInstance> InAnimLayerClass)
{
	if (IsValid(GetMesh()) == false)
	{
		return;
	}

	UVremAnimInstance* AnimInstance = Cast<UVremAnimInstance>(GetMesh()->GetAnimInstance());
	if (IsValid(AnimInstance) && InAnimLayerClass != nullptr)
	{
		AnimInstance->SetWeaponAnimLayer(InAnimLayerClass);
	}
}

void AVremCharacter::OnEquipmentActorDetached(const TSubclassOf<UAnimInstance> InAnimLayerClass)
{	
	if (IsValid(GetMesh()) == false)
	{
		return;
	}

	UVremAnimInstance* AnimInstance = Cast<UVremAnimInstance>(GetMesh()->GetAnimInstance());
	if (IsValid(AnimInstance) && InAnimLayerClass != nullptr)
	{
		if (AnimInstance->GetCurrentLayer() == InAnimLayerClass)
		{
			AnimInstance->SetWeaponAnimLayer(nullptr);
		}
	}
}

void AVremCharacter::OnWeaponFired(const FRecoilProfile& RecoilProfile)
{
	const float VerticalPitch = -RecoilProfile.VerticalKick
		+ FMath::RandRange(-RecoilProfile.VerticalVariance, RecoilProfile.VerticalVariance);
	AddControllerPitchInput(VerticalPitch);

	const float HorizontalYaw = FMath::RandRange(-RecoilProfile.HorizontalKick, RecoilProfile.HorizontalKick);
	AddControllerYawInput(HorizontalYaw);

	if (IsValid(CameraSystem))
	{
		CameraSystem->AddTransientFOVKick(RecoilProfile.FOVKick, RecoilProfile.FOVRecoverSpeed);
	}
}

void AVremCharacter::OnMeleeAttackStarted(int32 ComboIndex)
{
	AddStateTag(FVremGameplayTags::State_Combat_MeleeAttacking);
}

void AVremCharacter::OnMeleeAttackFinished()
{
	RemoveStateTag(FVremGameplayTags::State_Combat_MeleeAttacking);
}

void AVremCharacter::HandleRangeAttackInput(bool bStart)
{
	if (IsValid(EquipmentComponent) == false)
	{
		return;
	}

	AVremEquipmentActor* OnHandActor = EquipmentComponent->GetCurrentEquipmentActor();
	if (IsValid(OnHandActor) == false)
	{
		return;
	}

	UVremWeaponComponent* Weapon = OnHandActor->FindComponentByClass<UVremWeaponComponent>();
	if (IsValid(Weapon))
	{
		if (bStart)
		{
			Weapon->Fire();
		}
		else
		{
			Weapon->StopFire();
		}
	}
}

void AVremCharacter::HandleMeleeAttackInput()
{
	// 근접 공격 모션 중이면 재입력 차단
	if (HasStateTag(FVremGameplayTags::State_Combat_MeleeAttacking))
	{
		return;
	}

	if (IsValid(EquipmentComponent) == false) return;

	const EEquipmentSlotType OnHandType = EquipmentComponent->GetOnHandSlotType();
	if (OnHandType == EEquipmentSlotType::Melee)
	{
		// 이미 근접이 손에 있음 → 공격
		AVremEquipmentActor* OnHandActor = EquipmentComponent->GetCurrentEquipmentActor();
		if (IsValid(OnHandActor) == false) 
		{
			return;
		}

		UVremMeleeComponent* Melee = OnHandActor->FindComponentByClass<UVremMeleeComponent>();
		if (IsValid(Melee))
		{
			Melee->TryMeleeAttack();
		}
	}
	else if (OnHandType == EEquipmentSlotType::Ranged)
	{
		// 원거리가 손에 있음 → Holstered(근접)로 스왑
		const int32 HolsteredIndex = EquipmentComponent->GetHolsteredSlotIndex();
		if (HolsteredIndex != INDEX_NONE)
		{
			RequestSetCurrentWeapon(HolsteredIndex, EEquipmentState::Holstered);
		}
	}
}

void AVremCharacter::RequestSetCurrentWeapon(int32 InSlotIndex, EEquipmentState PrevOnHandDest)
{
	if (HasAuthority())
	{
		EquipmentComponent->SetCurrentWeapon(InSlotIndex, PrevOnHandDest);
	}
	else
	{
		EquipmentComponent->ServerSetCurrentWeapon(InSlotIndex, PrevOnHandDest);
	}
}

void AVremCharacter::UpdateMovementStateTags()
{
	// Moving
	const bool bIsMoving = GetVelocity().SizeSquared2D() > FMath::Square(50.f);
	if (bIsMoving)
	{
		AddStateTag(FVremGameplayTags::State_Movement_Moving);
	}
	else
	{
		RemoveStateTag(FVremGameplayTags::State_Movement_Moving);
	}

	// InAir
	const bool bIsInAir = GetCharacterMovement()->IsFalling();
	if (bIsInAir)
	{
		AddStateTag(FVremGameplayTags::State_Movement_InAir);
	}
	else
	{
		RemoveStateTag(FVremGameplayTags::State_Movement_InAir);
	}
}
