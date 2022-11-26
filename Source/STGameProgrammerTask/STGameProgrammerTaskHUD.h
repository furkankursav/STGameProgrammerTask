// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "STGameProgrammerTaskHUD.generated.h"

UCLASS()
class ASTGameProgrammerTaskHUD : public AHUD
{
	GENERATED_BODY()

public:
	ASTGameProgrammerTaskHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

