// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
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

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * DeltaTime;

	ApplyRotation(DeltaTime);

	UpdateLocationFromVelocity(DeltaTime);

	DrawDebugString(GetWorld(), FVector(0, 0, 100.f), GetEnumText(Role), this, FColor::White, DeltaTime);

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
	Server_MoveForward(Value);

}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
	Server_MoveRight(Value);

}

// Implementation of the Server_MoveForward function. Suffix: '_Implementation'
void AGoKart::Server_MoveForward_Implementation(float Value)
{
	Throttle = Value;

}

// Server validation of the Server_MoveForward function. Suffix: '_Validate'
bool AGoKart::Server_MoveForward_Validate(float Value)
{
	/**
	 * This will check that our value is between -1 and 1.
	 * If the value is not between -1 and 1, then this function will return false.
	 * If any validation check returns false, the client will be disconnected.
	 *
	 */
	return FMath::Abs(Value) <= 1;

}

void AGoKart::Server_MoveRight_Implementation(float Value)
{
	SteeringThrow = Value;

}

bool AGoKart::Server_MoveRight_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;

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

void AGoKart::ApplyRotation(float DeltaTime)
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
