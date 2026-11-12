#pragma once

#include "ArpgCore/Combat/Damage.h"
#include "ArpgCore/Core/Rng.h"
#include "ArpgCore/Core/Types.h"

namespace arpg
{
	enum class EItemType : uint8
	{
		Helmet, BodyArmor, Gloves, Boots, Belt, Weapon, Shield, Ring, Amulet, Count
	};

	enum class ERarity : uint8
	{
		Common, Magic, Rare, Epic, Legendary,
		Mythic, // diseñados a mano; no salen del generador aleatorio
		Count
	};

	enum class EAffixStat : uint8
	{
		Strength, Dexterity, Vitality, Energy,
		Life, Mana, AttackRating, Defense, Armor,
		ResistFire, ResistCold, ResistLightning, ResistPoison, ResistShadow,
		CritChancePermille,   // 10 = 1 %
		AddedPhysicalDamage,
		LifeOnHit,
		MagicFind,            // % de mejora en la rareza del botín
		Count
	};
	inline constexpr int AffixStatCount = static_cast<int>(EAffixStat::Count);

	/** Tipo base de objeto ("espada corta"). En la Fase 7 pasa a archivos de datos. */
	struct ItemBase
	{
		const char* Id;
		EItemType Type;
		int32 Width;   // casillas de inventario
		int32 Height;
		int32 RequiredLevel;
		int32 RequiredStrength;
		int32 Armor;
		DamageRange Damage;   // solo armas
		int32 BlockPercent;   // solo escudos
	};

	int32 GetItemBaseCount();
	/** nullptr si el índice no existe. */
	const ItemBase* GetItemBase(int32 BaseIndex);
	/** -1 si no existe. */
	int32 FindItemBase(const char* Id);

	struct Affix
	{
		EAffixStat Stat = EAffixStat::Strength;
		int32 Value = 0;
	};

	inline constexpr int MaxAffixes = 6;
	inline constexpr int32 MaxItemLevel = 85;

	/** Un objeto concreto. Es lo que se guarda, se replica y se comercia. */
	struct ItemInstance
	{
		int32 BaseIndex = -1;
		ERarity Rarity = ERarity::Common;
		int32 ItemLevel = 1;
		int32 AffixCount = 0;
		Affix Affixes[MaxAffixes];
	};

	enum class EMonsterRank : uint8 { Normal, Champion, Elite, Boss };

	/** Decide la rareza de una caída. Nunca devuelve Mythic. */
	ERarity RollRarity(EMonsterRank Rank, int32 MagicFindPercent, Rng& Random);

	/**
	 * Crea un objeto con propiedades aleatorias: sin repetir propiedad y solo
	 * las permitidas para su tipo. BaseIndex inválido => objeto con BaseIndex -1.
	 */
	ItemInstance GenerateItem(int32 BaseIndex, int32 ItemLevel, ERarity Rarity, Rng& Random);

	/** Oro que paga un mercader. La compra cuesta BuyPriceMultiplier veces esto. */
	int32 ComputeSellValue(const ItemInstance& Item);
	inline constexpr int32 BuyPriceMultiplier = 4;

	/** true si esa propiedad puede salir en ese tipo de objeto. */
	bool IsAffixAllowed(EAffixStat Stat, EItemType Type);
}
