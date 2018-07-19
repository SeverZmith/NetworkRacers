// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartMovementComponent.h"
#include "GoKart.generated.h"


USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove PrevMove;

};

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
	void ClearAcknowledgedMoves(FGoKartMove PrevMove);

	void MoveForward(float Value);

	void MoveRight(float Value);

	/**
	 * To replicate movement over a server we begin by applying Server, Reliable, WithValidation as properties in the UFUNCTION().
	 * Prefix function name with 'Server_'. This is the new name that we will bind our input to in the cpp.
	 * Definitions of these functions will be split between 2 functions with different suffixes: '_Implementation' & '_Validate' (see cpp).
	 *
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	// Function called when ReplicatedTransform becomes replicated.
	UFUNCTION()
	void OnRep_ServerState();

	TArray<FGoKartMove> UnacknowledgedMoves;

	UPROPERTY(EditAnywhere)
	UGoKartMovementComponent* MovementComponent;

};
