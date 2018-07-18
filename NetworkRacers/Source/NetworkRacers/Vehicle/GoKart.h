// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

UCLASS()
class NETWORKRACERS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	FVector GetAirResistance();

	FVector GetRollingResistance();

	void MoveForward(float Value);

	void MoveRight(float Value);

	/**
	 * To replicate movement over a server we begin by applying Server, Reliable, WithValidation as properties in the UFUNCTION().
	 * Prefix function name with 'Server_'. This is the new name that we will bind our input to in the cpp.
	 * Definitions of these functions will be split between 2 functions with different suffixes: '_Implementation' & '_Validate' (see cpp).
	 *
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float Value);

	void ApplyRotation(float DeltaTime);

	void UpdateLocationFromVelocity(float DeltaTime);

	// Mass of GoKart (kg)
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	// Force applied to GoKart when throttle is fully engaged (N)
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	// Minimum radius of the car turning circle at full steering lock (m)
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10;

	/**
	 * Value that represents the aerodynamics of the GoKart. Higher value = higher drag.
	 * AirResistance = -Speed^2 * DragCoefficient
	 * ...therefore
	 * DragCoefficient = AirResistance / Speed^2
	 * which is...
	 * 16 = 10000 / 25^2
	 *
	 */
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16;

	/**
	* Value that represents the Rolling Resistance of the GoKart. Higher value = higher rolling resistance.
	* RollingResistance = RollingResistanceCoefficient * NormalForce
	*
	*/
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.015;

	UPROPERTY(Replicated)
	FVector Velocity;

	UPROPERTY(Replicated)
	float Throttle;
	
	UPROPERTY(Replicated)
	float SteeringThrow;

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedTransform)
	FTransform ReplicatedTransform;

	// Function called when ReplicatedTransform becomes replicated.
	UFUNCTION()
	void OnRep_ReplicatedTransform();

};
