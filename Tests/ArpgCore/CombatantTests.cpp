#include "ArpgCore/Combat/Combatant.h"
#include "TestFramework.h"

using namespace arpg;

ARPG_TEST(Combatant_DamageHealAndManaAreClamped)
{
	Combatant C;
	InitCombatant(C, 100, 50);
	EXPECT_EQ(ApplyDamage(C, 30), 30);
	EXPECT_EQ(ApplyDamage(C, -10), 0);
	EXPECT_EQ(Heal(C, 1000), 30);
	EXPECT_EQ(C.Life, 100);
	EXPECT_TRUE(!SpendMana(C, 51) && C.Mana == 50);
	EXPECT_TRUE(!SpendMana(C, -5));
	EXPECT_TRUE(SpendMana(C, 50) && C.Mana == 0);
	EXPECT_EQ(RestoreMana(C, 999), 50);
	InitCombatant(C, -5, -5);
	EXPECT_TRUE(C.MaxLife == 1 && C.MaxMana == 0 && C.IsAlive());
}

ARPG_TEST(Combatant_DeathIsFinalAndClearsStatuses)
{
	Combatant C;
	InitCombatant(C, 40, 10);
	C.Statuses.Apply(EStatusEffect::Burn, 5000, 10);
	EXPECT_EQ(ApplyDamage(C, 9999), 40);
	EXPECT_TRUE(!C.IsAlive() && !C.Statuses.Has(EStatusEffect::Burn));
	EXPECT_EQ(Heal(C, 50), 0);       // curar no revive
	EXPECT_EQ(ApplyDamage(C, 5), 0);
	EXPECT_TRUE(!SpendMana(C, 1));
	C.LifeRegenMilliPerSecond = 5000;
	TickCombatant(C, 10000);
	EXPECT_EQ(C.Life, 0);
}

ARPG_TEST(Combatant_RegenIsFrameRateIndependentAndPoisonBlocksLife)
{
	const int32 Steps[] = {16, 33, 250, 4000};
	for (int32 Step : Steps)
	{
		Combatant C;
		InitCombatant(C, 100, 100);
		C.Life = 10; C.Mana = 10;
		C.LifeRegenMilliPerSecond = 2500; // 2,5 por segundo
		C.ManaRegenMilliPerSecond = 1500;
		for (int Elapsed = 0; Elapsed < 4000; Elapsed += Step) { TickCombatant(C, Step); }
		const int32 Seconds = (4000 + Step - 1) / Step * Step; // tiempo realmente simulado
		EXPECT_EQ(C.Life, 10 + 2500 * Seconds / 1000000);
		EXPECT_EQ(C.Mana, 10 + 1500 * Seconds / 1000000);
	}
	Combatant P;
	InitCombatant(P, 100, 100);
	P.Life = 50; P.Mana = 50;
	P.LifeRegenMilliPerSecond = 10000; P.ManaRegenMilliPerSecond = 10000;
	P.Statuses.Apply(EStatusEffect::Poison, 2000, 5);
	const TickResult R = TickCombatant(P, 2000);
	EXPECT_EQ(R.StatusDamage, 10);
	EXPECT_EQ(P.Life, 40);  // sin regeneración de vida mientras dura el veneno
	EXPECT_EQ(P.Mana, 70);  // el maná sí regenera
	TickCombatant(P, 1000);
	EXPECT_EQ(P.Life, 50);
}

ARPG_TEST(Combatant_DotCanKill)
{
	Combatant C;
	InitCombatant(C, 20, 0);
	C.Statuses.Apply(EStatusEffect::Burn, 10000, 15);
	EXPECT_TRUE(!TickCombatant(C, 1000).bDied);
	const TickResult R = TickCombatant(C, 1000);
	EXPECT_TRUE(R.bDied && R.StatusDamage == 5 && !C.IsAlive());
}

ARPG_TEST(Combatant_SetMaxValuesKeepsCurrentInRange)
{
	Combatant C;
	InitCombatant(C, 100, 100);
	SetMaxValues(C, 60, 40);
	EXPECT_TRUE(C.Life == 60 && C.Mana == 40);
	SetMaxValues(C, 200, 200);
	EXPECT_TRUE(C.Life == 60 && C.Mana == 40); // subir el máximo no cura
}

ARPG_TEST(Combatant_PerformAttackAppliesDamageAndStatus)
{
	AttackProfile A;
	A.Damage[static_cast<int>(EDamageType::Fire)] = {10, 10};
	A.bIsSpell = true; A.CritChance = 0.0f;
	const DefenseProfile D;
	const OnHitStatus Burn{EStatusEffect::Burn, 1000, 4000, 3, 0};
	Rng R(5);

	Combatant C;
	InitCombatant(C, 100, 0);
	const AttackOutcome First = PerformAttack(A, D, C, R, &Burn);
	EXPECT_TRUE(First.DamageDealt == 10 && !First.bKilled && First.bStatusApplied);
	EXPECT_EQ(C.Statuses.RemainingMs(EStatusEffect::Burn), 4000);

	Combatant Boss;
	InitCombatant(Boss, 100, 0);
	const OnHitStatus Stun{EStatusEffect::Stun, 1000, 2000, 0, 0};
	PerformAttack(A, D, Boss, R, &Stun, 40);
	EXPECT_EQ(Boss.Statuses.RemainingMs(EStatusEffect::Stun), 800);
	Combatant Immune;
	InitCombatant(Immune, 100, 0);
	EXPECT_TRUE(!PerformAttack(A, D, Immune, R, &Stun, 0).bStatusApplied);

	const OnHitStatus Never{EStatusEffect::Freeze, 0, 2000, 0, 0};
	EXPECT_TRUE(!PerformAttack(A, D, C, R, &Never).bStatusApplied);

	// El golpe mortal no deja estados en el cadáver; atacar a un muerto no hace nada.
	Combatant Weak;
	InitCombatant(Weak, 5, 0);
	const AttackOutcome Kill = PerformAttack(A, D, Weak, R, &Burn);
	EXPECT_TRUE(Kill.bKilled && Kill.DamageDealt == 5 && !Kill.bStatusApplied);
	const AttackOutcome Again = PerformAttack(A, D, Weak, R, &Burn);
	EXPECT_TRUE(Again.DamageDealt == 0 && !Again.bKilled);
}
