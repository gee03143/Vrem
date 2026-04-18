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
	if (CVarDebugCharacterInput.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(
			VremDebugKey::DebugKey_InputWeaponPrimary,
			1.f,
			FColor::Yellow,
			TEXT("Attack"));
	}
	
	if (bIsADS)
	{
		// range attack
		AVremEquipmentActor* WeaponActor = EquipmentComponent->GetCurrentEquipmentActor();
		if (IsValid(WeaponActor) == false)
		{
			UE_LOG(LogVremWeapon, Warning, TEXT("AVremCharacter::Attack_Temp WeaponActor not valid"));
			return;
		}

		UVremWeaponComponent* WeaponComp = WeaponActor->FindComponentByClass<UVremWeaponComponent>();
		if (IsValid(WeaponComp) == false)
		{
			UE_LOG(LogVremWeapon, Warning, TEXT("AVremCharacter::Attack_Temp WeaponComp not valid"));
			return;
		}

		WeaponComp->Fire();
	}
	else
	{
		// melee attack
	}
}

void AVremCharacter::ToggleADS(const FInputActionValue& Value)
{
	bIsADS = !bIsADS;
	if (bIsADS)
	{
		CameraSystem->SetTargetCameraMode(ADSCameraMode);
	}
	else
	{
		CameraSystem->SetTargetCameraMode(DefaultCameraMode);
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
			EIC->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AVremCharacter::Attack_Temp);
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

void AVremCharacter::OnItemInstanceCreated(UVremItemInstance* ItemInstance)
{
	// server only
	check(HasAuthority());

	UE_LOG(LogVremInventory, Warning, TEXT("AVremCharacter::OnItemInstanceCreated"));

	const bool bHasEquipped = EquipmentComponent->GetEquipmentItemNum() > 0;

	UItemFragment_Equipment* EquipmentFragment = ItemInstance->FindFragment<UItemFragment_Equipment>();
	if (EquipmentFragment)
	{
		EquipmentComponent->TryEquipItem(
			EquipmentFragment->GetEquipmentDefinition(),
			EquipmentComponent->GetEquipmentItemNum() + 1
		);
	}

	if (bHasEquipped == false)
	{
		EquipmentComponent->SetCurrentWeapon(1);
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
	if (InAnimLayerClass == nullptr)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnEquipmentActorAttached AnimLayerClass is nullptr"));
		return;
	}

	UVremAnimInstance* AnimInstance = Cast<UVremAnimInstance>(GetMesh()->GetAnimInstance());
	if (IsValid(AnimInstance) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnEquipmentActorAttached AnimInstance is nullptr"));
		return;
	}

	AnimInstance->SetWeaponAnimLayer(InAnimLayerClass);
}

void AVremCharacter::OnEquipmentActorDetached(const TSubclassOf<UAnimInstance> InAnimLayerClass)
{	
	UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnEquipmentActorDetached"));
	if (InAnimLayerClass == nullptr)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnEquipmentActorDetached AnimLayerClass is nullptr"));
		return;
	}

	UVremAnimInstance* AnimInstance = Cast<UVremAnimInstance>(GetMesh()->GetAnimInstance());
	if (IsValid(AnimInstance) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnEquipmentActorDetached AnimInstance is nullptr"));
		return;
	}

	if (AnimInstance->GetCurrentLayer() == InAnimLayerClass)
	{
		AnimInstance->SetWeaponAnimLayer(nullptr);
	}
}
