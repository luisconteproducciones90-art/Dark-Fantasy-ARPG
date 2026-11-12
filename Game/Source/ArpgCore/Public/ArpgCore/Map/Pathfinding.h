#pragma once

#include "ArpgCore/Map/Dungeon.h"

#include <vector>

namespace arpg
{
	struct TilePoint
	{
		int32 X = 0;
		int32 Y = 0;
	};

	inline bool IsWalkable(const Dungeon& Map, int32 X, int32 Y)
	{
		return Map.At(X, Y) == ETile::Floor;
	}

	/**
	 * true si se puede caminar en línea recta entre los centros de dos casillas
	 * sin rozar un muro (deja un margen lateral para el cuerpo del personaje).
	 */
	bool HasClearLine(const Dungeon& Map, TilePoint From, TilePoint To);

	/**
	 * A* en 8 direcciones sin atajar esquinas. OutPath no incluye From e incluye To.
	 * Con bSmooth elimina los puntos intermedios innecesarios (camino con menos zigzag).
	 * Devuelve false (OutPath vacío) si no hay camino o algún extremo no es transitable.
	 */
	bool FindPath(const Dungeon& Map, TilePoint From, TilePoint To, std::vector<TilePoint>& OutPath,
		bool bSmooth = true);
}
