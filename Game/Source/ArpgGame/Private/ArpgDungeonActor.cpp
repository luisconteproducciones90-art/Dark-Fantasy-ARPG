#include "ArpgDungeonActor.h"

#include "ArpgCore/Map/Pathfinding.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AArpgDungeonActor::AArpgDungeonActor()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = ShapeMaterial.Object;

	FloorMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorMeshes"));
	SetRootComponent(FloorMeshes);
	FloorMeshes->SetStaticMesh(CubeMesh.Object);
	FloorMeshes->SetMobility(EComponentMobility::Movable);

	WallMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallMeshes"));
	WallMeshes->SetupAttachment(FloorMeshes);
	WallMeshes->SetStaticMesh(CubeMesh.Object);
	WallMeshes->SetMobility(EComponentMobility::Movable);
}

void AArpgDungeonActor::Build(const arpg::DungeonParams& Params)
{
	Map = arpg::GenerateDungeon(Params);

	FloorMeshes->ClearInstances();
	WallMeshes->ClearInstances();

	if (BaseMaterial != nullptr)
	{
		UMaterialInstanceDynamic* FloorMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		FloorMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.11f, 0.10f, 0.09f));
		FloorMeshes->SetMaterial(0, FloorMaterial);

		UMaterialInstanceDynamic* WallMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		WallMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.045f, 0.05f));
		WallMeshes->SetMaterial(0, WallMaterial);
	}

	// El cubo del motor mide 100 cm de lado.
	const FVector FloorScale(TileSize / 100.f, TileSize / 100.f, 0.2f);
	const FVector WallScale(TileSize / 100.f, TileSize / 100.f, WallHeight / 100.f);

	for (int32 Y = 0; Y < Map.Height; ++Y)
	{
		for (int32 X = 0; X < Map.Width; ++X)
		{
			const FVector Center = TileToWorld(X, Y);
			if (Map.At(X, Y) == arpg::ETile::Floor)
			{
				// Cara superior del suelo en Z = 0.
				FloorMeshes->AddInstance(
					FTransform(FRotator::ZeroRotator, Center - FVector(0.f, 0.f, 10.f), FloorScale), /*bWorldSpace=*/ true);
				continue;
			}
			// Solo se construyen los muros que tocan suelo; el resto no se ve nunca.
			bool bTouchesFloor = false;
			for (int32 Dy = -1; Dy <= 1 && !bTouchesFloor; ++Dy)
			{
				for (int32 Dx = -1; Dx <= 1; ++Dx)
				{
					if (Map.At(X + Dx, Y + Dy) == arpg::ETile::Floor)
					{
						bTouchesFloor = true;
						break;
					}
				}
			}
			if (bTouchesFloor)
			{
				WallMeshes->AddInstance(
					FTransform(FRotator::ZeroRotator, Center + FVector(0.f, 0.f, WallHeight * 0.5f), WallScale),
					/*bWorldSpace=*/ true);
			}
		}
	}
}

FVector AArpgDungeonActor::TileToWorld(int32 X, int32 Y) const
{
	return GetActorLocation()
		+ FVector((static_cast<float>(X) + 0.5f) * TileSize, (static_cast<float>(Y) + 0.5f) * TileSize, 0.f);
}

void AArpgDungeonActor::WorldToTile(const FVector& Location, int32& OutX, int32& OutY) const
{
	const FVector Local = Location - GetActorLocation();
	OutX = FMath::FloorToInt32(Local.X / TileSize);
	OutY = FMath::FloorToInt32(Local.Y / TileSize);
}

bool AArpgDungeonActor::IsWalkableLocation(const FVector& Location) const
{
	int32 X = 0, Y = 0;
	WorldToTile(Location, X, Y);
	return arpg::IsWalkable(Map, X, Y);
}

bool AArpgDungeonActor::FindWorldPath(const FVector& From, const FVector& To, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();
	arpg::TilePoint Start, Goal;
	WorldToTile(From, Start.X, Start.Y);
	WorldToTile(To, Goal.X, Goal.Y);

	std::vector<arpg::TilePoint> Tiles;
	if (!arpg::FindPath(Map, Start, Goal, Tiles))
	{
		return false;
	}
	for (const arpg::TilePoint& Tile : Tiles)
	{
		OutPoints.Add(TileToWorld(Tile.X, Tile.Y));
	}
	// El último tramo va al punto exacto pedido, no al centro de su casilla.
	const FVector Exact(To.X, To.Y, 0.f);
	if (OutPoints.Num() > 0)
	{
		OutPoints.Last() = Exact;
	}
	else
	{
		OutPoints.Add(Exact);
	}
	return true;
}

FVector AArpgDungeonActor::GetEntranceLocation() const
{
	if (Map.EntranceRoom < 0 || Map.EntranceRoom >= static_cast<int32>(Map.Rooms.size()))
	{
		return GetActorLocation();
	}
	const arpg::DungeonRoom& Room = Map.Rooms[static_cast<size_t>(Map.EntranceRoom)];
	return TileToWorld(Room.CenterX(), Room.CenterY());
}
