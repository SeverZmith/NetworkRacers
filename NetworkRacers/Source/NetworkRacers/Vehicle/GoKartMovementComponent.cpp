// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"


UGoKartMovementComponent::UGoKartMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If the player is an autonomous or simulated proxy, then create and simulate a move.
	if (GetOwnerRole() == ROLE_AutonomousProxy || GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// Track previous move for comparisons.
		PrevMove = CreateMove(DeltaTime);
		SimulateMove(PrevMove);

	}

}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;

	AGameStateBase* GameStateBase = GetWorld()->GetGameState();
	Move.TimeStamp = GameStateBase->GetServerWorldTimeSeconds(); // Get the server time

	return Move;

}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);

}

FVector UGoKartMovementComponent::GetAirResistance()
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

FVector UGoKartMovementComponent::GetRollingResistance()
{
	/**
	* GetGravityZ() gets the force of gravity in the Z axis.
	* Unreal gets the value in relation to cm, and automatically applies a (-) sign to denote a downward force on the Z axis.
	* ...therefore
	* We divide by 100 to get the value in relation to m, and apply another (-) sign to make the value positive so we can use it.
	*
	* NormalForce is the force applied to counteract gravity. nF = M * G (NormalForce = Mass * Acceleration of gravity)
	*
	*/
	float AccelerationDueToGravity = -(GetWorld()->GetGravityZ()) / 100;
	float NormalForce = Mass * AccelerationDueToGravity;
	return -(Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce);

}

void UGoKartMovementComponent::ApplyRotation(float DeltaTime, float SteeringThrow)
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
	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = (DeltaLocation / MinTurningRadius) * SteeringThrow;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	GetOwner()->AddActorWorldRotation(RotationDelta);

}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	/**
	 * dx = v * dt
	 * Change in location = Velocity * Change in time
	 *
	 * Our Velocity is calculated in meters per second (m/s).
	 * We multiply by 100 because AddActorWorldOffset expects the Translation to be in centimeters.
	 *
	 */
	FVector Translation = Velocity * 100 * DeltaTime;

	FHitResult Hit;
	GetOwner()->AddActorWorldOffset(Translation, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;

	}

}
