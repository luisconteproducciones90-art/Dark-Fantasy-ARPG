#include "ArpgCore/Combat/Damage.h"
#include "TestFramework.h"

using namespace arpg;

namespace
{
	constexpr int Phys = static_cast<int>(EDamageType::Physical);
	constexpr int Fire = static_cast<int>(EDamageType::Fire);

	AttackProfile SureHit(int32 Min, int32 Max, int Type)
	{
		AttackProfile A;
		A.Damage[Type] = {Min, Max};
		A.bIsSpell = true; // sin tirada de impacto
		A.CritChance = 0.0f;
		return A;
	}
}

ARPG_TEST(Combat_HitChanceIsClampedAndMonotonic)
{
	EXPECT_EQ(ComputeHitChance(0, 1000, 1, 1), MinHitChance);
	EXPECT_EQ(ComputeHitChance(100000, 1, 50, 1), MaxHitChance);
	EXPECT_EQ(ComputeHitChance(0, 0, 1, 1), MaxHitChance);
	EXPECT_TRUE(ComputeHitChance(200, 100, 10, 10) > ComputeHitChance(100, 100, 10, 10));
	EXPECT_TRUE(ComputeHitChance(100, 100, 10, 10) > ComputeHitChance(100, 200, 10, 10));
	EXPECT_TRUE(ComputeHitChance(100, 100, 15, 10) > ComputeHitChance(100, 100, 10, 15));
}

ARPG_TEST(Combat_ObservedHitRateMatchesFormula)
{
	AttackProfile A;
	A.Damage[Phys] = {10, 10};
	A.AttackRating = 300; A.Level = 10; A.CritChance = 0.0f;
	DefenseProfile D;
	D.Defense = 250; D.Level = 10;
	const float Expected = ComputeHitChance(300, 250, 10, 10); // 0.6
	Rng R(2024);
	int Hits = 0;
	const int N = 40000;
	for (int i = 0; i < N; ++i) { Hits += ResolveHit(A, D, R).Outcome == EHitOutcome::Hit ? 1 : 0; }
	const float Rate = static_cast<float>(Hits) / static_cast<float>(N);
	EXPECT_TRUE(Rate > Expected - 0.015f && Rate < Expected + 0.015f);
}

ARPG_TEST(Combat_ResistancesCapAndVulnerability)
{
	Rng R(1);
	DefenseProfile D;
	D.ResistPercent[Fire] = 50;
	EXPECT_EQ(ResolveHit(SureHit(100, 100, Fire), D, R).Total, 50);
	D.ResistPercent[Fire] = 200; // se satura al 75%
	EXPECT_EQ(ResolveHit(SureHit(100, 100, Fire), D, R).Total, 25);
	D.ResistPercent[Fire] = -500; // se satura al -100%
	EXPECT_EQ(ResolveHit(SureHit(100, 100, Fire), D, R).Total, 200);
}

ARPG_TEST(Combat_ArmorOnlyReducesPhysical)
{
	Rng R(1);
	DefenseProfile D;
	D.Armor = 60; // 60 / (60 + 50 + 10*1) = 50%
	EXPECT_EQ(ResolveHit(SureHit(100, 100, Phys), D, R).Total, 50);
	EXPECT_EQ(ResolveHit(SureHit(100, 100, Fire), D, R).Total, 100);
	EXPECT_EQ(ComputeArmorReduction(1000000, 1), MaxArmorReduction);
	EXPECT_EQ(ComputeArmorReduction(-5, 1), 0.0f);
	EXPECT_TRUE(ComputeArmorReduction(60, 30) < ComputeArmorReduction(60, 1));
}

ARPG_TEST(Combat_CriticalMultipliesDamage)
{
	Rng R(1);
	AttackProfile A = SureHit(100, 100, Phys);
	A.CritChance = 1.0f; A.CritMultiplier = 2.0f;
	const HitResult H = ResolveHit(A, DefenseProfile{}, R);
	EXPECT_TRUE(H.Outcome == EHitOutcome::Critical);
	EXPECT_EQ(H.Total, 200);
}

ARPG_TEST(Combat_BlockStopsWeaponHitsButNotSpells)
{
	Rng R(77);
	AttackProfile A;
	A.Damage[Phys] = {10, 20};
	A.AttackRating = 100000; A.Level = 50; A.CritChance = 0.0f;
	DefenseProfile D;
	D.BlockChance = 5.0f; // se satura al 75%
	int Blocked = 0, Missed = 0;
	const int N = 20000;
	for (int i = 0; i < N; ++i)
	{
		const HitResult H = ResolveHit(A, D, R);
		Blocked += H.Outcome == EHitOutcome::Blocked ? 1 : 0;
		Missed += H.Outcome == EHitOutcome::Miss ? 1 : 0;
		if (H.Outcome != EHitOutcome::Hit) { EXPECT_EQ(H.Total, 0); }
		else { EXPECT_TRUE(H.Total >= 10 && H.Total <= 20); }
	}
	// Esperado: 95% impacta * 75% bloqueo = 71.25%
	const float BlockRate = static_cast<float>(Blocked) / static_cast<float>(N);
	EXPECT_TRUE(BlockRate > 0.69f && BlockRate < 0.735f);
	EXPECT_TRUE(Missed > 0);

	A.bIsSpell = true;
	for (int i = 0; i < 200; ++i) { EXPECT_TRUE(ResolveHit(A, D, R).Outcome == EHitOutcome::Hit); }
}

ARPG_TEST(Combat_MixedDamageSumsPerType)
{
	Rng R(3);
	AttackProfile A = SureHit(40, 40, Phys);
	A.Damage[Fire] = {60, 60};
	DefenseProfile D;
	D.ResistPercent[Fire] = 50;
	const HitResult H = ResolveHit(A, D, R);
	EXPECT_EQ(H.Damage[Phys], 40);
	EXPECT_EQ(H.Damage[Fire], 30);
	EXPECT_EQ(H.Total, 70);
}

ARPG_TEST(Combat_SameSeedSameFight)
{
	AttackProfile A;
	A.Damage[Phys] = {5, 50};
	A.AttackRating = 150; A.Level = 5; A.CritChance = 0.2f;
	DefenseProfile D;
	D.Defense = 120; D.Level = 6; D.BlockChance = 0.2f; D.Armor = 30;
	Rng R1(555), R2(555);
	for (int i = 0; i < 2000; ++i)
	{
		const HitResult H1 = ResolveHit(A, D, R1), H2 = ResolveHit(A, D, R2);
		EXPECT_TRUE(H1.Outcome == H2.Outcome && H1.Total == H2.Total);
	}
}
