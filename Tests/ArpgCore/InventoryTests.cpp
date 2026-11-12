#include "ArpgCore/Inventory/Equipment.h"
#include "ArpgCore/Inventory/GridInventory.h"
#include "TestFramework.h"

using namespace arpg;

namespace
{
	ItemInstance Make(const char* Id, ERarity Rarity = ERarity::Common, int32 Level = 1)
	{
		Rng R(1);
		return GenerateItem(FindItemBase(Id), Level, Rarity, R);
	}

	ItemInstance WithAffix(const char* Id, EAffixStat Stat, int32 Value)
	{
		ItemInstance Item = Make(Id);
		Item.AffixCount = 1;
		Item.Affixes[0] = {Stat, Value};
		return Item;
	}
}

ARPG_TEST(Grid_PlaceRespectsBoundsAndOverlap)
{
	GridInventory Bag(10, 4);
	const ItemInstance Bow = Make("hunting_bow"); // 2×4
	EXPECT_TRUE(Bag.Place(Bow, 9, 0) == 0);   // se sale por la derecha
	EXPECT_TRUE(Bag.Place(Bow, 0, 1) == 0);   // se sale por abajo
	EXPECT_TRUE(Bag.Place(Bow, -1, 0) == 0);
	const int32 Id = Bag.Place(Bow, 0, 0);
	EXPECT_TRUE(Id > 0);
	EXPECT_TRUE(Bag.Place(Make("copper_ring"), 1, 3) == 0); // solapa
	EXPECT_TRUE(Bag.Place(Make("copper_ring"), 2, 3) > 0);
	EXPECT_TRUE(Bag.At(1, 3) != nullptr && Bag.At(1, 3)->Id == Id);
	EXPECT_TRUE(Bag.At(3, 3) == nullptr && Bag.At(99, 0) == nullptr && Bag.At(0, -1) == nullptr);
	EXPECT_EQ(Bag.CountFreeCells(), 40 - 8 - 1);
	EXPECT_TRUE(Bag.Place(ItemInstance{}, 5, 0) == 0); // objeto inválido
	EXPECT_TRUE(!Bag.CanPlace(INT32_MAX, 0, 2, 2) && !Bag.CanPlace(0, 0, INT32_MAX, 1));
}

ARPG_TEST(Grid_AutoPlaceFillsExactlyAndThenFails)
{
	GridInventory Bag(10, 4);
	for (int i = 0; i < 5; ++i) { EXPECT_TRUE(Bag.AutoPlace(Make("hunting_bow")) > 0); } // 5 × (2×4) = 40
	EXPECT_EQ(Bag.CountFreeCells(), 0);
	EXPECT_EQ(Bag.AutoPlace(Make("copper_ring")), 0);
	EXPECT_EQ(static_cast<int>(Bag.GetEntries().size()), 5);
}

ARPG_TEST(Grid_RemoveFreesCellsAndReturnsItem)
{
	GridInventory Bag(4, 4);
	const int32 A = Bag.AutoPlace(Make("padded_coat", ERarity::Rare, 20));
	const int32 B = Bag.AutoPlace(Make("copper_ring"));
	ItemInstance Out;
	EXPECT_TRUE(Bag.Remove(A, &Out));
	EXPECT_TRUE(Out.Rarity == ERarity::Rare && Out.ItemLevel == 20);
	EXPECT_TRUE(!Bag.Remove(A));
	EXPECT_TRUE(!Bag.Remove(0) && !Bag.Remove(-3) && !Bag.Remove(999));
	EXPECT_TRUE(Bag.Find(A) == nullptr && Bag.Find(B) != nullptr);
	EXPECT_EQ(Bag.CountFreeCells(), 15);
	EXPECT_TRUE(Bag.AutoPlace(Make("copper_ring")) != B); // los Id no se reutilizan
}

ARPG_TEST(Grid_MoveOntoItselfAndBlockedMove)
{
	GridInventory Bag(6, 4);
	const int32 Coat = Bag.Place(Make("padded_coat"), 0, 0); // 2×3
	const int32 Sword = Bag.Place(Make("short_sword"), 3, 0); // 1×3
	EXPECT_TRUE(Bag.Move(Coat, 1, 1)); // se solapa consigo mismo: válido
	EXPECT_TRUE(Bag.At(0, 0) == nullptr && Bag.At(2, 3)->Id == Coat);
	EXPECT_TRUE(!Bag.Move(Coat, 2, 0)); // chocaría con la espada
	EXPECT_TRUE(Bag.Find(Coat)->X == 1 && Bag.Find(Coat)->Y == 1); // sin cambios
	EXPECT_TRUE(Bag.At(3, 0)->Id == Sword);
	EXPECT_TRUE(!Bag.Move(Coat, 5, 0) && !Bag.Move(999, 0, 0));
	EXPECT_EQ(Bag.CountFreeCells(), 24 - 6 - 3);
}

ARPG_TEST(Equip_SlotAndRequirementChecks)
{
	CharacterProgress Sorc = CreateCharacter(ECharacterClass::Sorcerer); // Fuerza 10, nivel 1
	Equipment Gear;
	ItemInstance Prev; bool bHad = false;

	EXPECT_TRUE(Gear.Equip(Sorc, Make("short_sword"), EEquipSlot::Helmet, Prev, bHad) == EEquipResult::WrongSlot);
	EXPECT_TRUE(Gear.Equip(Sorc, ItemInstance{}, EEquipSlot::MainHand, Prev, bHad) == EEquipResult::InvalidItem);
	EXPECT_TRUE(Gear.Equip(Sorc, Make("war_axe"), EEquipSlot::MainHand, Prev, bHad) == EEquipResult::LevelTooLow);
	Sorc.Level = 20;
	EXPECT_TRUE(Gear.Equip(Sorc, Make("war_axe"), EEquipSlot::MainHand, Prev, bHad) == EEquipResult::StrengthTooLow);
	EXPECT_TRUE(Gear.Get(EEquipSlot::MainHand) == nullptr);

	EXPECT_TRUE(Gear.Equip(Sorc, Make("ash_staff"), EEquipSlot::MainHand, Prev, bHad) == EEquipResult::Ok);
	EXPECT_TRUE(!bHad);
	EXPECT_TRUE(Gear.Equip(Sorc, Make("short_sword"), EEquipSlot::MainHand, Prev, bHad) == EEquipResult::Ok);
	EXPECT_TRUE(bHad && Prev.BaseIndex == FindItemBase("ash_staff")); // intercambio

	// Dos anillos, cada uno en su ranura.
	EXPECT_TRUE(Gear.Equip(Sorc, Make("copper_ring"), EEquipSlot::Ring1, Prev, bHad) == EEquipResult::Ok);
	EXPECT_TRUE(Gear.Equip(Sorc, Make("copper_ring"), EEquipSlot::Ring2, Prev, bHad) == EEquipResult::Ok);
	EXPECT_TRUE(Gear.Equip(Sorc, Make("copper_ring"), EEquipSlot::Amulet, Prev, bHad) == EEquipResult::WrongSlot);

	ItemInstance Out;
	EXPECT_TRUE(Gear.Unequip(EEquipSlot::MainHand, &Out) && Out.BaseIndex == FindItemBase("short_sword"));
	EXPECT_TRUE(!Gear.Unequip(EEquipSlot::MainHand) && !Gear.Unequip(EEquipSlot::Count));
}

ARPG_TEST(Equip_StatsAggregateFromGear)
{
	const CharacterProgress W = CreateCharacter(ECharacterClass::Warrior); // Fue 30, Vit 25
	Equipment Gear;
	ItemInstance Prev; bool bHad = false;
	const EquippedStats Naked = ComputeEquippedStats(W, Gear);
	EXPECT_EQ(Naked.Armor, 0);
	EXPECT_EQ(Naked.WeaponDamage.Min, 1); // 1 × 1,30
	EXPECT_EQ(Naked.WeaponDamage.Max, 2); // 2 × 1,30 = 2,6
	EXPECT_EQ(Naked.CritChancePermille, 50);

	Gear.Equip(W, WithAffix("short_sword", EAffixStat::AddedPhysicalDamage, 3), EEquipSlot::MainHand, Prev, bHad);
	Gear.Equip(W, WithAffix("padded_coat", EAffixStat::Vitality, 10), EEquipSlot::BodyArmor, Prev, bHad);
	Gear.Equip(W, WithAffix("round_shield", EAffixStat::ResistFire, 20), EEquipSlot::OffHand, Prev, bHad);
	Gear.Equip(W, WithAffix("copper_ring", EAffixStat::Strength, 20), EEquipSlot::Ring1, Prev, bHad);
	Gear.Equip(W, WithAffix("copper_ring", EAffixStat::Life, 15), EEquipSlot::Ring2, Prev, bHad);

	const EquippedStats S = ComputeEquippedStats(W, Gear);
	EXPECT_EQ(S.Attr.Strength, 50);
	EXPECT_EQ(S.Derived.MaxLife, Naked.Derived.MaxLife + 10 * 4 + 15); // Vitalidad del equipo también da vida
	EXPECT_EQ(S.Armor, 8 + 5);
	EXPECT_EQ(S.BlockPercent, 15);
	EXPECT_EQ(S.WeaponDamage.Min, 9);  // (3+3) × 1,50
	EXPECT_EQ(S.WeaponDamage.Max, 15); // (7+3) × 1,50
	EXPECT_EQ(S.ResistPercent[static_cast<int>(EDamageType::Fire)], 20);

	Gear.Unequip(EEquipSlot::Ring1);
	EXPECT_EQ(ComputeEquippedStats(W, Gear).Attr.Strength, 30);
}

ARPG_TEST(Equip_ProfilesFeedCombat)
{
	const CharacterProgress W = CreateCharacter(ECharacterClass::Warrior);
	Equipment Gear;
	ItemInstance Prev; bool bHad = false;
	Gear.Equip(W, WithAffix("round_shield", EAffixStat::ResistFire, 50), EEquipSlot::OffHand, Prev, bHad);
	Gear.Equip(W, Make("short_sword"), EEquipSlot::MainHand, Prev, bHad);
	const EquippedStats S = ComputeEquippedStats(W, Gear);

	const AttackProfile Attack = MakeAttackProfile(W, S);
	EXPECT_EQ(Attack.Damage[0].Min, S.WeaponDamage.Min);
	EXPECT_EQ(Attack.AttackRating, S.Derived.AttackRating);
	EXPECT_TRUE(Attack.CritChance > 0.049f && Attack.CritChance < 0.051f);

	StatusContainer Cursed;
	Cursed.Apply(EStatusEffect::Curse, 5000, 0, 30);
	const DefenseProfile Clean = MakeDefenseProfile(W, S);
	const DefenseProfile Weak = MakeDefenseProfile(W, S, &Cursed);
	const int Fire = static_cast<int>(EDamageType::Fire);
	EXPECT_EQ(Clean.ResistPercent[Fire], 50);
	EXPECT_EQ(Weak.ResistPercent[Fire], 20);
	EXPECT_TRUE(Clean.BlockChance > 0.149f && Clean.BlockChance < 0.151f);

	// La maldición se nota en el daño real recibido.
	AttackProfile Fireball;
	Fireball.Damage[Fire] = {100, 100};
	Fireball.bIsSpell = true; Fireball.CritChance = 0.0f;
	Rng R(9);
	EXPECT_EQ(ResolveHit(Fireball, Clean, R).Total, 50);
	EXPECT_EQ(ResolveHit(Fireball, Weak, R).Total, 80);
}
