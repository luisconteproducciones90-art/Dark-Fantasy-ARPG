#pragma once

#include "ArpgCore/Character/CharacterStats.h"
#include "ArpgCore/Combat/Combatant.h"

namespace arpg
{
	/** Habilidad activa. En la Fase 8 se amplía a árboles completos cargados de datos. */
	struct SkillDef
	{
		const char* Id;
		ECharacterClass Class;
		int32 ManaCost;
		int32 CooldownMs;
		int32 RangeCm;              // distancia máxima al objetivo o al centro del área
		int32 RadiusCm;             // 0 = un solo objetivo; > 0 = área
		int32 WeaponDamagePercent;  // parte del daño del arma que conserva (0 = ninguna)
		EDamageType Element;        // tipo del daño propio de la habilidad
		DamageRange ElementDamage;  // a nivel 1
		int32 ElementGrowthPercent; // por nivel de personaje
		bool bIsSpell;              // los hechizos no fallan ni se bloquean
		OnHitStatus Status;
	};

	inline constexpr int MaxSkills = 32;

	int32 GetSkillCount();
	const SkillDef* GetSkill(int32 SkillIndex);
	/** Habilidad inicial de la clase, o -1. */
	int32 GetClassSkill(ECharacterClass Class);

	/** Perfil de ataque de la habilidad a partir del ataque básico del personaje. */
	AttackProfile BuildSkillAttack(const SkillDef& Skill, const AttackProfile& BasicAttack, int32 CharacterLevel);

	struct SkillCooldowns
	{
		int32 RemainingMs[MaxSkills] = {};

		void Tick(int32 DeltaMs);
		bool IsReady(int32 SkillIndex) const;
	};

	enum class ESkillUseResult : uint8 { Ok, InvalidSkill, Dead, CannotAct, OnCooldown, NotEnoughMana };

	/** Comprueba todo y, solo si devuelve Ok, gasta el maná e inicia el enfriamiento. */
	ESkillUseResult TryUseSkill(int32 SkillIndex, Combatant& Caster, SkillCooldowns& Cooldowns);
}
