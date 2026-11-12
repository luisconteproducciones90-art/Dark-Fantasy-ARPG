#include "ArpgCore/Skill/Skill.h"
#include "TestFramework.h"

using namespace arpg;

ARPG_TEST(Skill_EveryClassHasAStartingSkill)
{
	for (int i = 0; i < static_cast<int>(ECharacterClass::Count); ++i)
	{
		const int32 Index = GetClassSkill(static_cast<ECharacterClass>(i));
		const SkillDef* Skill = GetSkill(Index);
		EXPECT_TRUE(Skill != nullptr);
		if (Skill == nullptr) { continue; }
		EXPECT_TRUE(Skill->Class == static_cast<ECharacterClass>(i));
		EXPECT_TRUE(Skill->ManaCost > 0 && Skill->CooldownMs > 0 && Skill->RangeCm > 0);
		EXPECT_TRUE(Skill->WeaponDamagePercent > 0 || Skill->ElementDamage.Max > 0); // hace algo de daño
		// Toda clase puede pagar su habilidad inicial a nivel 1.
		const CharacterProgress P = CreateCharacter(static_cast<ECharacterClass>(i));
		EXPECT_TRUE(ComputeBaseStats(P.Class, 1, P.Attr).MaxMana >= Skill->ManaCost);
	}
	EXPECT_TRUE(GetSkill(-1) == nullptr && GetSkill(GetSkillCount()) == nullptr);
}

ARPG_TEST(Skill_BuildAttackScalesWeaponAndElement)
{
	AttackProfile Basic;
	Basic.Damage[0] = {10, 20};
	Basic.AttackRating = 300; Basic.Level = 1;

	const SkillDef& Cleave = *GetSkill(GetClassSkill(ECharacterClass::Warrior));
	const AttackProfile A = BuildSkillAttack(Cleave, Basic, 1);
	EXPECT_TRUE(A.Damage[0].Min == 15 && A.Damage[0].Max == 30 && !A.bIsSpell && A.AttackRating == 300);

	const SkillDef& Bolt = *GetSkill(GetClassSkill(ECharacterClass::Sorcerer));
	const int Fire = static_cast<int>(EDamageType::Fire);
	const AttackProfile B1 = BuildSkillAttack(Bolt, Basic, 1);
	const AttackProfile B20 = BuildSkillAttack(Bolt, Basic, 20);
	EXPECT_TRUE(B1.bIsSpell && B1.Damage[0].Max == 0); // el hechizo no usa el arma
	EXPECT_TRUE(B1.Damage[Fire].Min == 6 && B1.Damage[Fire].Max == 11);
	EXPECT_TRUE(B20.Damage[Fire].Max > B1.Damage[Fire].Max * 4);

	// Un golpe con veneno suma arma y elemento.
	const SkillDef& Venom = *GetSkill(GetClassSkill(ECharacterClass::Assassin));
	const AttackProfile V = BuildSkillAttack(Venom, Basic, 1);
	EXPECT_TRUE(V.Damage[0].Max == 26 && V.Damage[static_cast<int>(EDamageType::Poison)].Max == 4);
}

ARPG_TEST(Skill_UseChecksManaCooldownAndState)
{
	const int32 Index = GetClassSkill(ECharacterClass::Sorcerer); // 5 de maná, 600 ms
	Combatant C;
	InitCombatant(C, 50, 12);
	SkillCooldowns Cd;

	EXPECT_TRUE(TryUseSkill(Index, C, Cd) == ESkillUseResult::Ok);
	EXPECT_EQ(C.Mana, 7);
	EXPECT_TRUE(TryUseSkill(Index, C, Cd) == ESkillUseResult::OnCooldown);
	EXPECT_EQ(C.Mana, 7); // un intento fallido no gasta
	Cd.Tick(599);
	EXPECT_TRUE(!Cd.IsReady(Index));
	Cd.Tick(1);
	EXPECT_TRUE(TryUseSkill(Index, C, Cd) == ESkillUseResult::Ok);
	Cd.Tick(100000);
	EXPECT_TRUE(TryUseSkill(Index, C, Cd) == ESkillUseResult::NotEnoughMana);
	EXPECT_TRUE(Cd.IsReady(Index)); // sin maná no arranca el enfriamiento

	RestoreMana(C, 10);
	C.Statuses.Apply(EStatusEffect::Stun, 500);
	EXPECT_TRUE(TryUseSkill(Index, C, Cd) == ESkillUseResult::CannotAct);
	C.Statuses.Clear();
	ApplyDamage(C, 9999);
	EXPECT_TRUE(TryUseSkill(Index, C, Cd) == ESkillUseResult::Dead);
	EXPECT_TRUE(TryUseSkill(-1, C, Cd) == ESkillUseResult::InvalidSkill);
	EXPECT_TRUE(TryUseSkill(9999, C, Cd) == ESkillUseResult::InvalidSkill);
	EXPECT_TRUE(!Cd.IsReady(-1) && !Cd.IsReady(MaxSkills));
	Cd.Tick(-5);
}
