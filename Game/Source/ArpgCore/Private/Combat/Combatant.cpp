#include "ArpgCore/Combat/Combatant.h"

namespace arpg
{
	namespace
	{
		// Suma regeneración con resto fraccionario; devuelve unidades enteras ganadas.
		int32 Accumulate(int32 MilliPerSecond, int32 DeltaMs, int32& RemainderMilli)
		{
			if (MilliPerSecond <= 0)
			{
				return 0;
			}
			// milli-unidades = (milli/s) × ms / 1000; se guarda el resto en micro-unidades / 1000.
			const int64 Micro = static_cast<int64>(MilliPerSecond) * DeltaMs + RemainderMilli;
			const int64 Whole = Micro / 1000000;
			RemainderMilli = static_cast<int32>(Micro % 1000000);
			return static_cast<int32>(Clamp<int64>(Whole, 0, INT32_MAX));
		}
	}

	void InitCombatant(Combatant& Target, int32 MaxLife, int32 MaxMana)
	{
		Target = Combatant{};
		Target.MaxLife = MaxLife < 1 ? 1 : MaxLife;
		Target.MaxMana = MaxMana < 0 ? 0 : MaxMana;
		Target.Life = Target.MaxLife;
		Target.Mana = Target.MaxMana;
	}

	void SetMaxValues(Combatant& Target, int32 MaxLife, int32 MaxMana)
	{
		Target.MaxLife = MaxLife < 1 ? 1 : MaxLife;
		Target.MaxMana = MaxMana < 0 ? 0 : MaxMana;
		if (Target.Life > Target.MaxLife) { Target.Life = Target.MaxLife; }
		if (Target.Mana > Target.MaxMana) { Target.Mana = Target.MaxMana; }
	}

	int32 ApplyDamage(Combatant& Target, int32 Amount)
	{
		if (Amount <= 0 || !Target.IsAlive())
		{
			return 0;
		}
		const int32 Dealt = Amount < Target.Life ? Amount : Target.Life;
		Target.Life -= Dealt;
		if (!Target.IsAlive())
		{
			Target.Statuses.Clear();
		}
		return Dealt;
	}

	int32 Heal(Combatant& Target, int32 Amount)
	{
		if (Amount <= 0 || !Target.IsAlive())
		{
			return 0;
		}
		const int32 Missing = Target.MaxLife - Target.Life;
		const int32 Healed = Amount < Missing ? Amount : Missing;
		Target.Life += Healed;
		return Healed;
	}

	int32 RestoreMana(Combatant& Target, int32 Amount)
	{
		if (Amount <= 0 || !Target.IsAlive())
		{
			return 0;
		}
		const int32 Missing = Target.MaxMana - Target.Mana;
		const int32 Restored = Amount < Missing ? Amount : Missing;
		Target.Mana += Restored;
		return Restored;
	}

	bool SpendMana(Combatant& Target, int32 Cost)
	{
		if (Cost < 0 || !Target.IsAlive() || Target.Mana < Cost)
		{
			return false;
		}
		Target.Mana -= Cost;
		return true;
	}

	TickResult TickCombatant(Combatant& Target, int32 DeltaMs)
	{
		TickResult Result;
		if (DeltaMs <= 0 || !Target.IsAlive())
		{
			return Result;
		}
		const bool bRegenBlocked = Target.Statuses.BlocksRegeneration();
		Result.StatusDamage = ApplyDamage(Target, Target.Statuses.Tick(DeltaMs));
		if (!Target.IsAlive())
		{
			Result.bDied = true;
			return Result;
		}
		if (!bRegenBlocked)
		{
			Heal(Target, Accumulate(Target.LifeRegenMilliPerSecond, DeltaMs, Target.LifeRegenRemainderMilli));
		}
		RestoreMana(Target, Accumulate(Target.ManaRegenMilliPerSecond, DeltaMs, Target.ManaRegenRemainderMilli));
		return Result;
	}

	AttackOutcome PerformAttack(const AttackProfile& Attacker, const DefenseProfile& Defender, Combatant& Target,
		Rng& Random, const OnHitStatus* Status, int32 StatusDurationScalePercent)
	{
		AttackOutcome Outcome;
		if (!Target.IsAlive())
		{
			return Outcome;
		}
		Outcome.Hit = ResolveHit(Attacker, Defender, Random);
		if (Outcome.Hit.Outcome != EHitOutcome::Hit && Outcome.Hit.Outcome != EHitOutcome::Critical)
		{
			return Outcome;
		}
		Outcome.DamageDealt = ApplyDamage(Target, Outcome.Hit.Total);
		Outcome.bKilled = !Target.IsAlive();
		if (!Outcome.bKilled && Status != nullptr
			&& Random.Chance(static_cast<float>(Clamp(Status->ChancePermille, 0, 1000)) / 1000.0f))
		{
			Target.Statuses.Apply(Status->Type, Status->DurationMs, Status->DamagePerSecond, Status->Magnitude,
				StatusDurationScalePercent);
			Outcome.bStatusApplied = Target.Statuses.Has(Status->Type);
		}
		return Outcome;
	}
}
