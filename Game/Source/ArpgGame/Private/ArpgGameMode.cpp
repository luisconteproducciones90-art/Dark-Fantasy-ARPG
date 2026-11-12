#include "ArpgGameMode.h"

#include "ArpgPlayerCharacter.h"
#include "ArpgPlayerController.h"

AArpgGameMode::AArpgGameMode()
{
	DefaultPawnClass = AArpgPlayerCharacter::StaticClass();
	PlayerControllerClass = AArpgPlayerController::StaticClass();
}
