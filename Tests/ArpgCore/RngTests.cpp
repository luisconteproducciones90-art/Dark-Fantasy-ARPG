#include "ArpgCore/Core/Rng.h"
#include "TestFramework.h"

using namespace arpg;

ARPG_TEST(Rng_SameSeedSameSequence)
{
	Rng A(1234, 7), B(1234, 7);
	for (int i = 0; i < 1000; ++i) { EXPECT_EQ(A.NextU32(), B.NextU32()); }
}

ARPG_TEST(Rng_MatchesPcg32ReferenceVector)
{
	// Vector de referencia de pcg32 (semilla 42, flujo 54). Si esto cambia,
	// se rompen las semillas de mapas y botín ya guardadas.
	Rng R(42, 54);
	const uint32 Expected[] = {0xa15c02b7u, 0x7b47f409u, 0xba1d3330u, 0x83d2f293u, 0xbfa4784bu, 0xcbed606eu};
	for (uint32 Value : Expected) { EXPECT_EQ(R.NextU32(), Value); }
}

ARPG_TEST(Rng_DifferentSeedsDiverge)
{
	Rng A(1), B(2);
	int Equal = 0;
	for (int i = 0; i < 100; ++i) { Equal += A.NextU32() == B.NextU32() ? 1 : 0; }
	EXPECT_TRUE(Equal < 3);
}

ARPG_TEST(Rng_RangeIntStaysInBoundsAndCoversRange)
{
	Rng R(99);
	bool Seen[6] = {};
	for (int i = 0; i < 5000; ++i)
	{
		const int32 V = R.RangeInt(-2, 3);
		EXPECT_TRUE(V >= -2 && V <= 3);
		if (V >= -2 && V <= 3) { Seen[V + 2] = true; }
	}
	for (bool bSeen : Seen) { EXPECT_TRUE(bSeen); }
	EXPECT_EQ(R.RangeInt(5, 5), 5);
	EXPECT_EQ(R.RangeInt(9, 3), 9);
	const int32 Full = R.RangeInt(INT32_MIN, INT32_MAX); // no debe colgarse ni desbordar
	(void)Full;
}

ARPG_TEST(Rng_FloatInUnitIntervalAndChanceEdges)
{
	Rng R(5);
	int Hits = 0;
	for (int i = 0; i < 20000; ++i)
	{
		const float F = R.NextFloat();
		EXPECT_TRUE(F >= 0.0f && F < 1.0f);
		Hits += R.Chance(0.3f) ? 1 : 0;
	}
	EXPECT_TRUE(Hits > 5600 && Hits < 6400); // 30% ± margen amplio
	EXPECT_TRUE(!R.Chance(0.0f) && !R.Chance(-1.0f));
	EXPECT_TRUE(R.Chance(1.0f) && R.Chance(2.0f));
}
