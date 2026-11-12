#pragma once

#include "ArpgCore/Core/Types.h"

namespace arpg
{
	/**
	 * Generador determinista PCG32. Misma semilla => misma secuencia en
	 * cualquier plataforma y compilador. Lo usan botín, combate y generación
	 * de mapas; en multijugador solo el servidor posee las semillas.
	 */
	class Rng
	{
	public:
		explicit Rng(uint64 Seed = 0x853c49e6748fea9bULL, uint64 Stream = 1);

		uint32 NextU32();

		/** Entero uniforme en [Lo, Hi] (ambos incluidos). Si Hi < Lo devuelve Lo. */
		int32 RangeInt(int32 Lo, int32 Hi);

		/** Real uniforme en [0, 1). */
		float NextFloat();

		/** true con probabilidad Probability (se satura a [0, 1]). */
		bool Chance(float Probability);

	private:
		uint64 State;
		uint64 Increment;
	};
}
