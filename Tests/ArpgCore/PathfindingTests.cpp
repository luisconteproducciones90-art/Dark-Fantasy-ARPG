#include "ArpgCore/Map/Pathfinding.h"
#include "TestFramework.h"

using namespace arpg;

namespace
{
	// Construye un mapa a partir de texto: '#' muro, cualquier otra cosa suelo.
	Dungeon FromText(const char* const* Rows, int32 RowCount)
	{
		Dungeon D;
		D.Height = RowCount;
		D.Width = 0;
		while (Rows[0][D.Width] != '\0') { ++D.Width; }
		for (int32 Y = 0; Y < RowCount; ++Y)
		{
			for (int32 X = 0; X < D.Width; ++X) { D.Tiles.push_back(Rows[Y][X] == '#' ? ETile::Wall : ETile::Floor); }
		}
		return D;
	}

	bool StepsAreAdjacentAndWalkable(const Dungeon& D, TilePoint From, const std::vector<TilePoint>& Path)
	{
		TilePoint Prev = From;
		for (const TilePoint& P : Path)
		{
			const int32 Dx = P.X - Prev.X, Dy = P.Y - Prev.Y;
			if (!IsWalkable(D, P.X, P.Y) || Dx < -1 || Dx > 1 || Dy < -1 || Dy > 1 || (Dx == 0 && Dy == 0)) { return false; }
			if (Dx != 0 && Dy != 0 && (!IsWalkable(D, Prev.X + Dx, Prev.Y) || !IsWalkable(D, Prev.X, Prev.Y + Dy))) { return false; }
			Prev = P;
		}
		return true;
	}
}

ARPG_TEST(Path_GoesAroundWallsWithoutCuttingCorners)
{
	const char* Rows[] = {
		"#########",
		"#...#...#",
		"#...#...#",
		"#...#...#",
		"#.......#",
		"#########",
	};
	const Dungeon D = FromText(Rows, 6);
	std::vector<TilePoint> Path;
	EXPECT_TRUE(FindPath(D, {1, 1}, {7, 1}, Path, false));
	EXPECT_TRUE(StepsAreAdjacentAndWalkable(D, {1, 1}, Path));
	EXPECT_TRUE(!Path.empty() && Path.back().X == 7 && Path.back().Y == 1);
	EXPECT_EQ(static_cast<int>(Path.size()), 8); // óptimo: baja en diagonal, pasa por el hueco y sube

	std::vector<TilePoint> Smooth;
	EXPECT_TRUE(FindPath(D, {1, 1}, {7, 1}, Smooth, true));
	EXPECT_TRUE(Smooth.size() < Path.size() && Smooth.size() >= 2);
	TilePoint Prev{1, 1};
	for (const TilePoint& P : Smooth)
	{
		EXPECT_TRUE(HasClearLine(D, Prev, P));
		Prev = P;
	}
	EXPECT_TRUE(Prev.X == 7 && Prev.Y == 1);
}

ARPG_TEST(Path_StraightLineBecomesSingleSegment)
{
	const char* Rows[] = {"#######", "#.....#", "#.....#", "#######"};
	const Dungeon D = FromText(Rows, 4);
	std::vector<TilePoint> Path;
	EXPECT_TRUE(FindPath(D, {1, 1}, {5, 2}, Path));
	EXPECT_EQ(static_cast<int>(Path.size()), 1);
	EXPECT_TRUE(FindPath(D, {2, 2}, {2, 2}, Path) && Path.empty()); // ya está en el destino
}

ARPG_TEST(Path_UnreachableAndInvalidEndpoints)
{
	const char* Rows[] = {"#######", "#..#..#", "#..#..#", "#######"};
	const Dungeon D = FromText(Rows, 4);
	std::vector<TilePoint> Path;
	Path.push_back({9, 9});
	EXPECT_TRUE(!FindPath(D, {1, 1}, {5, 1}, Path) && Path.empty()); // muro de por medio
	EXPECT_TRUE(!FindPath(D, {1, 1}, {3, 1}, Path));   // destino en muro
	EXPECT_TRUE(!FindPath(D, {0, 0}, {1, 1}, Path));   // origen en muro
	EXPECT_TRUE(!FindPath(D, {1, 1}, {-4, 99}, Path)); // fuera del mapa
	EXPECT_TRUE(!HasClearLine(D, {1, 1}, {5, 1}));
	EXPECT_TRUE(HasClearLine(D, {1, 1}, {2, 2}));
}

ARPG_TEST(Path_DiagonalThroughPinchIsNotAllowed)
{
	const char* Rows[] = {"####", "#.##", "##.#", "####"};
	const Dungeon D = FromText(Rows, 4);
	std::vector<TilePoint> Path;
	EXPECT_TRUE(!FindPath(D, {1, 1}, {2, 2}, Path)); // solo se tocan por la esquina
	EXPECT_TRUE(!HasClearLine(D, {1, 1}, {2, 2}));
}

ARPG_TEST(Path_WorksAcrossGeneratedDungeons)
{
	for (uint64 Seed = 1; Seed <= 150; ++Seed)
	{
		DungeonParams P;
		P.Seed = Seed;
		const Dungeon D = GenerateDungeon(P);
		const DungeonRoom& E = D.Rooms[static_cast<size_t>(D.EntranceRoom)];
		const TilePoint Start{E.CenterX(), E.CenterY()};
		// De la entrada a cada monstruo y cofre: siempre hay camino y cada tramo es transitable.
		for (const DungeonSpawn& S : D.Spawns)
		{
			std::vector<TilePoint> Raw, Smooth;
			EXPECT_TRUE(FindPath(D, Start, {S.X, S.Y}, Raw, false));
			EXPECT_TRUE(StepsAreAdjacentAndWalkable(D, Start, Raw));
			EXPECT_TRUE(FindPath(D, Start, {S.X, S.Y}, Smooth, true));
			EXPECT_TRUE(Smooth.size() <= Raw.size());
			TilePoint Prev = Start;
			for (const TilePoint& Pt : Smooth) { EXPECT_TRUE(HasClearLine(D, Prev, Pt)); Prev = Pt; }
			if (Seed > 20) { break; } // en el resto basta con un monstruo por mazmorra
		}
	}
}
