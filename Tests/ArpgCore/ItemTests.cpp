#include "ArpgCore/Item/Item.h"
#include "TestFramework.h"

using namespace arpg;

ARPG_TEST(Item_BaseTableIsSane)
{
	EXPECT_TRUE(GetItemBaseCount() >= 9);
	bool TypeSeen[static_cast<int>(EItemType::Count)] = {};
	for (int32 i = 0; i < GetItemBaseCount(); ++i)
	{
		const ItemBase* B = GetItemBase(i);
		EXPECT_TRUE(B != nullptr && B->Width >= 1 && B->Width <= 2 && B->Height >= 1 && B->Height <= 4);
		EXPECT_EQ(FindItemBase(B->Id), i);
		EXPECT_TRUE((B->Type == EItemType::Weapon) == (B->Damage.Max > 0));
		EXPECT_TRUE(B->Damage.Min <= B->Damage.Max);
		TypeSeen[static_cast<int>(B->Type)] = true;
	}
	for (bool bSeen : TypeSeen) { EXPECT_TRUE(bSeen); } // hay al menos una base por tipo
	EXPECT_TRUE(GetItemBase(-1) == nullptr && GetItemBase(GetItemBaseCount()) == nullptr);
	EXPECT_EQ(FindItemBase("no_existe"), -1);
	EXPECT_EQ(FindItemBase(nullptr), -1);
}

ARPG_TEST(Item_GeneratedAffixesRespectRarityTypeAndUniqueness)
{
	Rng R(31337);
	const int32 MinCount[] = {0, 1, 3, 4, 5};
	const int32 MaxCount[] = {0, 2, 4, 5, 6};
	for (int32 Base = 0; Base < GetItemBaseCount(); ++Base)
	{
		const EItemType Type = GetItemBase(Base)->Type;
		for (int Rarity = 0; Rarity <= static_cast<int>(ERarity::Legendary); ++Rarity)
		{
			for (int Rep = 0; Rep < 200; ++Rep)
			{
				const ItemInstance Item = GenerateItem(Base, 1 + Rep % 85, static_cast<ERarity>(Rarity), R);
				EXPECT_EQ(Item.BaseIndex, Base);
				EXPECT_TRUE(Item.AffixCount >= MinCount[Rarity] && Item.AffixCount <= MaxCount[Rarity]);
				bool Seen[AffixStatCount] = {};
				for (int32 i = 0; i < Item.AffixCount; ++i)
				{
					const Affix& A = Item.Affixes[i];
					EXPECT_TRUE(A.Value >= 1);
					EXPECT_TRUE(IsAffixAllowed(A.Stat, Type));
					EXPECT_TRUE(!Seen[static_cast<int>(A.Stat)]);
					Seen[static_cast<int>(A.Stat)] = true;
				}
			}
		}
	}
}

ARPG_TEST(Item_HigherItemLevelRollsHigherValues)
{
	Rng R(8);
	const int32 Sword = FindItemBase("short_sword");
	int64 Low = 0, High = 0;
	for (int i = 0; i < 2000; ++i)
	{
		const ItemInstance A = GenerateItem(Sword, 5, ERarity::Rare, R);
		const ItemInstance B = GenerateItem(Sword, 60, ERarity::Rare, R);
		for (int32 k = 0; k < A.AffixCount; ++k) { Low += A.Affixes[k].Value; }
		for (int32 k = 0; k < B.AffixCount; ++k) { High += B.Affixes[k].Value; }
	}
	EXPECT_TRUE(High > Low * 2);
}

ARPG_TEST(Item_GenerationIsDeterministicAndHandlesBadInput)
{
	Rng R1(77), R2(77);
	for (int i = 0; i < 300; ++i)
	{
		const ItemInstance A = GenerateItem(i % GetItemBaseCount(), 30, ERarity::Legendary, R1);
		const ItemInstance B = GenerateItem(i % GetItemBaseCount(), 30, ERarity::Legendary, R2);
		EXPECT_EQ(A.AffixCount, B.AffixCount);
		for (int32 k = 0; k < A.AffixCount; ++k)
		{
			EXPECT_TRUE(A.Affixes[k].Stat == B.Affixes[k].Stat && A.Affixes[k].Value == B.Affixes[k].Value);
		}
	}
	Rng R(1);
	EXPECT_EQ(GenerateItem(-1, 10, ERarity::Rare, R).BaseIndex, -1);
	EXPECT_EQ(GenerateItem(9999, 10, ERarity::Rare, R).BaseIndex, -1);
	EXPECT_EQ(GenerateItem(0, -5, ERarity::Rare, R).ItemLevel, 1);
	EXPECT_EQ(GenerateItem(0, 5000, ERarity::Rare, R).ItemLevel, MaxItemLevel);
	EXPECT_TRUE(GenerateItem(0, 10, ERarity::Mythic, R).Rarity == ERarity::Legendary);
}

namespace
{
	struct RarityCounts { int Count[static_cast<int>(ERarity::Count)] = {}; };

	RarityCounts Sample(EMonsterRank Rank, int32 MagicFind, uint64 Seed)
	{
		Rng R(Seed);
		RarityCounts Out;
		for (int i = 0; i < 200000; ++i) { ++Out.Count[static_cast<int>(RollRarity(Rank, MagicFind, R))]; }
		return Out;
	}
}

ARPG_TEST(Item_RarityDistribution)
{
	const RarityCounts Base = Sample(EMonsterRank::Normal, 0, 1);
	// Esperado sobre 200 000: legendario 400, épico 3 000, raro 14 000, mágico 60 000.
	EXPECT_TRUE(Base.Count[4] > 300 && Base.Count[4] < 500);
	EXPECT_TRUE(Base.Count[3] > 2700 && Base.Count[3] < 3300);
	EXPECT_TRUE(Base.Count[2] > 13300 && Base.Count[2] < 14700);
	EXPECT_TRUE(Base.Count[1] > 58500 && Base.Count[1] < 61500);
	EXPECT_EQ(Base.Count[static_cast<int>(ERarity::Mythic)], 0);

	const RarityCounts WithMf = Sample(EMonsterRank::Normal, 200, 2);
	const RarityCounts Boss = Sample(EMonsterRank::Boss, 0, 3);
	EXPECT_TRUE(WithMf.Count[4] > Base.Count[4] * 3 / 2);
	EXPECT_TRUE(Boss.Count[4] > Base.Count[4] * 4);
	EXPECT_TRUE(Boss.Count[0] < Base.Count[0]);

	// Hallazgo mágico absurdo o negativo no rompe nada ni garantiza legendarios.
	const RarityCounts Huge = Sample(EMonsterRank::Normal, INT32_MAX, 4);
	EXPECT_TRUE(Huge.Count[4] < 200000 / 50);
	const RarityCounts Negative = Sample(EMonsterRank::Normal, -500, 1);
	EXPECT_EQ(Negative.Count[4], Base.Count[4]);
}

ARPG_TEST(Item_SellValueGrowsWithRarityAndLevel)
{
	Rng R(4);
	const int32 Ring = FindItemBase("copper_ring");
	const ItemInstance Common = GenerateItem(Ring, 10, ERarity::Common, R);
	const ItemInstance Rare = GenerateItem(Ring, 10, ERarity::Rare, R);
	const ItemInstance RareHigh = GenerateItem(Ring, 50, ERarity::Rare, R);
	EXPECT_EQ(ComputeSellValue(Common), 60);
	EXPECT_TRUE(ComputeSellValue(Rare) > ComputeSellValue(Common) * 4);
	EXPECT_TRUE(ComputeSellValue(RareHigh) > ComputeSellValue(Rare));
	EXPECT_EQ(ComputeSellValue(ItemInstance{}), 0);
}
