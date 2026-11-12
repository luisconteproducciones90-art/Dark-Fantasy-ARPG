#include "ArpgCore/Map/Dungeon.h"
#include "TestFramework.h"

using namespace arpg;

namespace
{
	// Comprueba todas las garantías documentadas. Devuelve false al primer incumplimiento.
	bool IsValid(const Dungeon& D)
	{
		const int32 RoomCount = static_cast<int32>(D.Rooms.size());
		if (RoomCount < 3 || D.Tiles.size() != static_cast<size_t>(D.Width) * static_cast<size_t>(D.Height)) { return false; }
		if (D.EntranceRoom < 0 || D.BossRoom < 0 || D.EntranceRoom == D.BossRoom) { return false; }

		int Entrances = 0, Bosses = 0;
		for (int32 i = 0; i < RoomCount; ++i)
		{
			const DungeonRoom& A = D.Rooms[static_cast<size_t>(i)];
			if (A.X < 1 || A.Y < 1 || A.X + A.Width > D.Width - 1 || A.Y + A.Height > D.Height - 1) { return false; }
			Entrances += A.Kind == ERoomKind::Entrance ? 1 : 0;
			Bosses += A.Kind == ERoomKind::Boss ? 1 : 0;
			for (int32 k = i + 1; k < RoomCount; ++k)
			{
				const DungeonRoom& B = D.Rooms[static_cast<size_t>(k)];
				if (A.X < B.X + B.Width && A.X + A.Width > B.X && A.Y < B.Y + B.Height && A.Y + A.Height > B.Y) { return false; }
			}
		}
		if (Entrances != 1 || Bosses != 1) { return false; }
		if (D.Rooms[static_cast<size_t>(D.EntranceRoom)].Kind != ERoomKind::Entrance) { return false; }
		if (!D.Rooms[static_cast<size_t>(D.BossRoom)].bOnCriticalPath) { return false; }

		// El borde es muro.
		for (int32 X = 0; X < D.Width; ++X) { if (D.At(X, 0) != ETile::Wall || D.At(X, D.Height - 1) != ETile::Wall) { return false; } }
		for (int32 Y = 0; Y < D.Height; ++Y) { if (D.At(0, Y) != ETile::Wall || D.At(D.Width - 1, Y) != ETile::Wall) { return false; } }

		// Relleno por inundación desde la entrada: todo el suelo debe ser alcanzable.
		std::vector<bool> Seen(D.Tiles.size(), false);
		std::vector<int32> Stack;
		const DungeonRoom& Entrance = D.Rooms[static_cast<size_t>(D.EntranceRoom)];
		Stack.push_back(Entrance.CenterY() * D.Width + Entrance.CenterX());
		while (!Stack.empty())
		{
			const int32 Index = Stack.back();
			Stack.pop_back();
			if (Seen[static_cast<size_t>(Index)]) { continue; }
			Seen[static_cast<size_t>(Index)] = true;
			const int32 X = Index % D.Width, Y = Index / D.Width;
			const int32 Dx[] = {1, -1, 0, 0}, Dy[] = {0, 0, 1, -1};
			for (int i = 0; i < 4; ++i)
			{
				if (D.At(X + Dx[i], Y + Dy[i]) == ETile::Floor) { Stack.push_back((Y + Dy[i]) * D.Width + X + Dx[i]); }
			}
		}
		for (size_t i = 0; i < D.Tiles.size(); ++i)
		{
			if (D.Tiles[i] == ETile::Floor && !Seen[i]) { return false; }
		}

		int BossSpawns = 0;
		for (const DungeonSpawn& S : D.Spawns)
		{
			if (D.At(S.X, S.Y) != ETile::Floor || S.RoomIndex == D.EntranceRoom) { return false; }
			if (S.Rank == EMonsterRank::Boss)
			{
				++BossSpawns;
				if (S.RoomIndex != D.BossRoom) { return false; }
			}
		}
		if (BossSpawns != 1) { return false; }
		for (const DungeonChest& Chest : D.Chests)
		{
			if (D.At(Chest.X, Chest.Y) != ETile::Floor) { return false; }
			const DungeonRoom& Room = D.Rooms[static_cast<size_t>(Chest.RoomIndex)];
			if (Room.Kind != ERoomKind::Treasure || Room.bOnCriticalPath) { return false; }
		}
		return true;
	}
}

ARPG_TEST(Dungeon_ThousandSeedsAreAllValid)
{
	int TotalRooms = 0, WithTreasure = 0;
	for (uint64 Seed = 1; Seed <= 1000; ++Seed)
	{
		DungeonParams P;
		P.Seed = Seed;
		const Dungeon D = GenerateDungeon(P);
		EXPECT_TRUE(IsValid(D));
		TotalRooms += static_cast<int>(D.Rooms.size());
		WithTreasure += D.Chests.empty() ? 0 : 1;
	}
	EXPECT_TRUE(TotalRooms / 1000 >= 9);   // se acerca al objetivo de 12 salas
	EXPECT_TRUE(WithTreasure > 700);       // casi todas tienen alguna sala de tesoro
}

ARPG_TEST(Dungeon_SameSeedSameDungeonDifferentSeedDifferent)
{
	DungeonParams P;
	P.Seed = 777;
	const Dungeon A = GenerateDungeon(P), B = GenerateDungeon(P);
	EXPECT_TRUE(A.Tiles == B.Tiles && A.Rooms.size() == B.Rooms.size() && A.Spawns.size() == B.Spawns.size());
	EXPECT_TRUE(A.EntranceRoom == B.EntranceRoom && A.BossRoom == B.BossRoom);
	for (size_t i = 0; i < A.Spawns.size(); ++i)
	{
		EXPECT_TRUE(A.Spawns[i].X == B.Spawns[i].X && A.Spawns[i].Y == B.Spawns[i].Y && A.Spawns[i].Rank == B.Spawns[i].Rank);
	}
	P.Seed = 778;
	EXPECT_TRUE(!(GenerateDungeon(P).Tiles == A.Tiles));
}

ARPG_TEST(Dungeon_BossIsFarFromEntrance)
{
	int FarEnough = 0;
	for (uint64 Seed = 1; Seed <= 300; ++Seed)
	{
		DungeonParams P;
		P.Seed = Seed;
		const Dungeon D = GenerateDungeon(P);
		const DungeonRoom& E = D.Rooms[static_cast<size_t>(D.EntranceRoom)];
		const DungeonRoom& B = D.Rooms[static_cast<size_t>(D.BossRoom)];
		const int32 Dx = E.CenterX() - B.CenterX(), Dy = E.CenterY() - B.CenterY();
		FarEnough += (Dx < 0 ? -Dx : Dx) + (Dy < 0 ? -Dy : Dy) >= 30 ? 1 : 0;
		int OnPath = 0;
		for (const DungeonRoom& Room : D.Rooms) { OnPath += Room.bOnCriticalPath ? 1 : 0; }
		EXPECT_TRUE(OnPath >= 3); // entrada, al menos una sala intermedia y jefe
	}
	EXPECT_TRUE(FarEnough > 270);
}

ARPG_TEST(Dungeon_ExtremeParamsStillValid)
{
	const DungeonParams Cases[] = {
		{1, 0, 0, 0, 0, 0, 0},                       // todo a cero
		{2, -50, -50, -3, -1, -1, -9},               // negativos
		{3, 100000, 100000, 100000, 1000, 1000, 1},  // enormes
		{4, 24, 24, 64, 16, 16, 4},                  // mapa mínimo con salas que no caben
		{5, 256, 24, 30, 5, 9, 14},                  // muy alargado
		{6, 64, 64, 12, 11, 5, 14},                  // mínimo mayor que máximo
	};
	for (const DungeonParams& P : Cases)
	{
		const Dungeon D = GenerateDungeon(P);
		EXPECT_TRUE(IsValid(D));
		EXPECT_TRUE(D.Width >= 24 && D.Width <= 256 && D.Height >= 24 && D.Height <= 256);
	}
}

ARPG_TEST(Dungeon_DensityParameterControlsMonsterCount)
{
	DungeonParams Sparse, Dense;
	Sparse.Seed = Dense.Seed = 31;
	Sparse.TilesPerMonster = 40;
	Dense.TilesPerMonster = 6;
	const Dungeon A = GenerateDungeon(Sparse), B = GenerateDungeon(Dense);
	EXPECT_TRUE(A.Tiles == B.Tiles); // la densidad no altera el trazado
	EXPECT_TRUE(B.Spawns.size() > A.Spawns.size() * 2);
}
