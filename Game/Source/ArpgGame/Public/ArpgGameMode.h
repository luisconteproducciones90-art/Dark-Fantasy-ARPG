#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArpgCore/Save/SaveGame.h"
#include "ArpgGameMode.generated.h"

class AArpgDungeonActor;
class AArpgMonster;
class AArpgPlayerCharacter;

/**
 * Dirige la partida: genera la mazmorra, la puebla, reparte experiencia y
 * botín, gestiona muerte y reaparición, baja de nivel al caer el jefe y guarda.
 * No necesita ningún mapa ni asset propio: lo construye todo al empezar.
 */
UCLASS()
class ARPGGAME_API AArpgGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArpgGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void NotifyMonsterDied(AArpgMonster* Monster);
	void NotifyPlayerDied(AArpgPlayerCharacter* Player);

	void SpawnItemPickup(const arpg::ItemInstance& Item, const FVector& Location);

	/** Reinicia la mazmorra según el estado actual del personaje (p. ej. tras crear uno nuevo). */
	void RestartFromState();

	/** Guarda el personaje en disco. Devuelve false si no se pudo escribir. */
	bool SaveGame();

	AArpgDungeonActor* GetDungeon() const { return Dungeon; }
	int32 GetDepth() const { return Depth; }

private:
	void StartLevel(int32 NewDepth, arpg::uint64 WorldSeed);
	void AdvanceLevel();
	void RespawnPlayer();
	void DropLoot(const arpg::LootDrop& Drop, const FVector& Location);
	FVector ScatterAround(const FVector& Location) const;
	AArpgPlayerCharacter* GetPlayerCharacter() const;
	void Announce(const FString& Text) const;
	static FString GetSavePath();

	UPROPERTY()
	TObjectPtr<AArpgDungeonActor> Dungeon;

	arpg::CharacterSave LoadedSave;
	bool bHasLoadedSave = false;
	int32 Depth = 1;
	arpg::uint64 CurrentWorldSeed = 1;
	FTimerHandle AdvanceTimer;
	FTimerHandle RespawnTimer;
};
