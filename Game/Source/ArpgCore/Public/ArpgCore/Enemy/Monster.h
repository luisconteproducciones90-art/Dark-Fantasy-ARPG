#pragma once

#include "ArpgCore/Combat/Combatant.h"
#include "ArpgCore/Item/Item.h"

namespace arpg
{
	enum class EDifficulty : uint8 { Pilgrim, Penitent, Condemned, Count };

	enum class EMonsterBehavior : uint8
	{
		MeleeChaser, // persigue y golpea
		RangedKiter, // dispara y mantiene distancia
		Charger,     // embiste desde lejos
		Summoner,    // invoca y evita el cuerpo a cuerpo
		Support,     // cura o potencia aliados
		Ambusher,    // espera oculto hasta tener cerca al jugador
		Exploder,    // corre hacia el jugador y estalla
		Count
	};

	/** Datos de equilibrio de un tipo de monstruo. */
	struct MonsterDef
	{
		const char* Id;
		EMonsterBehavior Behavior;
		int32 BaseLife;
		int32 LifePerLevel;
		DamageRange BaseDamage;        // a nivel 1
		int32 DamageGrowthPercent;     // por nivel
		EDamageType DamageType;
		int32 ArmorPerLevel;
		int32 ResistPercent[DamageTypeCount];
		int32 BaseExperience;
		int32 AttackRangeCm;
	};

	int32 GetMonsterDefCount();
	const MonsterDef* GetMonsterDef(int32 DefIndex);
	int32 FindMonsterDef(const char* Id);

	/** Monstruo concreto ya escalado por nivel, rango y dificultad. */
	struct MonsterInstance
	{
		int32 DefIndex = -1;
		int32 Level = 1;
		EMonsterRank Rank = EMonsterRank::Normal;
		Combatant Body;
		AttackProfile Attack;
		DefenseProfile Defense;
		int32 Experience = 0;
		/** % de duración de estados de control que sufre (100 normal, jefes menos). */
		int32 StatusDurationPercent = 100;
	};

	/** DefIndex inválido => instancia con DefIndex -1. */
	MonsterInstance SpawnMonster(int32 DefIndex, int32 Level, EMonsterRank Rank, EDifficulty Difficulty);

	/** Experiencia que recibe un jugador: completa hasta 5 niveles por encima del monstruo, luego decae. */
	int64 ComputeKillExperience(int32 MonsterExperience, int32 PlayerLevel, int32 MonsterLevel);

	inline constexpr int MaxDropItems = 6;

	struct LootDrop
	{
		int32 Gold = 0;
		int32 ItemCount = 0;
		ItemInstance Items[MaxDropItems];
	};

	/** Botín de un monstruo muerto. Los jefes garantizan al menos un objeto raro o mejor. */
	LootDrop RollLoot(const MonsterInstance& Monster, int32 MagicFindPercent, Rng& Random);

	// ---- Decisión de IA (pura: el motor aporta percepción y ejecuta la acción) ----

	struct AiContext
	{
		int32 DistanceCm = 0;       // al objetivo
		int32 AttackRangeCm = 150;
		int32 LifePercent = 100;
		bool bHasTarget = false;
		bool bSpecialReady = false; // embestida, invocación, cura, explosión
		bool bRevealed = true;      // los emboscadores empiezan ocultos
		bool bAllyNeedsHelp = false;
	};

	enum class EAiAction : uint8 { Idle, Approach, Attack, Retreat, UseSpecial, StayHidden };

	EAiAction DecideAction(EMonsterBehavior Behavior, const AiContext& Context);
}
