// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NetworkRacersGameMode.h"
#include "NetworkRacersPawn.h"
#include "NetworkRacersHud.h"

ANetworkRacersGameMode::ANetworkRacersGameMode()
{
	DefaultPawnClass = ANetworkRacersPawn::StaticClass();
	HUDClass = ANetworkRacersHud::StaticClass();
}
