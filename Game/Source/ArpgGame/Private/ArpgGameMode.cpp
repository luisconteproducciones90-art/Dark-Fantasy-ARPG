#include "ArpgGameMode.h"

#include "ArpgCombatComponent.h"
#include "ArpgDungeonActor.h"
#include "ArpgHUD.h"
#include "ArpgLootPickup.h"
#include "ArpgMonster.h"
#include "ArpgPlayerCharacter.h"
#include "ArpgPlayerController.h"
#include "ArpgText.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

AArpgGameMode::AArpgGameMode()
{
	DefaultPawnClass = AArpgPlayerCharacter::StaticClass();
	PlayerControllerClass = AArpgPlayerController::StaticClass();
	HUDClass = AArpgHUD::StaticClass();
}

FString AArpgGameMode::GetSavePath()
{
	return FPaths::ProjectSavedDir() / TEXT("SaveGames") / TEXT("Personaje.vhsv");
}

void AArpgGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Partida guardada, si existe y es válida. Un archivo dañado no se toca: se empieza de cero
	// y la copia .bak sigue ahí.
	TArray<uint8> Bytes;
	if (FFileHelper::LoadFileToArray(Bytes, *GetSavePath(), FILEREAD_Silent))
	{
		const arpg::ESaveResult Result =
			arpg::DeserializeCharacter(Bytes.GetData(), static_cast<std::size_t>(Bytes.Num()), LoadedSave);
		bHasLoadedSave = Result == arpg::ESaveResult::Ok;
		if (!bHasLoadedSave)
		{
			UE_LOG(LogTemp, Warning, TEXT("Guardado no valido (codigo %d): se empieza una partida nueva."), static_cast<int32>(Result));
		}
	}

	// Luz de luna tenue y fría; la antorcha del personaje pone el contraste cálido.
	if (ADirectionalLight* Moon = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-55.f, 35.f, 0.f)))
	{
		if (ULightComponent* Light = Moon->GetLightComponent())
		{
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetIntensity(1.2f);
			Light->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));
		}
	}

	Dungeon = GetWorld()->SpawnActor<AArpgDungeonActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	const arpg::uint64 Seed = bHasLoadedSave ? LoadedSave.WorldSeed : static_cast<arpg::uint64>(ArpgGlobalRng().NextU32()) + 1;
	StartLevel(bHasLoadedSave ? LoadedSave.Depth : 1, Seed);
}

void AArpgGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	AArpgPlayerCharacter* Player = NewPlayer != nullptr ? Cast<AArpgPlayerCharacter>(NewPlayer->GetPawn()) : nullptr;
	if (Player == nullptr)
	{
		return;
	}
	if (bHasLoadedSave)
	{
		Player->ApplyLoadedState(LoadedSave);
	}
	else
	{
		Player->ResetCharacter(arpg::ECharacterClass::Warrior);
		Player->State.WorldSeed = CurrentWorldSeed;
	}
	if (Dungeon != nullptr)
	{
		Player->Respawn(Dungeon->GetEntranceLocation());
	}
}

void AArpgGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SaveGame();
	Super::EndPlay(EndPlayReason);
}

AArpgPlayerCharacter* AArpgGameMode::GetPlayerCharacter() const
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	return PlayerController != nullptr ? Cast<AArpgPlayerCharacter>(PlayerController->GetPawn()) : nullptr;
}

void AArpgGameMode::Announce(const FString& Text) const
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (AArpgHUD* Hud = PlayerController != nullptr ? Cast<AArpgHUD>(PlayerController->GetHUD()) : nullptr)
	{
		Hud->ShowMessage(Text);
	}
}

// ---------------------------------------------------------------- Niveles

void AArpgGameMode::StartLevel(int32 NewDepth, arpg::uint64 WorldSeed)
{
	Depth = FMath::Clamp(NewDepth, 1, arpg::MaxDepth);
	CurrentWorldSeed = WorldSeed;

	for (TActorIterator<AArpgMonster> It(GetWorld()); It; ++It) { It->Destroy(); }
	for (TActorIterator<AArpgLootPickup> It(GetWorld()); It; ++It) { It->Destroy(); }
	if (Dungeon == nullptr)
	{
		return;
	}

	// Cada profundidad es un poco más grande. Misma semilla y profundidad => misma mazmorra.
	arpg::DungeonParams Params;
	Params.Seed = WorldSeed + static_cast<arpg::uint64>(Depth) * 7919u;
	Params.Width = 56 + FMath::Min(Depth, 12) * 2;
	Params.Height = Params.Width;
	Params.TargetRooms = 8 + FMath::Min(Depth, 8);
	Dungeon->Build(Params);

	const arpg::Dungeon& Map = Dungeon->GetMap();
	arpg::Rng Population(Params.Seed, 77);
	const int32 BossDef = arpg::FindMonsterDef("ash_walker");

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (const arpg::DungeonSpawn& Spawn : Map.Spawns)
	{
		const bool bBoss = Spawn.Rank == arpg::EMonsterRank::Boss;
		const int32 DefIndex = bBoss ? BossDef : Population.RangeInt(0, arpg::GetMonsterDefCount() - 1);
		const arpg::MonsterInstance Instance =
			arpg::SpawnMonster(DefIndex, Depth, Spawn.Rank, arpg::EDifficulty::Pilgrim);
		const FVector Location = Dungeon->TileToWorld(Spawn.X, Spawn.Y) + FVector(0.f, 0.f, 100.f);
		if (AArpgMonster* Monster = GetWorld()->SpawnActor<AArpgMonster>(Location, FRotator::ZeroRotator, SpawnParams))
		{
			Monster->Init(Instance, Dungeon);
		}
	}

	// Los cofres todavía no son un objeto interactivo: su contenido espera en el suelo de la sala de tesoro.
	for (const arpg::DungeonChest& Chest : Map.Chests)
	{
		const arpg::MonsterInstance Guardian =
			arpg::SpawnMonster(BossDef, Depth, arpg::EMonsterRank::Elite, arpg::EDifficulty::Pilgrim);
		DropLoot(arpg::RollLoot(Guardian, 0, ArpgGlobalRng()), Dungeon->TileToWorld(Chest.X, Chest.Y));
	}

	if (AArpgPlayerCharacter* Player = GetPlayerCharacter())
	{
		Player->State.Depth = Depth;
		Player->State.WorldSeed = WorldSeed;
		Player->Respawn(Dungeon->GetEntranceLocation());
	}
	Announce(FString::Printf(TEXT("Profundidad %d"), Depth));
}

void AArpgGameMode::RestartFromState()
{
	if (const AArpgPlayerCharacter* Player = GetPlayerCharacter())
	{
		StartLevel(Player->State.Depth, Player->State.WorldSeed);
	}
}

void AArpgGameMode::AdvanceLevel()
{
	StartLevel(Depth + 1, CurrentWorldSeed);
	SaveGame();
}

// ---------------------------------------------------------------- Muertes y botín

FVector AArpgGameMode::ScatterAround(const FVector& Location) const
{
	arpg::Rng& Random = ArpgGlobalRng();
	const FVector Offset(static_cast<float>(Random.RangeInt(-110, 110)), static_cast<float>(Random.RangeInt(-110, 110)), 0.f);
	const FVector Candidate = Location + Offset;
	// Nunca dentro de un muro.
	const FVector Chosen = Dungeon != nullptr && !Dungeon->IsWalkableLocation(Candidate) ? Location : Candidate;
	return FVector(Chosen.X, Chosen.Y, 25.f);
}

void AArpgGameMode::SpawnItemPickup(const arpg::ItemInstance& Item, const FVector& Location)
{
	if (AArpgLootPickup* Pickup = GetWorld()->SpawnActor<AArpgLootPickup>(ScatterAround(Location), FRotator::ZeroRotator))
	{
		Pickup->InitAsItem(Item);
	}
}

void AArpgGameMode::DropLoot(const arpg::LootDrop& Drop, const FVector& Location)
{
	if (Drop.Gold > 0)
	{
		if (AArpgLootPickup* Pickup = GetWorld()->SpawnActor<AArpgLootPickup>(ScatterAround(Location), FRotator::ZeroRotator))
		{
			Pickup->InitAsGold(Drop.Gold);
		}
	}
	for (int32 i = 0; i < Drop.ItemCount; ++i)
	{
		SpawnItemPickup(Drop.Items[i], Location);
	}
}

void AArpgGameMode::NotifyMonsterDied(AArpgMonster* Monster)
{
	if (Monster == nullptr)
	{
		return;
	}
	const arpg::MonsterInstance& Instance = Monster->GetInstance();
	const FVector Location = Monster->GetActorLocation();
	AArpgPlayerCharacter* Player = GetPlayerCharacter();
	const int32 MagicFind = Player != nullptr ? Player->Stats.MagicFind : 0;

	if (Player != nullptr)
	{
		Player->GainExperience(arpg::ComputeKillExperience(Instance.Experience, Player->State.Progress.Level, Instance.Level));
	}
	DropLoot(arpg::RollLoot(Instance, MagicFind, ArpgGlobalRng()), Location);

	// Pociones: 12 % cada tipo; los jefes sueltan una de cada.
	const bool bBoss = Instance.Rank == arpg::EMonsterRank::Boss;
	for (int32 Kind = 0; Kind < 2; ++Kind)
	{
		if (bBoss || ArpgGlobalRng().RangeInt(0, 99) < 12)
		{
			if (AArpgLootPickup* Pickup = GetWorld()->SpawnActor<AArpgLootPickup>(ScatterAround(Location), FRotator::ZeroRotator))
			{
				Pickup->InitAsPotion(Kind == 1);
			}
		}
	}

	if (bBoss)
	{
		Announce(TEXT("¡Jefe derrotado! Recoge el botín: desciendes en 12 segundos"));
		GetWorldTimerManager().SetTimer(AdvanceTimer, this, &AArpgGameMode::AdvanceLevel, 12.f, false);
	}
}

void AArpgGameMode::NotifyPlayerDied(AArpgPlayerCharacter* /*Player*/)
{
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AArpgGameMode::RespawnPlayer, 3.f, false);
}

void AArpgGameMode::RespawnPlayer()
{
	AArpgPlayerCharacter* Player = GetPlayerCharacter();
	if (Player != nullptr && Dungeon != nullptr)
	{
		Player->Respawn(Dungeon->GetEntranceLocation());
	}
}

// ---------------------------------------------------------------- Guardado

bool AArpgGameMode::SaveGame()
{
	const AArpgPlayerCharacter* Player = GetPlayerCharacter();
	if (Player == nullptr)
	{
		return false;
	}
	std::vector<arpg::uint8> Serialized;
	arpg::SerializeCharacter(Player->State, Serialized);
	const TArray<uint8> Bytes(Serialized.data(), static_cast<int32>(Serialized.size()));

	// Escritura segura: primero a un temporal, luego se conserva el guardado anterior como .bak
	// y solo entonces se sustituye. Un corte de luz a mitad nunca deja la partida a medias.
	const FString Path = GetSavePath();
	const FString TempPath = Path + TEXT(".tmp");
	IFileManager& Files = IFileManager::Get();
	if (!FFileHelper::SaveArrayToFile(Bytes, *TempPath))
	{
		UE_LOG(LogTemp, Error, TEXT("No se pudo escribir el guardado en %s"), *TempPath);
		return false;
	}
	if (Files.FileExists(*Path))
	{
		Files.Copy(*(Path + TEXT(".bak")), *Path, /*bReplace=*/ true);
	}
	return Files.Move(*Path, *TempPath, /*bReplace=*/ true);
}
