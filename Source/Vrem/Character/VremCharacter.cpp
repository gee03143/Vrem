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
#include "Vrem/Input/VremInputConfig.h"
#include "Vrem/Equipment/VremEquipmentComponent.h"
#include "Vrem/Equipment/VremEquipmentActor.h"
#include "Vrem/Equipment/Weapon/VremWeaponComponent.h"

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
}

void AVremCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GameModeDefinitionLoadedHandle.IsValid())
	{
		GameModeDefinitionLoadedHandle.Reset();
	}
	
	Super::EndPlay(EndPlayReason);
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
		else if (CameraSystem->HasTargetCameraMode())
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
	OnAttackPressed();
}

void AVremCharacter::StopAttack_Temp(const FInputActionValue& Value)
{
	OnAttackReleased();
}

void AVremCharacter::ToggleADS(const FInputActionValue& Value)
{
	OnToggleADSPressed();
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
	UVremEquipmentComponent* EquipmentComponent = FindComponentByClass<UVremEquipmentComponent>();
	if (IsValid(EquipmentComponent) == false)
	{
		return 0.f;
	}

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
	EnterMeleeAttackingState();
}

void AVremCharacter::OnMeleeAttackFinished()
{
	ExitMeleeAttackingState();
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

void AVremCharacter::EnterMeleeAttackingState()
{
	AddStateTag(FVremGameplayTags::State_Combat_MeleeAttacking);
}

void AVremCharacter::ExitMeleeAttackingState()
{
	RemoveStateTag(FVremGameplayTags::State_Combat_MeleeAttacking);
}

void AVremCharacter::EnterADSState()
{
	AddStateTag(FVremGameplayTags::State_Aiming_ADS);
}

void AVremCharacter::ExitADSState()
{
	RemoveStateTag(FVremGameplayTags::State_Aiming_ADS);
}

void AVremCharacter::EnterScopedState()
{
	AddStateTag(FVremGameplayTags::State_Aiming_Scoped);
}

void AVremCharacter::ExitScopedState()
{
	RemoveStateTag(FVremGameplayTags::State_Aiming_Scoped);
}

void AVremCharacter::AddStateTag(const FGameplayTag& Tag)
{
	if (StateTags.HasTag(Tag) == false)
	{
		StateTags.AddTag(Tag);
		OnStateTagAdded.Broadcast(Tag);
	}
}

void AVremCharacter::RemoveStateTag(const FGameplayTag& Tag)
{
	if (StateTags.HasTag(Tag))
	{
		StateTags.RemoveTag(Tag);
		OnStateTagRemoved.Broadcast(Tag);
	}
}