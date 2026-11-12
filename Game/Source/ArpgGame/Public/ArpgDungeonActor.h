#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArpgCore/Map/Dungeon.h"
#include "ArpgDungeonActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Convierte una mazmorra de ArpgCore en geometría: suelo y muros como mallas
 * instanciadas (una llamada de dibujado por tipo de pieza). Hoy usa cubos del
 * motor; en producción cada casilla elegirá una pieza modular del bioma.
 * También traduce entre mundo y casillas y resuelve caminos sobre la cuadrícula.
 */
UCLASS()
class ARPGGAME_API AArpgDungeonActor : public AActor
{
	GENERATED_BODY()

public:
	AArpgDungeonActor();

	/** Lado de una casilla en centímetros. */
	static constexpr float TileSize = 300.f;
	static constexpr float WallHeight = 160.f;

	void Build(const arpg::DungeonParams& Params);

	const arpg::Dungeon& GetMap() const { return Map; }

	FVector TileToWorld(int32 X, int32 Y) const;
	void WorldToTile(const FVector& Location, int32& OutX, int32& OutY) const;
	bool IsWalkableLocation(const FVector& Location) const;

	/** Camino a pie entre dos puntos. OutPoints no incluye el origen. false si no hay camino. */
	bool FindWorldPath(const FVector& From, const FVector& To, TArray<FVector>& OutPoints) const;

	FVector GetEntranceLocation() const;

private:
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> FloorMeshes;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> WallMeshes;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	arpg::Dungeon Map;
};
