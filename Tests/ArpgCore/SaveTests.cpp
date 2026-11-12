#include "ArpgCore/Save/SaveGame.h"
#include "TestFramework.h"

#include <cstring>

using namespace arpg;

namespace
{
	void SetName(CharacterSave& Save, const char* Text)
	{
		std::memset(Save.Name, 0, sizeof(Save.Name));
		std::memcpy(Save.Name, Text, std::strlen(Text) < sizeof(Save.Name) - 1 ? std::strlen(Text) : sizeof(Save.Name) - 1);
	}

	CharacterSave MakeRichSave()
	{
		CharacterSave Save;
		SetName(Save, "Mirel");
		Save.Progress = CreateCharacter(ECharacterClass::Warrior);
		AddExperience(Save.Progress, 250000);
		SpendAttributePoints(Save.Progress, EAttribute::Strength, 20);
		Save.Gold = 123456789012LL;
		Save.Difficulty = EDifficulty::Penitent;
		Save.WorldSeed = 0xDEADBEEFCAFEF00DULL;
		Save.Depth = 7;
		Save.LifePotions = 3;
		Save.ManaPotions = 99;

		Rng R(11);
		ItemInstance Prev; bool bHad = false;
		Save.Gear.Equip(Save.Progress, GenerateItem(FindItemBase("war_axe"), 30, ERarity::Legendary, R),
			EEquipSlot::MainHand, Prev, bHad);
		Save.Gear.Equip(Save.Progress, GenerateItem(FindItemBase("copper_ring"), 12, ERarity::Rare, R),
			EEquipSlot::Ring2, Prev, bHad);
		for (int i = 0; i < 6; ++i)
		{
			Save.Bag.AutoPlace(GenerateItem(i * 2, 10 + i, static_cast<ERarity>(i % 5), R));
		}
		Save.Stash.Place(GenerateItem(FindItemBase("hunting_bow"), 40, ERarity::Epic, R), 8, 6);
		return Save;
	}

	bool SameItem(const ItemInstance& A, const ItemInstance& B)
	{
		if (A.BaseIndex != B.BaseIndex || A.Rarity != B.Rarity || A.ItemLevel != B.ItemLevel || A.AffixCount != B.AffixCount)
		{
			return false;
		}
		for (int32 i = 0; i < A.AffixCount; ++i)
		{
			if (A.Affixes[i].Stat != B.Affixes[i].Stat || A.Affixes[i].Value != B.Affixes[i].Value) { return false; }
		}
		return true;
	}

	bool SameGrid(const GridInventory& A, const GridInventory& B)
	{
		if (A.GetEntries().size() != B.GetEntries().size()) { return false; }
		for (std::size_t i = 0; i < A.GetEntries().size(); ++i)
		{
			const GridInventory::Entry& Ea = A.GetEntries()[i];
			const GridInventory::Entry& Eb = B.GetEntries()[i];
			if (Ea.X != Eb.X || Ea.Y != Eb.Y || !SameItem(Ea.Item, Eb.Item)) { return false; }
		}
		return true;
	}
}

ARPG_TEST(Save_RoundTripPreservesEverything)
{
	const CharacterSave Original = MakeRichSave();
	EXPECT_TRUE(Original.Progress.Level > 10);
	EXPECT_EQ(static_cast<int>(Original.Bag.GetEntries().size()), 6);

	std::vector<uint8> Bytes;
	SerializeCharacter(Original, Bytes);
	CharacterSave Loaded;
	EXPECT_TRUE(DeserializeCharacter(Bytes.data(), Bytes.size(), Loaded) == ESaveResult::Ok);

	EXPECT_TRUE(std::strcmp(Loaded.Name, "Mirel") == 0);
	EXPECT_TRUE(Loaded.Progress.Class == Original.Progress.Class);
	EXPECT_EQ(Loaded.Progress.Level, Original.Progress.Level);
	EXPECT_EQ(Loaded.Progress.Experience, Original.Progress.Experience);
	EXPECT_EQ(Loaded.Progress.Attr.Strength, Original.Progress.Attr.Strength);
	EXPECT_EQ(Loaded.Progress.UnspentAttributePoints, Original.Progress.UnspentAttributePoints);
	EXPECT_EQ(Loaded.Progress.UnspentSkillPoints, Original.Progress.UnspentSkillPoints);
	EXPECT_EQ(Loaded.Gold, Original.Gold);
	EXPECT_TRUE(Loaded.Difficulty == EDifficulty::Penitent);
	EXPECT_EQ(Loaded.WorldSeed, Original.WorldSeed);
	EXPECT_TRUE(Loaded.Depth == 7 && Loaded.LifePotions == 3 && Loaded.ManaPotions == 99);
	for (int Slot = 0; Slot < EquipSlotCount; ++Slot)
	{
		const ItemInstance* A = Original.Gear.Get(static_cast<EEquipSlot>(Slot));
		const ItemInstance* B = Loaded.Gear.Get(static_cast<EEquipSlot>(Slot));
		EXPECT_TRUE((A == nullptr) == (B == nullptr));
		if (A != nullptr && B != nullptr) { EXPECT_TRUE(SameItem(*A, *B)); }
	}
	EXPECT_TRUE(SameGrid(Original.Bag, Loaded.Bag));
	EXPECT_TRUE(SameGrid(Original.Stash, Loaded.Stash));

	// Guardar lo cargado da exactamente los mismos bytes.
	std::vector<uint8> Again;
	SerializeCharacter(Loaded, Again);
	EXPECT_TRUE(Again == Bytes);
}

ARPG_TEST(Save_EmptyCharacterRoundTrips)
{
	CharacterSave Fresh;
	Fresh.Progress = CreateCharacter(ECharacterClass::Necromancer);
	std::vector<uint8> Bytes;
	SerializeCharacter(Fresh, Bytes);
	CharacterSave Loaded;
	EXPECT_TRUE(DeserializeCharacter(Bytes.data(), Bytes.size(), Loaded) == ESaveResult::Ok);
	EXPECT_TRUE(Loaded.Name[0] == '\0' && Loaded.Bag.GetEntries().empty() && Loaded.Gold == 0);
}

ARPG_TEST(Save_RejectsEveryTruncation)
{
	std::vector<uint8> Bytes;
	SerializeCharacter(MakeRichSave(), Bytes);
	CharacterSave Loaded;
	EXPECT_TRUE(DeserializeCharacter(nullptr, 0, Loaded) == ESaveResult::TooShort);
	for (std::size_t Size = 0; Size < Bytes.size(); ++Size)
	{
		EXPECT_TRUE(DeserializeCharacter(Bytes.data(), Size, Loaded) != ESaveResult::Ok);
	}
	Bytes.push_back(0); // basura al final
	EXPECT_TRUE(DeserializeCharacter(Bytes.data(), Bytes.size(), Loaded) != ESaveResult::Ok);
}

ARPG_TEST(Save_RejectsEverySingleByteCorruption)
{
	std::vector<uint8> Bytes;
	SerializeCharacter(MakeRichSave(), Bytes);
	CharacterSave Loaded;
	for (std::size_t i = 0; i < Bytes.size(); ++i)
	{
		const uint8 Original = Bytes[i];
		Bytes[i] = static_cast<uint8>(Original ^ 0x5Au);
		EXPECT_TRUE(DeserializeCharacter(Bytes.data(), Bytes.size(), Loaded) != ESaveResult::Ok);
		Bytes[i] = Original;
	}
	EXPECT_TRUE(DeserializeCharacter(Bytes.data(), Bytes.size(), Loaded) == ESaveResult::Ok);
}

namespace
{
	// Recalcula el CRC para simular un guardado editado a mano "bien".
	void FixChecksum(std::vector<uint8>& Bytes)
	{
		uint32 Crc = 0xFFFFFFFFu;
		for (std::size_t i = 12; i < Bytes.size() - 4; ++i)
		{
			Crc ^= Bytes[i];
			for (int Bit = 0; Bit < 8; ++Bit) { Crc = (Crc >> 1) ^ (0xEDB88320u & (0u - (Crc & 1u))); }
		}
		Crc = ~Crc;
		for (int i = 0; i < 4; ++i) { Bytes[Bytes.size() - 4 + static_cast<std::size_t>(i)] = static_cast<uint8>(Crc >> (8 * i)); }
	}
}

ARPG_TEST(Save_ValidatesContentEvenWithGoodChecksum)
{
	CharacterSave Fresh;
	SetName(Fresh, "Ab");
	Fresh.Progress = CreateCharacter(ECharacterClass::Hunter);
	std::vector<uint8> Clean;
	SerializeCharacter(Fresh, Clean);
	CharacterSave Loaded;

	// Contenido: [12] largo del nombre, [13..14] nombre, [15] clase, [16..19] nivel.
	std::vector<uint8> Bad = Clean;
	Bad[15] = 99; FixChecksum(Bad); // clase inexistente
	EXPECT_TRUE(DeserializeCharacter(Bad.data(), Bad.size(), Loaded) == ESaveResult::Corrupt);

	Bad = Clean;
	Bad[16] = 200; FixChecksum(Bad); // nivel 200
	EXPECT_TRUE(DeserializeCharacter(Bad.data(), Bad.size(), Loaded) == ESaveResult::Corrupt);

	Bad = Clean;
	Bad[4] = 2; // versión futura
	EXPECT_TRUE(DeserializeCharacter(Bad.data(), Bad.size(), Loaded) == ESaveResult::NewerVersion);

	Bad = Clean;
	Bad[0] = 'X';
	EXPECT_TRUE(DeserializeCharacter(Bad.data(), Bad.size(), Loaded) == ESaveResult::BadMagic);

	Bad = Clean;
	Bad[Bad.size() - 1] ^= 1;
	EXPECT_TRUE(DeserializeCharacter(Bad.data(), Bad.size(), Loaded) == ESaveResult::ChecksumFailed);
}

ARPG_TEST(Save_RandomGarbageNeverCrashes)
{
	Rng R(999);
	CharacterSave Loaded;
	std::vector<uint8> Bytes;
	SerializeCharacter(MakeRichSave(), Bytes);
	for (int Round = 0; Round < 3000; ++Round)
	{
		std::vector<uint8> Fuzz = Bytes;
		const int Edits = R.RangeInt(1, 8);
		for (int i = 0; i < Edits; ++i)
		{
			const std::size_t At = static_cast<std::size_t>(R.RangeInt(12, static_cast<int32>(Fuzz.size()) - 5));
			Fuzz[At] = static_cast<uint8>(R.NextU32());
		}
		FixChecksum(Fuzz); // pasa el CRC: obliga a que la validación de contenido aguante
		const ESaveResult Result = DeserializeCharacter(Fuzz.data(), Fuzz.size(), Loaded);
		EXPECT_TRUE(Result == ESaveResult::Ok || Result == ESaveResult::Corrupt);
	}
}
