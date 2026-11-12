#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArpgGameMode.generated.h"

/** Modo de juego base: asigna el personaje y el controlador isométricos. */
UCLASS()
class ARPGGAME_API AArpgGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArpgGameMode();
};
