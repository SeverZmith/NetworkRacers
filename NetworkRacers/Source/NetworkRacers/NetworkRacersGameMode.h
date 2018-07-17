// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/GameModeBase.h"
#include "NetworkRacersGameMode.generated.h"

UCLASS(minimalapi)
class ANetworkRacersGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANetworkRacersGameMode();

	// Overriding this function from AGameModeBase to adjust SpawnInfo.SpawnCollisionHandlingOverride.
	APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform);

};
