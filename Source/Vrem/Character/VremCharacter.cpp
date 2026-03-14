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
#include "Vrem/Input/VremInputConfig.h"

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
			GameModeDefinitionLoadedHandle = GameModeManager->OnGameModeDefinitionLoadedDelegate.AddUObject(this, &ThisClass::OnInputConfigLoaded);
			
			if (GameModeManager->IsGameModeDefinitionLoaded())
			{
				OnInputConfigLoaded(GameModeManager->GetGameModeDefinition());
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

	// TODO: 이거 지우고 카메라 관리 시스템 별도 구현
	{
		if (IsValid(FollowCamera))
		{
			float TargetFOV = bIsADS ? 70.f : 90.f;
			FollowCamera->SetFieldOfView(
				FMath::FInterpTo(
					FollowCamera->FieldOfView,
					TargetFOV,
					GetWorld()->GetDeltaSeconds(),
					10.f
				));
		}

		if (IsValid(SpringArm))
		{
			float TargetArm = bIsADS ? 120.f : 300.f;

			SpringArm->TargetArmLength =
				FMath::FInterpTo(
					SpringArm->TargetArmLength,
					TargetArm,
					GetWorld()->GetDeltaSeconds(),
					10.f
				);
		}
	}
}

inline void AVremCharacter::OnInputConfigLoaded(const UVremGameModeDefinition* InGameModeDefinition)
{
	CurrentInputConfig = InGameModeDefinition->PawnData->InputConfig;	// TODO: 이부분은 개선 여지가 있을 것 같다, Character가 GameModeDefinition 전체를 알 필요는 없다.
	
	if (CurrentInputConfig.IsValid() == false)
	{
		UE_LOG(LogVremInput, Warning, TEXT("AVremCharacter::OnInputConfigLoaded InputConfig Is Invalid! NetRole : [%s]"), *GetNetRoleString(this));
		return;
	}

	if (IsValid(InputComponent) == false)
	{
		UE_LOG(LogVremInput, Warning, TEXT("AVremCharacter::OnInputConfigLoaded InputComponent Is Invalid! NetRole : [%s]"), *GetNetRoleString(this));
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

// Called to bind functionality to input
void AVremCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
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
}

void AVremCharacter::ToggleADS(const FInputActionValue& Value)
{
	bIsADS = !bIsADS;
}
