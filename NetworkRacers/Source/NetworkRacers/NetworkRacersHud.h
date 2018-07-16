// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/HUD.h"
#include "NetworkRacersHud.generated.h"


UCLASS(config = Game)
class ANetworkRacersHud : public AHUD
{
	GENERATED_BODY()

public:
	ANetworkRacersHud();

	/** Font used to render the vehicle info */
	UPROPERTY()
	UFont* HUDFont;

	// Begin AHUD interface
	virtual void DrawHUD() override;
	// End AHUD interface
};
