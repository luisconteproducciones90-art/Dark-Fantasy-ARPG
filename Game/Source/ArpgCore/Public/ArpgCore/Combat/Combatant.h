#pragma once

#include "ArpgCore/Combat/Damage.h"
#include "ArpgCore/Combat/StatusEffects.h"
#include "ArpgCore/Core/Rng.h"

namespace arpg
{
	/** Estado vivo de cualquier criatura que pelea: jugador, monstruo o invocación. */
	struct Combatant
	{
		int32 MaxLife = 1;
		int32 MaxMana = 0;
		int32 Life = 1;
		int32 Mana = 0;
		int32 LifeRegenMilliPerSecond = 0; // 1000 = 1 de vida por segundo
		int32 ManaRegenMilliPerSecond = 0;
		int32 LifeRegenRemainderMilli = 0;
		int32 ManaRegenRemainderMilli = 0;
		StatusContainer Statuses;

		bool IsAlive() const { return Life > 0; }
	};

	/** Deja la criatura con vida y maná al máximo y sin estados. */
	void InitCombatant(Combatant& Target, int32 MaxLife, int32 MaxMana);

	/** Cambia los máximos (p. ej. al equipar) conservando los valores actuales dentro del nuevo rango. */
	void SetMaxValues(Combatant& Target, int32 MaxLife, int32 MaxMana);

	/** Devuelve el daño realmente aplicado. Al morir se limpian los estados. */
	int32 ApplyDamage(Combatant& Target, int32 Amount);
	/** Devuelve lo realmente curado. No revive. */
	int32 Heal(Combatant& Target, int32 Amount);
	int32 RestoreMana(Combatant& Target, int32 Amount);
	/** false (sin gastar) si no hay maná suficiente o la criatura está muerta. */
	bool SpendMana(Combatant& Target, int32 Cost);

	struct TickResult
	{
		int32 StatusDamage = 0;
		bool bDied = false;
	};

	/** Avanza estados y regeneración. El veneno anula la regeneración de vida. */
	TickResult TickCombatant(Combatant& Target, int32 DeltaMs);

	/** Estado que un ataque puede aplicar al impactar. */
	struct OnHitStatus
	{
		EStatusEffect Type = EStatusEffect::Burn;
		int32 ChancePermille = 0;
		int32 DurationMs = 0;
		int32 DamagePerSecond = 0;
		int32 Magnitude = 0;
	};

	struct AttackOutcome
	{
		HitResult Hit;
		int32 DamageDealt = 0;
		bool bKilled = false;
		bool bStatusApplied = false;
	};

	/**
	 * Ataque completo contra un objetivo: resuelve el golpe, aplica el daño y,
	 * si impacta y el objetivo sobrevive, intenta aplicar el estado.
	 * StatusDurationScalePercent: 100 normal; los jefes usan menos.
	 */
	AttackOutcome PerformAttack(const AttackProfile& Attacker, const DefenseProfile& Defender, Combatant& Target,
		Rng& Random, const OnHitStatus* Status = nullptr, int32 StatusDurationScalePercent = 100);
}
