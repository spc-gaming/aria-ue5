// Copyright (c) SPC Gaming. All rights reserved.

#include "Game/AriaGameMode.h"
#include "Character/AriaCharacter.h"
#include "Game/AriaPlayerController.h"

AAriaGameMode::AAriaGameMode()
{
	DefaultPawnClass = AAriaCharacter::StaticClass();
	PlayerControllerClass = AAriaPlayerController::StaticClass();
}
