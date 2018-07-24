// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"
#include "UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"


UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);

}

void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
	
}

void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementComponent == nullptr) return;

	FGoKartMove PrevMove = MovementComponent->GetPrevMove();
	// If we are a client.
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(PrevMove);

		Server_SendMove(PrevMove);

	}

	// If we are the server and controlling the pawn.
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(PrevMove);

	}

	// If we are being observed by other clients.
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);

	}

}

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{
	// Update player's move, location, and speed.
	ServerState.PrevMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();

}

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;
	if (MovementComponent == nullptr) return;

	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;

	// Crete spline and interpolate variables along it.
	FHermiteCubicSpline Spline = CreateSpline();

	InterpolateLocation(Spline, LerpRatio);

	InterpolateVelocity(Spline, LerpRatio);

	InterpolateRotation(LerpRatio);

}

FHermiteCubicSpline UGoKartMovementReplicator::CreateSpline()
{
	// Update spline variables.
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return Spline;

}

void UGoKartMovementReplicator::InterpolateLocation(const FHermiteCubicSpline &Spline, float LerpRatio)
{
	// Move the actor to the interpolated location.
	FVector NextLocation = Spline.InterpolateLocation(LerpRatio);
	if (MeshOffsetRoot != nullptr)
	{
		MeshOffsetRoot->SetWorldLocation(NextLocation);

	}

}

void UGoKartMovementReplicator::InterpolateVelocity(const FHermiteCubicSpline &Spline, float LerpRatio)
{
	// Interpolate and set the velocity.
	FVector NextDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NextVelocity = NextDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(NextVelocity);

}

void UGoKartMovementReplicator::InterpolateRotation(float LerpRatio)
{
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();

	// Interpolate using Slerp for rotation so the bounds become -180 and 180 (spherical)
	FQuat NextRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	if (MeshOffsetRoot != nullptr)
	{
		MeshOffsetRoot->SetWorldRotation(NextRotation);

	}
}

float UGoKartMovementReplicator::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdates * 100;

}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Variables to replicate on server.
	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);

}

void UGoKartMovementReplicator::OnRep_ServerState()
{
	// When ServerState is replicated...
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}

}

// As client.
void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	// Clear tracked moved.
	ClearAcknowledgedMoves(ServerState.PrevMove);

	// Iterate through UnacknowledgedMoves and simulate move.
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);

	}

}

// As client to other clients.
void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	// Update client variables.
	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	if (MeshOffsetRoot != nullptr)
	{
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());

	}
	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);

}

void UGoKartMovementReplicator::ClearAcknowledgedMoves(FGoKartMove PrevMove)
{
	TArray<FGoKartMove> FreshMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		// If next move happens after prev move update FreshMoves.
		if (Move.TimeStamp > PrevMove.TimeStamp)
		{
			FreshMoves.Add(Move);

		}

	}

	// Track moves before updating.
	UnacknowledgedMoves = FreshMoves;

}

// Implementation of the Server_MoveForward function. Suffix: '_Implementation'
void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (MovementComponent == nullptr) return;

	// Simulate move on server.
	ClientSimulatedTime += Move.DeltaTime;
	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);

}

// Server validation of the Server_MoveForward function. Suffix: '_Validate'
bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)
{
	/**
	* '_Validate' methods are where anti-cheat logic is placed.
	*
	* Here we check that our client is making valid moves and not manipulating time variables.
	*
	*/
	float ProposedTime = ClientSimulatedTime + Move.DeltaTime;
	bool ClientNotRunningAhead = ProposedTime < GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	if (!ClientNotRunningAhead)
	{
		UE_LOG(LogTemp, Error, TEXT("Client is running too fast."));
		return false;

	}
	if (!Move.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Received invalid move."));
		return false;

	}

	return true;

}
