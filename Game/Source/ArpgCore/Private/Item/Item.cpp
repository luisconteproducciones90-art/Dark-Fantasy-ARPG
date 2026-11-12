#include "ArpgCore/Item/Item.h"

#include <cstring>

namespace arpg
{
	namespace
	{
		using T = EItemType;

		constexpr ItemBase BaseTable[] = {
			// Id               Tipo          An Al Niv Fue Arm  Daño      Bloq
			{"leather_cap",     T::Helmet,    2, 2,  1,  0,  4, {0, 0},    0},
			{"iron_helm",       T::Helmet,    2, 2, 12, 30, 14, {0, 0},    0},
			{"padded_coat",     T::BodyArmor, 2, 3,  1,  0,  8, {0, 0},    0},
			{"chain_hauberk",   T::BodyArmor, 2, 3, 15, 45, 30, {0, 0},    0},
			{"hide_gloves",     T::Gloves,    2, 2,  1,  0,  2, {0, 0},    0},
			{"worn_boots",      T::Boots,     2, 2,  1,  0,  2, {0, 0},    0},
			{"rope_belt",       T::Belt,      2, 1,  1,  0,  1, {0, 0},    0},
			{"short_sword",     T::Weapon,    1, 3,  1,  0,  0, {3, 7},    0},
			{"war_axe",         T::Weapon,    2, 3, 10, 40,  0, {8, 18},   0},
			{"hunting_bow",     T::Weapon,    2, 4,  1,  0,  0, {2, 9},    0},
			{"ash_staff",       T::Weapon,    1, 4,  1,  0,  0, {2, 5},    0},
			{"round_shield",    T::Shield,    2, 2,  1,  0,  5, {0, 0},   15},
			{"copper_ring",     T::Ring,      1, 1,  1,  0,  0, {0, 0},    0},
			{"bone_amulet",     T::Amulet,    1, 1,  1,  0,  0, {0, 0},    0},
		};
		constexpr int32 BaseCount = static_cast<int32>(sizeof(BaseTable) / sizeof(BaseTable[0]));

		constexpr uint32 Bit(EItemType Type) { return 1u << static_cast<unsigned>(Type); }
		constexpr uint32 Wearable = Bit(T::Helmet) | Bit(T::BodyArmor) | Bit(T::Gloves) | Bit(T::Boots) | Bit(T::Belt);
		constexpr uint32 Jewelry = Bit(T::Ring) | Bit(T::Amulet);
		constexpr uint32 All = Wearable | Jewelry | Bit(T::Weapon) | Bit(T::Shield);
		constexpr uint32 NotWeapon = All & ~Bit(T::Weapon);

		struct AffixDef
		{
			int32 BaseMin;
			int32 BaseMax;
			int32 GrowthPercentPerLevel;
			uint32 TypeMask;
		};

		// Orden idéntico a EAffixStat. Valor = tirada base × (1 + crecimiento × nivel de objeto).
		constexpr AffixDef AffixTable[] = {
			/* Strength   */ {1, 3, 8, All},
			/* Dexterity  */ {1, 3, 8, All},
			/* Vitality   */ {1, 3, 8, All},
			/* Energy     */ {1, 3, 8, All},
			/* Life       */ {5, 10, 10, Wearable | Bit(T::Shield) | Bit(T::Amulet)},
			/* Mana       */ {5, 10, 10, Bit(T::Helmet) | Jewelry | Bit(T::Weapon)},
			/* AttackRat. */ {10, 20, 10, Bit(T::Weapon) | Bit(T::Gloves) | Jewelry},
			/* Defense    */ {5, 10, 10, Wearable | Bit(T::Shield)},
			/* Armor      */ {3, 6, 10, Wearable | Bit(T::Shield)},
			/* ResFire    */ {3, 6, 4, NotWeapon},
			/* ResCold    */ {3, 6, 4, NotWeapon},
			/* ResLightn. */ {3, 6, 4, NotWeapon},
			/* ResPoison  */ {3, 6, 4, NotWeapon},
			/* ResShadow  */ {3, 6, 4, NotWeapon},
			/* Crit ‰     */ {5, 10, 2, Bit(T::Weapon) | Bit(T::Gloves) | Jewelry},
			/* AddedPhys  */ {1, 3, 10, Bit(T::Weapon) | Bit(T::Ring)},
			/* LifeOnHit  */ {1, 2, 4, Bit(T::Weapon) | Bit(T::Ring)},
			/* MagicFind  */ {3, 6, 2, Bit(T::Helmet) | Bit(T::Gloves) | Bit(T::Boots) | Jewelry},
		};
		static_assert(sizeof(AffixTable) / sizeof(AffixTable[0]) == static_cast<unsigned>(EAffixStat::Count));

		struct AffixCountRange { int32 Min; int32 Max; };
		// Orden idéntico a ERarity (Mythic usa el rango de Legendary).
		constexpr AffixCountRange AffixCounts[] = {{0, 0}, {1, 2}, {3, 4}, {4, 5}, {5, 6}, {5, 6}};
		constexpr int32 RarityValuePercent[] = {100, 100, 100, 125, 125, 125};
		constexpr int32 RaritySellMultiplier[] = {1, 2, 4, 8, 16, 40};
	}

	int32 GetItemBaseCount()
	{
		return BaseCount;
	}

	const ItemBase* GetItemBase(int32 BaseIndex)
	{
		return BaseIndex >= 0 && BaseIndex < BaseCount ? &BaseTable[BaseIndex] : nullptr;
	}

	int32 FindItemBase(const char* Id)
	{
		if (Id == nullptr)
		{
			return -1;
		}
		for (int32 i = 0; i < BaseCount; ++i)
		{
			if (std::strcmp(BaseTable[i].Id, Id) == 0)
			{
				return i;
			}
		}
		return -1;
	}

	bool IsAffixAllowed(EAffixStat Stat, EItemType Type)
	{
		if (Stat >= EAffixStat::Count || Type >= EItemType::Count)
		{
			return false;
		}
		return (AffixTable[static_cast<int>(Stat)].TypeMask & Bit(Type)) != 0u;
	}

	ERarity RollRarity(EMonsterRank Rank, int32 MagicFindPercent, Rng& Random)
	{
		// Pesos sobre 10 000 para un monstruo normal sin hallazgo mágico.
		constexpr int32 BaseLegendary = 20, BaseEpic = 150, BaseRare = 700, BaseMagic = 3000;
		constexpr int32 RankPercent[] = {100, 200, 300, 600};

		// Rendimientos decrecientes: 100 % MF rinde ~71 %, 500 % rinde ~167 %, techo 250 %.
		const int64 Mf = MagicFindPercent < 0 ? 0 : MagicFindPercent;
		const int64 EffectiveMf = Mf * 250 / (Mf + 250);
		const int64 Scale = static_cast<int64>(RankPercent[static_cast<int>(Rank) & 3]) * (100 + EffectiveMf); // ×10 000

		const int32 Roll = Random.RangeInt(0, 9999);
		int64 Threshold = BaseLegendary * Scale / 10000;
		if (Roll < Threshold) { return ERarity::Legendary; }
		Threshold += BaseEpic * Scale / 10000;
		if (Roll < Threshold) { return ERarity::Epic; }
		Threshold += BaseRare * Scale / 10000;
		if (Roll < Threshold) { return ERarity::Rare; }
		Threshold += BaseMagic * Scale / 10000;
		if (Roll < Threshold) { return ERarity::Magic; }
		return ERarity::Common;
	}

	ItemInstance GenerateItem(int32 BaseIndex, int32 ItemLevel, ERarity Rarity, Rng& Random)
	{
		ItemInstance Item;
		const ItemBase* Base = GetItemBase(BaseIndex);
		if (Base == nullptr)
		{
			return Item;
		}
		if (Rarity >= ERarity::Mythic)
		{
			Rarity = ERarity::Legendary;
		}
		Item.BaseIndex = BaseIndex;
		Item.Rarity = Rarity;
		Item.ItemLevel = Clamp(ItemLevel, 1, MaxItemLevel);

		EAffixStat Eligible[AffixStatCount];
		int32 EligibleCount = 0;
		for (int i = 0; i < AffixStatCount; ++i)
		{
			if ((AffixTable[i].TypeMask & Bit(Base->Type)) != 0u)
			{
				Eligible[EligibleCount++] = static_cast<EAffixStat>(i);
			}
		}

		const AffixCountRange& Range = AffixCounts[static_cast<int>(Rarity)];
		int32 Wanted = Random.RangeInt(Range.Min, Range.Max);
		if (Wanted > EligibleCount) { Wanted = EligibleCount; }
		if (Wanted > MaxAffixes) { Wanted = MaxAffixes; }

		for (int32 i = 0; i < Wanted; ++i)
		{
			// Fisher–Yates parcial: elige sin repetición.
			const int32 Pick = Random.RangeInt(i, EligibleCount - 1);
			const EAffixStat Chosen = Eligible[Pick];
			Eligible[Pick] = Eligible[i];
			Eligible[i] = Chosen;

			const AffixDef& Def = AffixTable[static_cast<int>(Chosen)];
			const int64 BaseRoll = Random.RangeInt(Def.BaseMin, Def.BaseMax);
			int64 Value = BaseRoll * (100 + static_cast<int64>(Def.GrowthPercentPerLevel) * Item.ItemLevel) / 100;
			Value = Value * RarityValuePercent[static_cast<int>(Rarity)] / 100;
			Item.Affixes[i].Stat = Chosen;
			Item.Affixes[i].Value = static_cast<int32>(Value < 1 ? 1 : Value);
		}
		Item.AffixCount = Wanted;
		return Item;
	}

	int32 ComputeSellValue(const ItemInstance& Item)
	{
		const ItemBase* Base = GetItemBase(Item.BaseIndex);
		if (Base == nullptr || Item.Rarity >= ERarity::Count)
		{
			return 0;
		}
		const int64 Value = static_cast<int64>(10 + 5 * Clamp(Item.ItemLevel, 1, MaxItemLevel))
			* RaritySellMultiplier[static_cast<int>(Item.Rarity)]
			* (2 + Clamp(Item.AffixCount, 0, MaxAffixes)) / 2;
		return static_cast<int32>(Value);
	}
}
