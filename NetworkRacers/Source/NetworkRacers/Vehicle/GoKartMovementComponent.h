// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"


USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float TimeStamp;

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NETWORKRACERS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SimulateMove(const FGoKartMove& Move);


	FGoKartMove GetPrevMove() { return PrevMove; };

	FVector GetVelocity() { return Velocity; };
	void SetVelocity(FVector Val) { Velocity = Val; };

	void SetThrottle(float Val) { Throttle = Val; };
	void SetSteeringThrow(float Val) { SteeringThrow = Val; };

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	FGoKartMove CreateMove(float DeltaTime);

	FVector GetAirResistance();

	FVector GetRollingResistance();

	void ApplyRotation(float DeltaTime, float SteeringThrow);

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

	FVector Velocity;

	float Throttle;

	float SteeringThrow;

	FGoKartMove PrevMove;
	
};
