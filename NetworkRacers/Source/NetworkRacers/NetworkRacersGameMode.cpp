// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NetworkRacersGameMode.h"
#include "NetworkRacersPawn.h"
#include "NetworkRacersHud.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"

ANetworkRacersGameMode::ANetworkRacersGameMode()
{
	DefaultPawnClass = ANetworkRacersPawn::StaticClass();
	HUDClass = ANetworkRacersHud::StaticClass();

}

APawn* ANetworkRacersGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Instigator;
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn; // Always spawn even if colliding
	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo);
	if (!ResultPawn)
	{
		UE_LOG(LogGameMode, Warning, TEXT("SpawnDefaultPawnAtTransform: Couldn't spawn Pawn of type %s at %s"), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
	}
	return ResultPawn;

}
