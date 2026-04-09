// Fill out your copyright notice in the Description page of Project Settings.


#include "VremCharacter.h"

#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "VremPawnData.h"
#include "Camera/CameraComponent.h"
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
#include "Vrem/Animation/VremAnimInstance.h"

// temp
#include "Components/ArrowComponent.h"

static TAutoConsoleVariable<int32> CVarDebugCharacterInput(
	TEXT("vrem.DebugCharacterInput"),
	0,
	TEXT("Enable Character Input Debug\n")
	TEXT("0: Off\n")
	TEXT("1: On"),
	ECVF_Cheat);

static TAutoConsoleVariable<int32> CVarDebugCharacterShooting(
	TEXT("vrem.DebugCharacterShooting"),
	0,
	TEXT("Enable Character Shooting Debug\n")
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
	// temp code
	{
		Muzzle_Temp = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
		Muzzle_Temp->SetupAttachment(GetMesh());
		Muzzle_Temp->SetRelativeLocation(FVector(0.f, 0.f, 148.f));
		Muzzle_Temp->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
	}
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

	if (IsValid(InventoryComponent))
	{
		InventoryComponent->InitializeFromOwner();
	}

	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->InitializeFromOwner();
	}
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
	
	if (IsValid(EquipmentComponent) && GetNetMode() != NM_DedicatedServer)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::BeginPlay Event Binded"));
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
	
	UE_LOG(LogVremInput, Warning, TEXT("AVremCharacter::Attack_Temp NetRole : [%s]"), *GetNetRoleString(this));

	const bool bShowDebug = CVarDebugCharacterShooting.GetValueOnGameThread() > 0;

	if (bIsADS)
	{
		// range attack
		
		FVector WorldLocation;
		FRotator WorldDirection;
		GetController()->GetPlayerViewPoint(WorldLocation, WorldDirection);

		FVector ViewpointLinetraceEnd = WorldLocation + WorldDirection.Vector() * 10000.f;

		// linetrace from viewpoint
		FHitResult ViewHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		bool bHitFromViewpoint = GetWorld()->LineTraceSingleByChannel(
			ViewHit,
			WorldLocation,
			ViewpointLinetraceEnd,
			ECC_Visibility,
			Params
		);

		FVector ViewportLinetraceEndPoint = bHitFromViewpoint ? ViewHit.Location : ViewpointLinetraceEnd;

		if (bShowDebug)
		{ 
			DrawDebugLine(GetWorld(), WorldLocation, ViewportLinetraceEndPoint, bHitFromViewpoint ? FColor::Green : FColor::Red, false, 1.f, 0, 1.f);
		}

		// linetrace from muzzle
		FHitResult MuzzleHit;
		FVector MuzzleLocation = Muzzle_Temp->GetComponentLocation();
		FVector ShootDirection = (ViewportLinetraceEndPoint - MuzzleLocation).GetSafeNormal();
		FVector MuzzleEnd = MuzzleLocation + ShootDirection * 10000.f;

		bool bHitFromMuzzle = GetWorld()->LineTraceSingleByChannel(
			MuzzleHit,
			MuzzleLocation,
			MuzzleEnd,
			ECC_Visibility,
			Params
		);

		if (bHitFromMuzzle)
		{
			if (bShowDebug)
			{ 
				DrawDebugLine(GetWorld(), MuzzleLocation, MuzzleHit.Location, FColor::Green, false, 1.f, 0, 1.f);
				DrawDebugPoint(GetWorld(), MuzzleHit.Location, 10.f, FColor::Red, false, 1.f);
			}
		}
		else
		{
			if (bShowDebug)
			{ 
				DrawDebugLine(GetWorld(), MuzzleLocation, MuzzleEnd, FColor::Red, false, 1.f, 0, 1.f);
			}
		}
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

void AVremCharacter::OnEquipmentActorAttached(const UVremEquipmentDefinition* EquipmentDefinition)
{
	if (IsValid(EquipmentDefinition) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnEquipmentActorAttached EquipmentDefinition is nullptr"));
		return;
	}

	if (EquipmentDefinition->AnimLayerClass == nullptr)
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

	UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnEquipmentActorAttached LinkAnimLayer : [%s]"), *EquipmentDefinition->AnimLayerClass->GetName());
	AnimInstance->SetWeaponAnimLayer(EquipmentDefinition->AnimLayerClass);
}

void AVremCharacter::OnEquipmentActorDetached(const UVremEquipmentDefinition* EquipmentDefinition)
{
	if (IsValid(EquipmentDefinition) == false)
	{
		UE_LOG(LogVremEquipment, Warning, TEXT("AVremCharacter::OnEquipmentActorDetached EquipmentDefinition is nullptr"));
		return;
	}

	if (EquipmentDefinition->AnimLayerClass == nullptr)
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

	if (AnimInstance->GetCurrentLayer() != EquipmentDefinition->AnimLayerClass)
	{
		AnimInstance->SetWeaponAnimLayer(nullptr);
	}
}
