#include "ArpgCore/Skill/Skill.h"

namespace arpg
{
	namespace
	{
		using Cls = ECharacterClass;
		using Elem = EDamageType;
		using Fx = EStatusEffect;

		constexpr SkillDef SkillTable[] = {
			// Id               Clase             Maná Enfr. Alcance Radio %Arma Elemento        Daño     %Nv  Hechizo  Estado {tipo, ‰, ms, daño/s, magnitud}
			{"cleave",          Cls::Warrior,      6, 1500,  200,   280, 150, Elem::Physical,  {0, 0},    0, false, {Fx::Stun,   150, 1000, 0, 0}},
			{"ember_bolt",      Cls::Sorcerer,     5,  600, 1400,     0,   0, Elem::Fire,      {6, 11},  18, true,  {Fx::Burn,   500, 3000, 3, 0}},
			{"piercing_shot",   Cls::Hunter,       4,  800, 1500,     0, 180, Elem::Physical,  {0, 0},    0, false, {Fx::Chill,  250, 2000, 0, 30}},
			{"bone_spike",      Cls::Necromancer,  5,  700, 1300,     0,   0, Elem::Shadow,    {5, 10},  18, true,  {Fx::Curse,  400, 5000, 0, 20}},
			{"venom_strike",    Cls::Assassin,     4,  900,  200,     0, 130, Elem::Poison,    {2, 4},   15, false, {Fx::Poison, 1000, 4000, 4, 0}},
			{"judgment",        Cls::Paladin,      7, 2000,  200,   240, 120, Elem::Holy,      {4, 8},   15, false, {Fx::Stun,   300, 1200, 0, 0}},
		};
		constexpr int32 SkillCount = static_cast<int32>(sizeof(SkillTable) / sizeof(SkillTable[0]));
		static_assert(sizeof(SkillTable) / sizeof(SkillTable[0]) <= static_cast<unsigned>(MaxSkills));
	}

	int32 GetSkillCount()
	{
		return SkillCount;
	}

	const SkillDef* GetSkill(int32 SkillIndex)
	{
		return SkillIndex >= 0 && SkillIndex < SkillCount ? &SkillTable[SkillIndex] : nullptr;
	}

	int32 GetClassSkill(ECharacterClass Class)
	{
		for (int32 i = 0; i < SkillCount; ++i)
		{
			if (SkillTable[i].Class == Class)
			{
				return i;
			}
		}
		return -1;
	}

	AttackProfile BuildSkillAttack(const SkillDef& Skill, const AttackProfile& BasicAttack, int32 CharacterLevel)
	{
		AttackProfile Out = BasicAttack;
		Out.bIsSpell = Skill.bIsSpell;

		const int64 WeaponPercent = Clamp(Skill.WeaponDamagePercent, 0, 10000);
		for (int Type = 0; Type < DamageTypeCount; ++Type)
		{
			Out.Damage[Type].Min = static_cast<int32>(BasicAttack.Damage[Type].Min * WeaponPercent / 100);
			Out.Damage[Type].Max = static_cast<int32>(BasicAttack.Damage[Type].Max * WeaponPercent / 100);
		}

		const int64 Level = Clamp(CharacterLevel, 1, MaxCharacterLevel);
		const int64 Growth = 100 + static_cast<int64>(Skill.ElementGrowthPercent) * (Level - 1);
		DamageRange& Element = Out.Damage[static_cast<int>(Skill.Element)];
		Element.Min += static_cast<int32>(Skill.ElementDamage.Min * Growth / 100);
		Element.Max += static_cast<int32>(Skill.ElementDamage.Max * Growth / 100);
		return Out;
	}

	void SkillCooldowns::Tick(int32 DeltaMs)
	{
		if (DeltaMs <= 0)
		{
			return;
		}
		for (int32& Remaining : RemainingMs)
		{
			Remaining = Remaining > DeltaMs ? Remaining - DeltaMs : 0;
		}
	}

	bool SkillCooldowns::IsReady(int32 SkillIndex) const
	{
		return SkillIndex >= 0 && SkillIndex < MaxSkills && RemainingMs[SkillIndex] <= 0;
	}

	ESkillUseResult TryUseSkill(int32 SkillIndex, Combatant& Caster, SkillCooldowns& Cooldowns)
	{
		const SkillDef* Skill = GetSkill(SkillIndex);
		if (Skill == nullptr)
		{
			return ESkillUseResult::InvalidSkill;
		}
		if (!Caster.IsAlive())
		{
			return ESkillUseResult::Dead;
		}
		if (!Caster.Statuses.CanAct())
		{
			return ESkillUseResult::CannotAct;
		}
		if (!Cooldowns.IsReady(SkillIndex))
		{
			return ESkillUseResult::OnCooldown;
		}
		if (!SpendMana(Caster, Skill->ManaCost))
		{
			return ESkillUseResult::NotEnoughMana;
		}
		Cooldowns.RemainingMs[SkillIndex] = Skill->CooldownMs;
		return ESkillUseResult::Ok;
	}
}
