// Fill out your copyright notice in the Description page of Project Settings.


#include "VremDodgeComponent.h"
#include "Vrem/VremLogChannels.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/Character.h"

UVremDodgeComponent::UVremDodgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UVremDodgeComponent::RequestDodge(EDodgeDirection Direction)
{
	if (CanDodge() == false)
	{
		return;
	}

	ExecuteDodge(Direction);
}

bool UVremDodgeComponent::CanDodge() const
{
	if (bIsDodging || bIsOnCooldown)
	{
		return false;
	}

	if (IsValid(DodgeProfile) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::CanDodge DodgeProfile is invalid!"));
		return false;
	}

	return true;
}

void UVremDodgeComponent::ExecuteDodge(EDodgeDirection Direction)
{
	if (IsValid(DodgeProfile) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::ExecuteDodge DodgeProfile is invalid!"));
		return;
	}

	const FDodgeSequence& Sequence = DodgeProfile->GetDodgeSequence(Direction);

	ServerDodge(Direction);

	AActor* Owner = GetOwner();
	if (IsValid(Owner) && Owner->GetLocalRole() == ROLE_AutonomousProxy)
	{
		PlayMontageLocally(Sequence);
		ApplyDodgeForce(Sequence, Direction);
	}

	bIsDodging = true;
	LastDodgeCooldown = Sequence.DodgeCooldown;
	
	OnDodgeStarted.Broadcast(Direction);

	FTimerDelegate DodgeDurationDelegate;
	DodgeDurationDelegate.BindUObject(this, &UVremDodgeComponent::OnDodgeDurationFinished);
	GetWorld()->GetTimerManager().SetTimer(DodgeDurationTimer, DodgeDurationDelegate, Sequence.DodgeDuration, false);
}

void UVremDodgeComponent::ServerDodge_Implementation(EDodgeDirection Direction)
{
	if (IsValid(DodgeProfile) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::ServerDodge_Implementation DodgeProfile is invalid!"));
		return;
	}

	const FDodgeSequence& Sequence = DodgeProfile->GetDodgeSequence(Direction);

	ApplyDodgeForce(Sequence, Direction);
	MulticastOnDodge(Direction);
}

void UVremDodgeComponent::MulticastOnDodge_Implementation(EDodgeDirection Direction)
{
	AActor* Owner = GetOwner();
	if (IsValid(Owner) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::MulticastOnDodge_Implementation Owner is invalid!"));
		return;
	}

	if (Owner->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// autonomousproxy는 이미 몽타주 재생함
		return;
	}

	if (IsValid(DodgeProfile) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::MulticastOnDodge_Implementation DodgeProfile is invalid!"));
		return;
	}

	const FDodgeSequence& Sequence = DodgeProfile->GetDodgeSequence(Direction);
	PlayMontageLocally(Sequence);
}

void UVremDodgeComponent::PlayMontageLocally(const FDodgeSequence& Sequence)
{
	if (IsValid(Sequence.DodgeMontage) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::PlayMontageLocally DodgeMontage is invalid!"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (IsValid(Character))
	{
		Character->PlayAnimMontage(Sequence.DodgeMontage);
	}
	else
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::PlayMontageLocally Owner is nullptr or not charactger"));
	}
}

void UVremDodgeComponent::ApplyDodgeForce(const FDodgeSequence& Sequence, EDodgeDirection Direction)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (IsValid(Character) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::ApplyDodgeForce Owner is nullptr or not charactger"));
		return;
	}

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (IsValid(Movement) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::ApplyDodgeForce movementcomponent is invalid"));
		return;
	}

	const FVector WorldDir = GetWorldDirection(Direction);

	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = TEXT("Dodge");
	Force->AccumulateMode = ERootMotionAccumulateMode::Override;
	Force->Priority = 5;
	Force->Force = WorldDir * Sequence.ForceStrength;
	Force->Duration = Sequence.ForceDuration;
	Force->StrengthOverTime = nullptr;
	Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
	Force->FinishVelocityParams.ClampVelocity = 0.f;

	Movement->ApplyRootMotionSource(Force);
}

FVector UVremDodgeComponent::GetWorldDirection(EDodgeDirection Direction) const
{
	const AActor* Owner = GetOwner();
	if (IsValid(Owner) == false)
	{
		UE_LOG(LogVremDodge, Warning, TEXT("UVremDodgeComponent::GetWorldDirection Owner is invalid, ZeroVector returned"));
		return FVector::ZeroVector;
	}

	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Right = Owner->GetActorRightVector();

	switch (Direction)
	{
	case EDodgeDirection::Forward:  
		return Forward;
	case EDodgeDirection::Backward: 
		return -Forward;
	case EDodgeDirection::Left:     
		return -Right;
	case EDodgeDirection::Right:    
		return Right;
	default:                         
		return -Forward;
	}
}

void UVremDodgeComponent::OnDodgeDurationFinished()
{
	bIsDodging = false;

	OnDodgeFinished.Broadcast();

	if (LastDodgeCooldown > 0.f)
	{
		bIsOnCooldown = true;
		FTimerDelegate CooldownDelegate;
		CooldownDelegate.BindUObject(this, &UVremDodgeComponent::OnCooldownFinished);
		GetWorld()->GetTimerManager().SetTimer(
			CooldownTimer, CooldownDelegate, LastDodgeCooldown, false);
	}
}

void UVremDodgeComponent::OnCooldownFinished()
{
	bIsOnCooldown = false;
}

