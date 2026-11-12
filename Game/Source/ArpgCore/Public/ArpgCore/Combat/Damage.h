#pragma once

#include "ArpgCore/Core/Rng.h"
#include "ArpgCore/Core/Types.h"

namespace arpg
{
	enum class EDamageType : uint8
	{
		Physical,
		Fire,
		Cold,
		Lightning,
		Poison,
		Shadow,
		Holy,
		Count
	};
	inline constexpr int DamageTypeCount = static_cast<int>(EDamageType::Count);

	struct DamageRange
	{
		int32 Min = 0;
		int32 Max = 0;
	};

	/** Todo lo que el atacante aporta a un golpe. */
	struct AttackProfile
	{
		DamageRange Damage[DamageTypeCount];
		int32 AttackRating = 0;
		int32 Level = 1;
		float CritChance = 0.05f;
		float CritMultiplier = 1.5f;
		/** Los hechizos no tiran impacto ni pueden bloquearse. */
		bool bIsSpell = false;
	};

	/** Todo lo que el defensor opone a un golpe. */
	struct DefenseProfile
	{
		int32 Defense = 0;
		int32 Armor = 0;                      // reduce solo daño físico
		int32 Level = 1;
		float BlockChance = 0.0f;
		int32 ResistPercent[DamageTypeCount] = {}; // elementales; Physical se ignora
	};

	enum class EHitOutcome : uint8 { Miss, Blocked, Hit, Critical };

	struct HitResult
	{
		EHitOutcome Outcome = EHitOutcome::Miss;
		int32 Damage[DamageTypeCount] = {};
		int32 Total = 0;
	};

	inline constexpr float MinHitChance = 0.05f;
	inline constexpr float MaxHitChance = 0.95f;
	inline constexpr float MaxBlockChance = 0.75f;
	inline constexpr int32 MaxResistPercent = 75;
	inline constexpr int32 MinResistPercent = -100;
	inline constexpr float MaxArmorReduction = 0.75f;

	/** Probabilidad de impacto de un ataque de arma, ya saturada. */
	float ComputeHitChance(int32 AttackRating, int32 Defense, int32 AttackerLevel, int32 DefenderLevel);

	/** Fracción [0, 0.75] de daño físico absorbida por la armadura. */
	float ComputeArmorReduction(int32 Armor, int32 AttackerLevel);

	/**
	 * Resuelve un golpe completo: impacto, bloqueo, crítico, armadura y
	 * resistencias. Es la única función que decide daño; en multijugador
	 * solo la ejecuta el servidor.
	 */
	HitResult ResolveHit(const AttackProfile& Attacker, const DefenseProfile& Defender, Rng& Random);
}
