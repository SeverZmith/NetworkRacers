// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	// We want bReplicateMovement off so that we can do our own interpolation and handle movement replication with the GoKartMovementReplicator component.
	bReplicateMovement = false;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComponent"));
	MovementReplicator = CreateDefaultSubobject<UGoKartMovementReplicator>(TEXT("MovementReplicator"));

}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		/**
		 * How frequently the authoritative actor should be considered for replication.
		 * You want this number to be as low as possible to prevent excessive CPU or bandwidth usage.
		 * Typical values for this variable are as follows:
		 *		- 10.f (update every 0.1 seconds) for important or unpredictable actors such as player-controlled actors.
		 *		- 5.f (update every 0.2 seconds) for slower-moving or more predictable actors such as AI-controlled actors.
		 *		- 2.f (update every 0.5 seconds) for background or distant actors.
		 *
		 */
		NetUpdateFrequency = 1.f; // In this example, this value is set very low to help visualize our interpolation of the simulated-proxy actor's position.

	}
	
}

FString GetEnumText(ENetRole Role)
{
	// Used to visualize remote roles of actors.
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	default:
		return "ERROR";
	}

}

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawDebugString(GetWorld(), FVector(0.f, 0.f, 100.f), GetEnumText(Role), this, FColor::White, DeltaTime);

}

void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);

}

void AGoKart::MoveForward(float Value)
{
	if (MovementComponent == nullptr) return;

	// Send MoveForward input to movement component.
	MovementComponent->SetThrottle(Value);

}

void AGoKart::MoveRight(float Value)
{
	if (MovementComponent == nullptr) return;

	// Send MoveRight input to movement component.
	MovementComponent->SetSteeringThrow(Value);

}
