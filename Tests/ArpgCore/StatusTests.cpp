#include "ArpgCore/Combat/StatusEffects.h"
#include "TestFramework.h"

using namespace arpg;

ARPG_TEST(Status_DotIsFrameRateIndependent)
{
	// 7 de daño/s durante 3 s = 21, se tique como se tique.
	const int32 Steps[] = {16, 33, 100, 250, 3000, 7};
	for (int32 Step : Steps)
	{
		StatusContainer S;
		S.Apply(EStatusEffect::Burn, 3000, 7);
		int32 Total = 0;
		for (int Elapsed = 0; Elapsed < 6000; Elapsed += Step) { Total += S.Tick(Step); }
		EXPECT_EQ(Total, 21);
		EXPECT_TRUE(!S.Has(EStatusEffect::Burn));
	}
}

ARPG_TEST(Status_ReapplyKeepsStrongestAndDoesNotStack)
{
	StatusContainer S;
	S.Apply(EStatusEffect::Poison, 4000, 10);
	S.Apply(EStatusEffect::Poison, 1000, 30); // más fuerte pero más corto
	EXPECT_EQ(S.RemainingMs(EStatusEffect::Poison), 4000);
	EXPECT_EQ(S.Tick(1000), 30);
	S.Apply(EStatusEffect::Poison, 500, 5); // más débil: no cambia nada
	EXPECT_EQ(S.RemainingMs(EStatusEffect::Poison), 3000);
	EXPECT_EQ(S.Tick(1000), 30);
}

ARPG_TEST(Status_ControlEffects)
{
	StatusContainer S;
	EXPECT_TRUE(S.CanAct());
	EXPECT_EQ(S.MoveSpeedPercent(), 100);

	S.Apply(EStatusEffect::Chill, 2000, 0, 40);
	EXPECT_EQ(S.MoveSpeedPercent(), 60);
	S.Apply(EStatusEffect::Chill, 2000, 0, 500); // se satura
	EXPECT_EQ(S.MoveSpeedPercent(), 100 - MaxChillSlowPercent);

	S.Apply(EStatusEffect::Stun, 500);
	EXPECT_TRUE(!S.CanAct());
	EXPECT_EQ(S.MoveSpeedPercent(), 0);
	S.Tick(500);
	EXPECT_TRUE(S.CanAct());
	EXPECT_TRUE(S.Has(EStatusEffect::Chill));

	S.Apply(EStatusEffect::Freeze, 300);
	EXPECT_TRUE(!S.CanAct());
	S.Remove(EStatusEffect::Freeze);
	EXPECT_TRUE(S.CanAct());
}

ARPG_TEST(Status_PoisonBlocksRegenAndCurseLowersResists)
{
	StatusContainer S;
	EXPECT_TRUE(!S.BlocksRegeneration());
	EXPECT_EQ(S.ResistPenalty(), 0);
	S.Apply(EStatusEffect::Poison, 1000, 1);
	S.Apply(EStatusEffect::Curse, 1000, 0, 25);
	EXPECT_TRUE(S.BlocksRegeneration());
	EXPECT_EQ(S.ResistPenalty(), 25);
	S.Clear();
	EXPECT_TRUE(!S.BlocksRegeneration());
	EXPECT_EQ(S.ResistPenalty(), 0);
}

ARPG_TEST(Status_BossDurationScaleAndInvalidInput)
{
	StatusContainer S;
	S.Apply(EStatusEffect::Stun, 2000, 0, 0, 40);
	EXPECT_EQ(S.RemainingMs(EStatusEffect::Stun), 800);
	S.Clear();
	S.Apply(EStatusEffect::Stun, 2000, 0, 0, 0); // inmune
	S.Apply(EStatusEffect::Burn, -50, 10);
	S.Apply(EStatusEffect::Count, 1000, 10);
	EXPECT_TRUE(S.CanAct() && !S.Has(EStatusEffect::Burn));
	EXPECT_EQ(S.Tick(1000), 0);
	S.Apply(EStatusEffect::Burn, 1000, 10);
	EXPECT_EQ(S.Tick(0), 0);
	EXPECT_EQ(S.Tick(-100), 0);
	EXPECT_EQ(S.RemainingMs(EStatusEffect::Burn), 1000);
	S.Apply(EStatusEffect::Burn, INT32_MAX, INT32_MAX, 0, 1000); // no desborda
	EXPECT_TRUE(S.Tick(INT32_MAX) >= 0);
}
