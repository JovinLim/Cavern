// Copyright Epic Games, Inc. All Rights Reserved.

#include "CavernGameMode.h"
#include "CavernCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACavernGameMode::ACavernGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
