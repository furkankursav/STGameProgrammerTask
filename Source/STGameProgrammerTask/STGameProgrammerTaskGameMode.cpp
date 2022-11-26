// Copyright Epic Games, Inc. All Rights Reserved.

#include "STGameProgrammerTaskGameMode.h"
#include "STGameProgrammerTaskHUD.h"
#include "STGameProgrammerTaskCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASTGameProgrammerTaskGameMode::ASTGameProgrammerTaskGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ASTGameProgrammerTaskHUD::StaticClass();
}
