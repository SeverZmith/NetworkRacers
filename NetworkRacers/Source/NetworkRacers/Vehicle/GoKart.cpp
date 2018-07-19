// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"


// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;

	}
	
}

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKart, ServerState);

}

FString GetEnumText(ENetRole Role)
{
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

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If we are a client.
	if (Role == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		SimulateMove(Move);

		UnacknowledgedMoves.Add(Move);

		Server_SendMove(Move);

	}

	// If we are the server and controlling the pawn.
	if (Role == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		Server_SendMove(Move);

	}

	// If we are being observed by other clients.
	if (Role == ROLE_SimulatedProxy)
	{
		SimulateMove(ServerState.PrevMove);

	}

	DrawDebugString(GetWorld(), FVector(0, 0, 100.f), GetEnumText(Role), this, FColor::White, DeltaTime);

}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgedMoves(ServerState.PrevMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		SimulateMove(Move);

	}

}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);

}

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;

}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;

}

// Implementation of the Server_MoveForward function. Suffix: '_Implementation'
void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	SimulateMove(Move);

	ServerState.PrevMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;

}

// Server validation of the Server_MoveForward function. Suffix: '_Validate'
bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	/**
	 * This will check that our value is between -1 and 1.
	 * If the value is not between -1 and 1, then this function will return false.
	 * If any validation check returns false, the client will be disconnected.
	 *
	 */
	return true; // TODO better validation

}

void AGoKart::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);

}

FGoKartMove AGoKart::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;

	AGameStateBase* GameStateBase = GetWorld()->GetGameState();
	Move.TimeStamp = GameStateBase->GetServerWorldTimeSeconds();

	return Move;

}

void AGoKart::ClearAcknowledgedMoves(FGoKartMove PrevMove)
{
	TArray<FGoKartMove> FreshMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.TimeStamp > PrevMove.TimeStamp)
		{
			FreshMoves.Add(Move);

		}

	}

	UnacknowledgedMoves = FreshMoves;

}

FVector AGoKart::GetAirResistance()
{
	/**
	 * GetSafeNormal() is the direction of the vector that the GoKart is travelling.
	 * SizeSquared() is the speed of the vector, squared.
	 *
	 * -(Direction * Speed^2 * Drag)
	 *
	 */
	return -(Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient);

}

FVector AGoKart::GetRollingResistance()
{
	/**
	 * GetGravityZ() gets the force of gravity.
	 * Unreal gets the value in relation to cm, and automatically applies a (-) sign to denote a downward force on the Z axis.
	 * ...therefore
	 * We divide by 100 to get the value in relation to m, and apply another (-) sign to make the value positive so we can use it.
	 *
	 * NormalForce is the force applied to counteract gravity. nF = M * G
	 *
	 */
	float AccelerationDueToGravity = -(GetWorld()->GetGravityZ()) / 100;
	float NormalForce = Mass * AccelerationDueToGravity;
	return -(Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce);

}

void AGoKart::ApplyRotation(float DeltaTime, float SteeringThrow)
{
	/**
	 * Calculate Steering Turning
	 * dx = dTheta * r
	 * Change in location along the turning circle in 1 second (dx).
	 * Angle calculated from dx in relation to the turning circle (dTheta).
	 * Radius of the turning circle (r).
	 *
	 * DotProduct(A, B) returns a float that represents an angular relationship between A and B.
	 * This relationship projects the length of vector B in the direction of vector A. 
	 *
	 * dx is DeltaLocation
	 * r is MinTurningRadius
	 * dTheta is RotationAngle
	 *
	 */
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = (DeltaLocation / MinTurningRadius) * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta);

}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime;

	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;

	}

}
