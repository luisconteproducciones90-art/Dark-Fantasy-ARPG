#include "ArpgCore/Map/Pathfinding.h"

#include <algorithm>
#include <utility>

namespace arpg
{
	namespace
	{
		int32 AbsInt(int32 V) { return V < 0 ? -V : V; }

		int32 FloorToInt(float V)
		{
			const int32 Truncated = static_cast<int32>(V);
			return static_cast<float>(Truncated) > V ? Truncated - 1 : Truncated;
		}
	}

	bool HasClearLine(const Dungeon& Map, TilePoint From, TilePoint To)
	{
		if (!IsWalkable(Map, From.X, From.Y) || !IsWalkable(Map, To.X, To.Y))
		{
			return false;
		}
		const int32 Dx = To.X - From.X;
		const int32 Dy = To.Y - From.Y;
		const int32 Longest = AbsInt(Dx) > AbsInt(Dy) ? AbsInt(Dx) : AbsInt(Dy);
		const int32 Samples = Longest * 4;
		constexpr float Margin = 0.2f; // semianchura del cuerpo, en casillas
		for (int32 i = 1; i < Samples; ++i)
		{
			const float T = static_cast<float>(i) / static_cast<float>(Samples);
			const float Px = static_cast<float>(From.X) + 0.5f + static_cast<float>(Dx) * T;
			const float Py = static_cast<float>(From.Y) + 0.5f + static_cast<float>(Dy) * T;
			if (!IsWalkable(Map, FloorToInt(Px - Margin), FloorToInt(Py - Margin))
				|| !IsWalkable(Map, FloorToInt(Px + Margin), FloorToInt(Py - Margin))
				|| !IsWalkable(Map, FloorToInt(Px - Margin), FloorToInt(Py + Margin))
				|| !IsWalkable(Map, FloorToInt(Px + Margin), FloorToInt(Py + Margin)))
			{
				return false;
			}
		}
		return true;
	}

	bool FindPath(const Dungeon& Map, TilePoint From, TilePoint To, std::vector<TilePoint>& OutPath, bool bSmooth)
	{
		OutPath.clear();
		if (!IsWalkable(Map, From.X, From.Y) || !IsWalkable(Map, To.X, To.Y))
		{
			return false;
		}
		if (From.X == To.X && From.Y == To.Y)
		{
			return true;
		}

		const int32 W = Map.Width;
		const size_t Total = static_cast<size_t>(W) * static_cast<size_t>(Map.Height);
		const auto IndexOf = [W](int32 X, int32 Y) { return Y * W + X; };
		const auto Heuristic = [&To](int32 X, int32 Y)
		{
			const int32 Dx = AbsInt(X - To.X), Dy = AbsInt(Y - To.Y);
			return 10 * (Dx + Dy) - 6 * (Dx < Dy ? Dx : Dy); // distancia octil
		};

		std::vector<int32> Cost(Total, INT32_MAX);
		std::vector<int32> Parent(Total, -1);
		std::vector<bool> bClosed(Total, false);
		using Node = std::pair<int32, int32>; // (coste estimado total, índice)
		std::vector<Node> Open;
		const auto Worse = [](const Node& A, const Node& B) { return A > B; };

		const int32 Start = IndexOf(From.X, From.Y);
		const int32 Goal = IndexOf(To.X, To.Y);
		Cost[static_cast<size_t>(Start)] = 0;
		Open.push_back({Heuristic(From.X, From.Y), Start});

		constexpr int32 StepX[] = {1, -1, 0, 0, 1, 1, -1, -1};
		constexpr int32 StepY[] = {0, 0, 1, -1, 1, -1, 1, -1};

		bool bFound = false;
		while (!Open.empty())
		{
			std::pop_heap(Open.begin(), Open.end(), Worse);
			const int32 Current = Open.back().second;
			Open.pop_back();
			const size_t Sc = static_cast<size_t>(Current);
			if (bClosed[Sc])
			{
				continue;
			}
			bClosed[Sc] = true;
			if (Current == Goal)
			{
				bFound = true;
				break;
			}
			const int32 Cx = Current % W, Cy = Current / W;
			for (int Dir = 0; Dir < 8; ++Dir)
			{
				const int32 Nx = Cx + StepX[Dir], Ny = Cy + StepY[Dir];
				if (!IsWalkable(Map, Nx, Ny))
				{
					continue;
				}
				const bool bDiagonal = Dir >= 4;
				if (bDiagonal && (!IsWalkable(Map, Cx + StepX[Dir], Cy) || !IsWalkable(Map, Cx, Cy + StepY[Dir])))
				{
					continue; // no se atajan esquinas
				}
				const int32 Next = IndexOf(Nx, Ny);
				const size_t Sn = static_cast<size_t>(Next);
				const int32 NewCost = Cost[Sc] + (bDiagonal ? 14 : 10);
				if (!bClosed[Sn] && NewCost < Cost[Sn])
				{
					Cost[Sn] = NewCost;
					Parent[Sn] = Current;
					Open.push_back({NewCost + Heuristic(Nx, Ny), Next});
					std::push_heap(Open.begin(), Open.end(), Worse);
				}
			}
		}
		if (!bFound)
		{
			return false;
		}

		std::vector<TilePoint> Raw;
		for (int32 At = Goal; At != Start; At = Parent[static_cast<size_t>(At)])
		{
			Raw.push_back({At % W, At / W});
		}
		std::reverse(Raw.begin(), Raw.end());

		if (!bSmooth)
		{
			OutPath = Raw;
			return true;
		}
		TilePoint Anchor = From;
		size_t Index = 0;
		while (Index < Raw.size())
		{
			// Avanza hasta el punto más lejano visible en línea recta desde el ancla.
			size_t Farthest = Index;
			for (size_t Probe = Raw.size() - 1; Probe > Index; --Probe)
			{
				if (HasClearLine(Map, Anchor, Raw[Probe]))
				{
					Farthest = Probe;
					break;
				}
			}
			Anchor = Raw[Farthest];
			OutPath.push_back(Anchor);
			Index = Farthest + 1;
		}
		return true;
	}
}
