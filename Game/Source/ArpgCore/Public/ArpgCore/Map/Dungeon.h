#pragma once

#include "ArpgCore/Core/Types.h"
#include "ArpgCore/Item/Item.h"

#include <vector>

namespace arpg
{
	enum class ETile : uint8 { Wall, Floor };

	enum class ERoomKind : uint8
	{
		Entrance,
		Normal,
		Treasure, // rama fuera de la ruta principal, con cofre vigilado
		Boss
	};

	struct DungeonRoom
	{
		int32 X = 0;
		int32 Y = 0;
		int32 Width = 0;
		int32 Height = 0;
		ERoomKind Kind = ERoomKind::Normal;
		/** true si está en el camino más corto de la entrada al jefe. */
		bool bOnCriticalPath = false;

		int32 CenterX() const { return X + Width / 2; }
		int32 CenterY() const { return Y + Height / 2; }
	};

	struct DungeonSpawn
	{
		int32 X = 0;
		int32 Y = 0;
		int32 RoomIndex = 0;
		EMonsterRank Rank = EMonsterRank::Normal;
	};

	struct DungeonChest
	{
		int32 X = 0;
		int32 Y = 0;
		int32 RoomIndex = 0;
	};

	struct DungeonParams
	{
		uint64 Seed = 1;
		int32 Width = 64;        // en casillas; el motor decide cuántos cm mide una casilla
		int32 Height = 64;
		int32 TargetRooms = 12;
		int32 MinRoomSize = 5;
		int32 MaxRoomSize = 11;
		/** Casillas de sala por monstruo: menos = más densidad. */
		int32 TilesPerMonster = 14;
	};

	struct Dungeon
	{
		int32 Width = 0;
		int32 Height = 0;
		std::vector<ETile> Tiles;
		std::vector<DungeonRoom> Rooms;
		std::vector<DungeonSpawn> Spawns;
		std::vector<DungeonChest> Chests;
		int32 EntranceRoom = -1;
		int32 BossRoom = -1;

		/** Fuera del mapa cuenta como muro. */
		ETile At(int32 X, int32 Y) const
		{
			return X < 0 || Y < 0 || X >= Width || Y >= Height
				? ETile::Wall
				: Tiles[static_cast<size_t>(Y) * static_cast<size_t>(Width) + static_cast<size_t>(X)];
		}
	};

	/**
	 * Generación procedural controlada. Garantías, cubiertas por pruebas:
	 *  - misma semilla y parámetros => misma mazmorra;
	 *  - al menos 3 salas, sin solaparse, todas alcanzables a pie desde la entrada;
	 *  - exactamente una entrada y una sala de jefe, siendo esta la más lejana;
	 *  - sin monstruos en la sala de entrada; un único jefe, en su sala.
	 * Los parámetros fuera de rango se ajustan a valores válidos.
	 */
	Dungeon GenerateDungeon(const DungeonParams& Params);
}
