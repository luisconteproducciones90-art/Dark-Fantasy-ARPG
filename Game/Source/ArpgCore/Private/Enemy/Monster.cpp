#include "ArpgCore/Enemy/Monster.h"

#include "ArpgCore/Character/CharacterStats.h"

#include <cstring>

namespace arpg
{
	namespace
	{
		using Beh = EMonsterBehavior;
		using Dmg = EDamageType;

		// Acto I. Resistencias: físico, fuego, frío, rayo, veneno, sombra, sagrado.
		constexpr MonsterDef MonsterTable[] = {
			// Id                Conducta        Vida +Nv  Daño    %Nv Tipo         Arm  Resistencias                   XP  Alcance
			{"ash_walker",       Beh::MeleeChaser, 30, 12, {3, 6},   12, Dmg::Physical, 2, {0, 25, -25, 0, 0, 0, -25},      20, 150},
			{"bone_archer",      Beh::RangedKiter, 20,  8, {2, 7},   12, Dmg::Physical, 1, {0, 0, 0, 0, 50, 0, -25},        24, 1200},
			{"mire_boar",        Beh::Charger,     45, 16, {5, 9},   12, Dmg::Physical, 3, {0, 0, 0, 0, 0, 0, 0},           28, 180},
			{"grave_caller",     Beh::Summoner,    25,  9, {3, 5},   12, Dmg::Shadow,   1, {0, 0, 0, 0, 0, 50, -50},        36, 900},
			{"cinder_acolyte",   Beh::Support,     25, 10, {2, 5},   12, Dmg::Fire,     1, {0, 50, -25, 0, 0, 0, 0},        32, 800},
			{"burrower",         Beh::Ambusher,    28, 11, {5, 10},  12, Dmg::Poison,   2, {0, 0, 0, 0, 75, 0, 0},          26, 160},
			{"bloat",            Beh::Exploder,    18,  7, {10, 16}, 14, Dmg::Fire,     0, {0, -25, 0, 0, 0, 0, 0},         22, 200},
		};
		constexpr int32 MonsterCount = static_cast<int32>(sizeof(MonsterTable) / sizeof(MonsterTable[0]));

		// Orden de EMonsterRank: normal, campeón, élite, jefe.
		constexpr int32 RankLifePercent[] = {100, 200, 400, 1500};
		constexpr int32 RankDamagePercent[] = {100, 125, 150, 200};
		constexpr int32 RankExperiencePercent[] = {100, 300, 500, 2000};
		constexpr int32 RankStatusPercent[] = {100, 100, 75, 40};

		// Orden de EDifficulty.
		constexpr int32 DifficultyLifePercent[] = {100, 175, 300};
		constexpr int32 DifficultyDamagePercent[] = {100, 150, 225};
		constexpr int32 DifficultyResistBonus[] = {0, 15, 30};

		constexpr int32 MaxMonsterLevel = 99;

		int32 Saturate(int64 Value)
		{
			return static_cast<int32>(Clamp<int64>(Value, 0, INT32_MAX));
		}
	}

	int32 GetMonsterDefCount()
	{
		return MonsterCount;
	}

	const MonsterDef* GetMonsterDef(int32 DefIndex)
	{
		return DefIndex >= 0 && DefIndex < MonsterCount ? &MonsterTable[DefIndex] : nullptr;
	}

	int32 FindMonsterDef(const char* Id)
	{
		if (Id == nullptr)
		{
			return -1;
		}
		for (int32 i = 0; i < MonsterCount; ++i)
		{
			if (std::strcmp(MonsterTable[i].Id, Id) == 0)
			{
				return i;
			}
		}
		return -1;
	}

	MonsterInstance SpawnMonster(int32 DefIndex, int32 Level, EMonsterRank Rank, EDifficulty Difficulty)
	{
		MonsterInstance Out;
		const MonsterDef* Def = GetMonsterDef(DefIndex);
		if (Def == nullptr)
		{
			return Out;
		}
		const int RankIndex = static_cast<int>(Rank) & 3;
		const int DiffIndex = Difficulty < EDifficulty::Count ? static_cast<int>(Difficulty) : 0;
		const int64 Lvl = Clamp(Level, 1, MaxMonsterLevel);

		Out.DefIndex = DefIndex;
		Out.Level = static_cast<int32>(Lvl);
		Out.Rank = Rank;
		Out.StatusDurationPercent = RankStatusPercent[RankIndex];

		const int64 Life = (Def->BaseLife + Def->LifePerLevel * (Lvl - 1))
			* RankLifePercent[RankIndex] / 100 * DifficultyLifePercent[DiffIndex] / 100;
		InitCombatant(Out.Body, Saturate(Life), 0);

		const int64 DamageScale = (100 + Def->DamageGrowthPercent * (Lvl - 1))
			* RankDamagePercent[RankIndex] / 100 * DifficultyDamagePercent[DiffIndex] / 100; // en %
		DamageRange& Damage = Out.Attack.Damage[static_cast<int>(Def->DamageType)];
		Damage.Min = Saturate(Def->BaseDamage.Min * DamageScale / 100);
		Damage.Max = Saturate(Def->BaseDamage.Max * DamageScale / 100);
		Out.Attack.AttackRating = Saturate(40 + 12 * Lvl);
		Out.Attack.Level = Out.Level;
		Out.Attack.bIsSpell = Def->DamageType != EDamageType::Physical;

		Out.Defense.Defense = Saturate(10 + 6 * Lvl);
		Out.Defense.Armor = Saturate(Def->ArmorPerLevel * Lvl);
		Out.Defense.Level = Out.Level;
		for (int Type = 0; Type < DamageTypeCount; ++Type)
		{
			Out.Defense.ResistPercent[Type] = Def->ResistPercent[Type] + DifficultyResistBonus[DiffIndex];
		}

		const int64 Xp = Def->BaseExperience * (100 + 25 * (Lvl - 1)) / 100
			* RankExperiencePercent[RankIndex] / 100 * DifficultyLifePercent[DiffIndex] / 100;
		Out.Experience = Saturate(Xp);
		return Out;
	}

	int64 ComputeKillExperience(int32 MonsterExperience, int32 PlayerLevel, int32 MonsterLevel)
	{
		if (MonsterExperience <= 0)
		{
			return 0;
		}
		const int32 Over = PlayerLevel - MonsterLevel - 5;
		const int32 Percent = Over <= 0 ? 100 : Clamp(100 - Over * 15, 5, 100);
		const int64 Result = static_cast<int64>(MonsterExperience) * Percent / 100;
		return Result < 1 ? 1 : Result;
	}

	LootDrop RollLoot(const MonsterInstance& Monster, int32 MagicFindPercent, Rng& Random)
	{
		LootDrop Drop;
		if (GetMonsterDef(Monster.DefIndex) == nullptr)
		{
			return Drop;
		}
		const int RankIndex = static_cast<int>(Monster.Rank) & 3;

		constexpr int32 GoldChancePermille[] = {600, 1000, 1000, 1000};
		constexpr int32 GoldPercent[] = {100, 250, 400, 1500};
		if (Random.RangeInt(0, 999) < GoldChancePermille[RankIndex])
		{
			const int64 Gold = static_cast<int64>(Random.RangeInt(2, 6)) * Monster.Level * GoldPercent[RankIndex] / 100;
			Drop.Gold = Saturate(Gold < 1 ? 1 : Gold);
		}

		int32 Count = 0;
		switch (Monster.Rank)
		{
		case EMonsterRank::Normal:   Count = Random.RangeInt(0, 3) == 0 ? 1 : 0; break;
		case EMonsterRank::Champion: Count = 1 + Random.RangeInt(0, 1); break;
		case EMonsterRank::Elite:    Count = Random.RangeInt(2, 3); break;
		case EMonsterRank::Boss:     Count = Random.RangeInt(4, MaxDropItems); break;
		default: break;
		}

		// Solo bases que un personaje del nivel del monstruo podría usar.
		int32 Eligible[64];
		int32 EligibleCount = 0;
		for (int32 i = 0; i < GetItemBaseCount() && EligibleCount < 64; ++i)
		{
			if (GetItemBase(i)->RequiredLevel <= Monster.Level)
			{
				Eligible[EligibleCount++] = i;
			}
		}
		if (EligibleCount == 0)
		{
			return Drop;
		}

		for (int32 i = 0; i < Count; ++i)
		{
			ERarity Rarity = RollRarity(Monster.Rank, MagicFindPercent, Random);
			if (i == 0 && Monster.Rank == EMonsterRank::Boss && Rarity < ERarity::Rare)
			{
				Rarity = ERarity::Rare;
			}
			const int32 Base = Eligible[Random.RangeInt(0, EligibleCount - 1)];
			Drop.Items[Drop.ItemCount++] = GenerateItem(Base, Monster.Level, Rarity, Random);
		}
		return Drop;
	}

	EAiAction DecideAction(EMonsterBehavior Behavior, const AiContext& Context)
	{
		if (Behavior == EMonsterBehavior::Ambusher && !Context.bRevealed)
		{
			// Sale del suelo cuando el jugador está casi encima.
			const bool bClose = Context.bHasTarget && Context.DistanceCm <= Context.AttackRangeCm * 2;
			return bClose ? EAiAction::UseSpecial : EAiAction::StayHidden;
		}
		if (!Context.bHasTarget)
		{
			return EAiAction::Idle;
		}

		const bool bInRange = Context.DistanceCm <= Context.AttackRangeCm;
		const bool bTooClose = Context.DistanceCm < Context.AttackRangeCm / 3;

		switch (Behavior)
		{
		case EMonsterBehavior::RangedKiter:
			if (bTooClose) { return EAiAction::Retreat; }
			return bInRange ? EAiAction::Attack : EAiAction::Approach;

		case EMonsterBehavior::Charger:
			// Embiste solo desde lejos; de cerca pelea normal.
			if (!bInRange && Context.bSpecialReady && Context.DistanceCm <= Context.AttackRangeCm * 6)
			{
				return EAiAction::UseSpecial;
			}
			return bInRange ? EAiAction::Attack : EAiAction::Approach;

		case EMonsterBehavior::Summoner:
			if (bTooClose) { return EAiAction::Retreat; }
			if (Context.bSpecialReady && bInRange) { return EAiAction::UseSpecial; }
			return bInRange ? EAiAction::Attack : EAiAction::Approach;

		case EMonsterBehavior::Support:
			if (Context.bSpecialReady && Context.bAllyNeedsHelp) { return EAiAction::UseSpecial; }
			if (bTooClose) { return EAiAction::Retreat; }
			return bInRange ? EAiAction::Attack : EAiAction::Approach;

		case EMonsterBehavior::Exploder:
			return bInRange ? EAiAction::UseSpecial : EAiAction::Approach;

		case EMonsterBehavior::MeleeChaser:
		case EMonsterBehavior::Ambusher:
		default:
			return bInRange ? EAiAction::Attack : EAiAction::Approach;
		}
	}
}
