#include "ArpgCore/Map/Dungeon.h"

#include "ArpgCore/Core/Rng.h"

namespace arpg
{
	namespace
	{
		constexpr int32 MinMapSize = 24;
		constexpr int32 MaxMapSize = 256;
		constexpr int32 MinRooms = 3;

		int32 Abs(int32 V) { return V < 0 ? -V : V; }

		size_t TileIndex(const Dungeon& D, int32 X, int32 Y)
		{
			return static_cast<size_t>(Y) * static_cast<size_t>(D.Width) + static_cast<size_t>(X);
		}

		void Carve(Dungeon& D, int32 X, int32 Y)
		{
			// El borde exterior siempre es muro.
			if (X >= 1 && Y >= 1 && X < D.Width - 1 && Y < D.Height - 1)
			{
				D.Tiles[TileIndex(D, X, Y)] = ETile::Floor;
			}
		}

		bool Overlaps(const DungeonRoom& A, const DungeonRoom& B, int32 Margin)
		{
			return A.X - Margin < B.X + B.Width && A.X + A.Width + Margin > B.X
				&& A.Y - Margin < B.Y + B.Height && A.Y + A.Height + Margin > B.Y;
		}

		void CarveCorridor(Dungeon& D, const DungeonRoom& From, const DungeonRoom& To, bool bHorizontalFirst)
		{
			int32 X = From.CenterX();
			int32 Y = From.CenterY();
			const int32 Tx = To.CenterX();
			const int32 Ty = To.CenterY();
			const auto WalkX = [&]() { while (X != Tx) { Carve(D, X, Y); X += Tx > X ? 1 : -1; } };
			const auto WalkY = [&]() { while (Y != Ty) { Carve(D, X, Y); Y += Ty > Y ? 1 : -1; } };
			if (bHorizontalFirst) { WalkX(); WalkY(); } else { WalkY(); WalkX(); }
			Carve(D, X, Y);
		}

		int32 Distance(const DungeonRoom& A, const DungeonRoom& B)
		{
			return Abs(A.CenterX() - B.CenterX()) + Abs(A.CenterY() - B.CenterY());
		}

		void PlaceRooms(Dungeon& D, const DungeonParams& P, Rng& Random)
		{
			const int32 Attempts = P.TargetRooms * 40;
			for (int32 i = 0; i < Attempts && static_cast<int32>(D.Rooms.size()) < P.TargetRooms; ++i)
			{
				DungeonRoom Room;
				Room.Width = Random.RangeInt(P.MinRoomSize, P.MaxRoomSize);
				Room.Height = Random.RangeInt(P.MinRoomSize, P.MaxRoomSize);
				Room.X = Random.RangeInt(1, D.Width - Room.Width - 1);
				Room.Y = Random.RangeInt(1, D.Height - Room.Height - 1);
				bool bFree = true;
				for (const DungeonRoom& Other : D.Rooms)
				{
					if (Overlaps(Room, Other, 2)) { bFree = false; break; }
				}
				if (bFree)
				{
					D.Rooms.push_back(Room);
				}
			}
		}

		// Respaldo determinista si el azar no logra colocar suficientes salas: tres salas en diagonal.
		void PlaceFallbackRooms(Dungeon& D)
		{
			D.Rooms.clear();
			const int32 Size = 5;
			for (int32 i = 0; i < MinRooms; ++i)
			{
				DungeonRoom Room;
				Room.Width = Size;
				Room.Height = Size;
				Room.X = 1 + i * (D.Width - Size - 2) / (MinRooms - 1);
				Room.Y = 1 + i * (D.Height - Size - 2) / (MinRooms - 1);
				D.Rooms.push_back(Room);
			}
		}
	}

	Dungeon GenerateDungeon(const DungeonParams& InParams)
	{
		DungeonParams P = InParams;
		P.Width = Clamp(P.Width, MinMapSize, MaxMapSize);
		P.Height = Clamp(P.Height, MinMapSize, MaxMapSize);
		P.MinRoomSize = Clamp(P.MinRoomSize, 4, 16);
		const int32 LargestThatFits = (P.Width < P.Height ? P.Width : P.Height) / 3;
		P.MaxRoomSize = Clamp(P.MaxRoomSize, P.MinRoomSize, LargestThatFits < P.MinRoomSize ? P.MinRoomSize : LargestThatFits);
		P.TargetRooms = Clamp(P.TargetRooms, MinRooms, 64);
		P.TilesPerMonster = Clamp(P.TilesPerMonster, 4, 200);

		Rng Random(P.Seed, 0x9E3779B97F4A7C15ULL);

		Dungeon D;
		D.Width = P.Width;
		D.Height = P.Height;
		D.Tiles.assign(static_cast<size_t>(P.Width) * static_cast<size_t>(P.Height), ETile::Wall);

		PlaceRooms(D, P, Random);
		if (static_cast<int32>(D.Rooms.size()) < MinRooms)
		{
			PlaceFallbackRooms(D);
		}
		const int32 RoomCount = static_cast<int32>(D.Rooms.size());
		const size_t Count = static_cast<size_t>(RoomCount);

		for (const DungeonRoom& Room : D.Rooms)
		{
			for (int32 Y = Room.Y; Y < Room.Y + Room.Height; ++Y)
			{
				for (int32 X = Room.X; X < Room.X + Room.Width; ++X)
				{
					Carve(D, X, Y);
				}
			}
		}

		// Árbol de expansión mínima (Prim): conecta todas las salas con pasillos cortos.
		std::vector<int32> Parent(Count, -1);
		std::vector<int32> Best(Count, INT32_MAX);
		std::vector<bool> bInTree(Count, false);
		std::vector<std::vector<int32>> Links(Count);
		Best[0] = 0;
		for (int32 Step = 0; Step < RoomCount; ++Step)
		{
			int32 Next = -1;
			for (int32 i = 0; i < RoomCount; ++i)
			{
				const size_t Si = static_cast<size_t>(i);
				if (!bInTree[Si] && (Next < 0 || Best[Si] < Best[static_cast<size_t>(Next)])) { Next = i; }
			}
			const size_t Sn = static_cast<size_t>(Next);
			bInTree[Sn] = true;
			if (Parent[Sn] >= 0)
			{
				const size_t Sp = static_cast<size_t>(Parent[Sn]);
				Links[Sn].push_back(Parent[Sn]);
				Links[Sp].push_back(Next);
				CarveCorridor(D, D.Rooms[Sp], D.Rooms[Sn], Random.Chance(0.5f));
			}
			for (int32 i = 0; i < RoomCount; ++i)
			{
				const size_t Si = static_cast<size_t>(i);
				const int32 Dist = Distance(D.Rooms[Sn], D.Rooms[Si]);
				if (!bInTree[Si] && Dist < Best[Si]) { Best[Si] = Dist; Parent[Si] = Next; }
			}
		}

		// Entrada: la sala más cercana a una esquina. Jefe: la más lejana en saltos por el árbol.
		D.EntranceRoom = 0;
		for (int32 i = 1; i < RoomCount; ++i)
		{
			const DungeonRoom& R = D.Rooms[static_cast<size_t>(i)];
			const DungeonRoom& E = D.Rooms[static_cast<size_t>(D.EntranceRoom)];
			if (R.CenterX() + R.CenterY() < E.CenterX() + E.CenterY()) { D.EntranceRoom = i; }
		}
		std::vector<int32> Hops(Count, -1);
		std::vector<int32> Via(Count, -1);
		std::vector<int32> Queue;
		Queue.push_back(D.EntranceRoom);
		Hops[static_cast<size_t>(D.EntranceRoom)] = 0;
		D.BossRoom = D.EntranceRoom;
		for (size_t Head = 0; Head < Queue.size(); ++Head)
		{
			const int32 Current = Queue[Head];
			const size_t Sc = static_cast<size_t>(Current);
			if (Hops[Sc] > Hops[static_cast<size_t>(D.BossRoom)]) { D.BossRoom = Current; }
			for (const int32 Neighbor : Links[Sc])
			{
				const size_t Sn = static_cast<size_t>(Neighbor);
				if (Hops[Sn] < 0)
				{
					Hops[Sn] = Hops[Sc] + 1;
					Via[Sn] = Current;
					Queue.push_back(Neighbor);
				}
			}
		}
		for (int32 Room = D.BossRoom; Room >= 0; Room = Via[static_cast<size_t>(Room)])
		{
			D.Rooms[static_cast<size_t>(Room)].bOnCriticalPath = true;
		}
		D.Rooms[static_cast<size_t>(D.EntranceRoom)].Kind = ERoomKind::Entrance;
		D.Rooms[static_cast<size_t>(D.BossRoom)].Kind = ERoomKind::Boss;

		// Salas de tesoro: hojas del árbol fuera de la ruta principal.
		for (int32 i = 0; i < RoomCount; ++i)
		{
			const size_t Si = static_cast<size_t>(i);
			DungeonRoom& Room = D.Rooms[Si];
			if (Room.Kind == ERoomKind::Normal && !Room.bOnCriticalPath && Links[Si].size() == 1)
			{
				Room.Kind = ERoomKind::Treasure;
				D.Chests.push_back({Room.CenterX(), Room.CenterY(), i});
			}
		}

		// Atajos: algún pasillo extra entre salas cercanas para que haya bucles y no solo un árbol.
		// Nunca desde salas de tesoro ni hacia la sala del jefe, para no romper su papel.
		for (int32 i = 0; i < RoomCount; ++i)
		{
			const DungeonRoom& A = D.Rooms[static_cast<size_t>(i)];
			if (A.Kind != ERoomKind::Normal || !Random.Chance(0.2f)) { continue; }
			int32 Nearest = -1;
			for (int32 k = 0; k < RoomCount; ++k)
			{
				const DungeonRoom& B = D.Rooms[static_cast<size_t>(k)];
				bool bLinked = k == i;
				for (const int32 L : Links[static_cast<size_t>(i)]) { bLinked = bLinked || L == k; }
				if (bLinked || B.Kind != ERoomKind::Normal) { continue; }
				if (Nearest < 0 || Distance(A, B) < Distance(A, D.Rooms[static_cast<size_t>(Nearest)])) { Nearest = k; }
			}
			if (Nearest >= 0)
			{
				CarveCorridor(D, A, D.Rooms[static_cast<size_t>(Nearest)], Random.Chance(0.5f));
			}
		}

		// Población.
		for (int32 i = 0; i < RoomCount; ++i)
		{
			const DungeonRoom& Room = D.Rooms[static_cast<size_t>(i)];
			if (Room.Kind == ERoomKind::Entrance)
			{
				continue;
			}
			if (Room.Kind == ERoomKind::Boss)
			{
				D.Spawns.push_back({Room.CenterX(), Room.CenterY(), i, EMonsterRank::Boss});
				continue;
			}
			int32 Monsters = Room.Width * Room.Height / P.TilesPerMonster;
			if (Monsters < 1) { Monsters = 1; }
			if (Room.Kind == ERoomKind::Treasure) { ++Monsters; } // el cofre está vigilado
			// Un líder por sala como mucho: 10 % campeón, 4 % élite.
			const int32 LeaderRoll = Random.RangeInt(0, 99);
			for (int32 m = 0; m < Monsters; ++m)
			{
				DungeonSpawn Spawn;
				Spawn.X = Random.RangeInt(Room.X, Room.X + Room.Width - 1);
				Spawn.Y = Random.RangeInt(Room.Y, Room.Y + Room.Height - 1);
				Spawn.RoomIndex = i;
				if (m == 0)
				{
					Spawn.Rank = LeaderRoll < 4 ? EMonsterRank::Elite
						: (LeaderRoll < 14 ? EMonsterRank::Champion : EMonsterRank::Normal);
				}
				D.Spawns.push_back(Spawn);
			}
		}
		return D;
	}
}
