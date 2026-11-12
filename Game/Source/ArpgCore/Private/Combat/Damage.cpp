#include "ArpgCore/Combat/Damage.h"

namespace arpg
{
	float ComputeHitChance(int32 AttackRating, int32 Defense, int32 AttackerLevel, int32 DefenderLevel)
	{
		const float Ar = static_cast<float>(AttackRating < 0 ? 0 : AttackRating);
		const float Def = static_cast<float>(Defense < 0 ? 0 : Defense);
		const float Denominator = Ar + Def * 0.8f;
		const float Base = Denominator > 0.0f ? Ar / Denominator : 1.0f;
		const float LevelFactor = 1.0f + 0.02f * static_cast<float>(AttackerLevel - DefenderLevel);
		return Clamp(Base * LevelFactor, MinHitChance, MaxHitChance);
	}

	float ComputeArmorReduction(int32 Armor, int32 AttackerLevel)
	{
		if (Armor <= 0)
		{
			return 0.0f;
		}
		const float A = static_cast<float>(Armor);
		const float Lvl = static_cast<float>(AttackerLevel < 1 ? 1 : AttackerLevel);
		return Clamp(A / (A + 50.0f + 10.0f * Lvl), 0.0f, MaxArmorReduction);
	}

	HitResult ResolveHit(const AttackProfile& Attacker, const DefenseProfile& Defender, Rng& Random)
	{
		HitResult Result;

		if (!Attacker.bIsSpell)
		{
			const float HitChance = ComputeHitChance(
				Attacker.AttackRating, Defender.Defense, Attacker.Level, Defender.Level);
			if (!Random.Chance(HitChance))
			{
				Result.Outcome = EHitOutcome::Miss;
				return Result;
			}
			if (Random.Chance(Clamp(Defender.BlockChance, 0.0f, MaxBlockChance)))
			{
				Result.Outcome = EHitOutcome::Blocked;
				return Result;
			}
		}

		const bool bCritical = Random.Chance(Attacker.CritChance);
		const float CritScale = bCritical ? (Attacker.CritMultiplier < 1.0f ? 1.0f : Attacker.CritMultiplier) : 1.0f;
		Result.Outcome = bCritical ? EHitOutcome::Critical : EHitOutcome::Hit;

		const float ArmorReduction = ComputeArmorReduction(Defender.Armor, Attacker.Level);

		for (int Type = 0; Type < DamageTypeCount; ++Type)
		{
			const DamageRange& Range = Attacker.Damage[Type];
			if (Range.Max <= 0)
			{
				continue;
			}
			float Amount = static_cast<float>(Random.RangeInt(Range.Min < 0 ? 0 : Range.Min, Range.Max)) * CritScale;

			if (Type == static_cast<int>(EDamageType::Physical))
			{
				Amount *= 1.0f - ArmorReduction;
			}
			else
			{
				const int32 Resist = Clamp(Defender.ResistPercent[Type], MinResistPercent, MaxResistPercent);
				Amount *= 1.0f - static_cast<float>(Resist) / 100.0f;
			}

			const int32 Final = Amount < 0.0f ? 0 : static_cast<int32>(Amount);
			Result.Damage[Type] = Final;
			Result.Total += Final;
		}
		return Result;
	}
}
