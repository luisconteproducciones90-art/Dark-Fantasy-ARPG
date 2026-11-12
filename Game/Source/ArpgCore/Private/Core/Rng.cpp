#include "ArpgCore/Core/Rng.h"

namespace arpg
{
	Rng::Rng(uint64 Seed, uint64 Stream)
		: State(0)
		, Increment((Stream << 1u) | 1u)
	{
		NextU32();
		State += Seed;
		NextU32();
	}

	uint32 Rng::NextU32()
	{
		const uint64 Old = State;
		State = Old * 6364136223846793005ULL + Increment;
		const uint32 XorShifted = static_cast<uint32>(((Old >> 18u) ^ Old) >> 27u);
		const uint32 Rot = static_cast<uint32>(Old >> 59u);
		return (XorShifted >> Rot) | (XorShifted << ((32u - Rot) & 31u));
	}

	int32 Rng::RangeInt(int32 Lo, int32 Hi)
	{
		if (Hi <= Lo)
		{
			return Lo;
		}
		const uint32 Span = static_cast<uint32>(static_cast<int64>(Hi) - static_cast<int64>(Lo)) + 1u;
		if (Span == 0u) // rango completo de 32 bits
		{
			return static_cast<int32>(NextU32());
		}
		// Rechazo para eliminar el sesgo del módulo.
		const uint32 Threshold = (0u - Span) % Span;
		uint32 Value = NextU32();
		while (Value < Threshold)
		{
			Value = NextU32();
		}
		return static_cast<int32>(static_cast<int64>(Lo) + static_cast<int64>(Value % Span));
	}

	float Rng::NextFloat()
	{
		// 24 bits => representable de forma exacta en float.
		return static_cast<float>(NextU32() >> 8u) * (1.0f / 16777216.0f);
	}

	bool Rng::Chance(float Probability)
	{
		if (Probability <= 0.0f)
		{
			return false;
		}
		if (Probability >= 1.0f)
		{
			return true;
		}
		return NextFloat() < Probability;
	}
}
