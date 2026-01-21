// Copyright Epic Games, Inc. All Rights Reserved.

#include "MiniHuntGameMode.h"
#include "MiniHuntCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMiniHuntGameMode::AMiniHuntGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
