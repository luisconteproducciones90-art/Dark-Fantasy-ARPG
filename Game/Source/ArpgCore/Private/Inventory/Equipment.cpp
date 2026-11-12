#include "ArpgCore/Inventory/Equipment.h"

namespace arpg
{
	namespace
	{
		constexpr EItemType SlotTypes[] = {
			EItemType::Helmet, EItemType::BodyArmor, EItemType::Gloves, EItemType::Boots, EItemType::Belt,
			EItemType::Weapon, EItemType::Shield, EItemType::Ring, EItemType::Ring, EItemType::Amulet,
		};
		static_assert(sizeof(SlotTypes) / sizeof(SlotTypes[0]) == static_cast<unsigned>(EEquipSlot::Count));

		constexpr DamageRange UnarmedDamage = {1, 2};
	}

	EItemType GetSlotItemType(EEquipSlot Slot)
	{
		return Slot < EEquipSlot::Count ? SlotTypes[static_cast<int>(Slot)] : EItemType::Count;
	}

	const ItemInstance* Equipment::Get(EEquipSlot Slot) const
	{
		if (Slot >= EEquipSlot::Count || !Occupied[static_cast<int>(Slot)])
		{
			return nullptr;
		}
		return &Items[static_cast<int>(Slot)];
	}

	EEquipResult Equipment::CanEquip(const CharacterProgress& Progress, const ItemInstance& Item, EEquipSlot Slot) const
	{
		const ItemBase* Base = GetItemBase(Item.BaseIndex);
		if (Base == nullptr)
		{
			return EEquipResult::InvalidItem;
		}
		if (Slot >= EEquipSlot::Count || Base->Type != GetSlotItemType(Slot))
		{
			return EEquipResult::WrongSlot;
		}
		if (Progress.Level < Base->RequiredLevel)
		{
			return EEquipResult::LevelTooLow;
		}
		if (Progress.Attr.Strength < Base->RequiredStrength)
		{
			return EEquipResult::StrengthTooLow;
		}
		return EEquipResult::Ok;
	}

	EEquipResult Equipment::Equip(const CharacterProgress& Progress, const ItemInstance& Item, EEquipSlot Slot,
		ItemInstance& OutPrevious, bool& bOutHadPrevious)
	{
		bOutHadPrevious = false;
		const EEquipResult Result = CanEquip(Progress, Item, Slot);
		if (Result != EEquipResult::Ok)
		{
			return Result;
		}
		const int Index = static_cast<int>(Slot);
		const ItemInstance Incoming = Item; // Item puede ser alias de OutPrevious
		if (Occupied[Index])
		{
			OutPrevious = Items[Index];
			bOutHadPrevious = true;
		}
		Items[Index] = Incoming;
		Occupied[Index] = true;
		return EEquipResult::Ok;
	}

	bool Equipment::Unequip(EEquipSlot Slot, ItemInstance* OutItem)
	{
		if (Slot >= EEquipSlot::Count || !Occupied[static_cast<int>(Slot)])
		{
			return false;
		}
		const int Index = static_cast<int>(Slot);
		if (OutItem != nullptr)
		{
			*OutItem = Items[Index];
		}
		Items[Index] = ItemInstance{};
		Occupied[Index] = false;
		return true;
	}

	EquippedStats ComputeEquippedStats(const CharacterProgress& Progress, const Equipment& Gear)
	{
		int64 Bonus[AffixStatCount] = {};
		int64 Armor = 0;
		int32 BlockPercent = 0;
		DamageRange Weapon = UnarmedDamage;

		for (int Slot = 0; Slot < EquipSlotCount; ++Slot)
		{
			const ItemInstance* Item = Gear.Get(static_cast<EEquipSlot>(Slot));
			const ItemBase* Base = Item != nullptr ? GetItemBase(Item->BaseIndex) : nullptr;
			if (Base == nullptr)
			{
				continue;
			}
			Armor += Base->Armor;
			BlockPercent += Base->BlockPercent;
			if (Base->Type == EItemType::Weapon)
			{
				Weapon = Base->Damage;
			}
			const int32 Count = Clamp(Item->AffixCount, 0, MaxAffixes);
			for (int32 i = 0; i < Count; ++i)
			{
				const Affix& A = Item->Affixes[i];
				if (A.Stat < EAffixStat::Count)
				{
					Bonus[static_cast<int>(A.Stat)] += A.Value;
				}
			}
		}

		const auto Get = [&Bonus](EAffixStat Stat) -> int32
		{
			return static_cast<int32>(Clamp<int64>(Bonus[static_cast<int>(Stat)], -1000000, 1000000));
		};

		EquippedStats Out;
		Out.Attr = Progress.Attr;
		Out.Attr.Strength += Get(EAffixStat::Strength);
		Out.Attr.Dexterity += Get(EAffixStat::Dexterity);
		Out.Attr.Vitality += Get(EAffixStat::Vitality);
		Out.Attr.Energy += Get(EAffixStat::Energy);

		Out.Derived = ComputeBaseStats(Progress.Class, Progress.Level, Out.Attr);
		Out.Derived.MaxLife += Get(EAffixStat::Life);
		Out.Derived.MaxMana += Get(EAffixStat::Mana);
		Out.Derived.AttackRating += Get(EAffixStat::AttackRating);
		Out.Derived.Defense += Get(EAffixStat::Defense);

		Out.Armor = static_cast<int32>(Clamp<int64>(Armor, 0, 1000000)) + Get(EAffixStat::Armor);
		Out.BlockPercent = BlockPercent;

		// La Fuerza escala el daño físico: +1 % por punto.
		const int32 Added = Get(EAffixStat::AddedPhysicalDamage);
		const int64 StrScale = 100 + (Out.Attr.Strength > 0 ? Out.Attr.Strength : 0);
		Out.WeaponDamage.Min = static_cast<int32>((Weapon.Min + Added) * StrScale / 100);
		Out.WeaponDamage.Max = static_cast<int32>((Weapon.Max + Added) * StrScale / 100);

		Out.ResistPercent[static_cast<int>(EDamageType::Fire)] = Get(EAffixStat::ResistFire);
		Out.ResistPercent[static_cast<int>(EDamageType::Cold)] = Get(EAffixStat::ResistCold);
		Out.ResistPercent[static_cast<int>(EDamageType::Lightning)] = Get(EAffixStat::ResistLightning);
		Out.ResistPercent[static_cast<int>(EDamageType::Poison)] = Get(EAffixStat::ResistPoison);
		Out.ResistPercent[static_cast<int>(EDamageType::Shadow)] = Get(EAffixStat::ResistShadow);

		Out.CritChancePermille = 50 + Get(EAffixStat::CritChancePermille);
		Out.LifeOnHit = Get(EAffixStat::LifeOnHit);
		Out.MagicFind = Get(EAffixStat::MagicFind);
		return Out;
	}

	AttackProfile MakeAttackProfile(const CharacterProgress& Progress, const EquippedStats& Stats)
	{
		AttackProfile Attack;
		Attack.Damage[static_cast<int>(EDamageType::Physical)] = Stats.WeaponDamage;
		Attack.AttackRating = Stats.Derived.AttackRating;
		Attack.Level = Progress.Level;
		Attack.CritChance = static_cast<float>(Clamp(Stats.CritChancePermille, 0, 1000)) / 1000.0f;
		return Attack;
	}

	DefenseProfile MakeDefenseProfile(const CharacterProgress& Progress, const EquippedStats& Stats,
		const StatusContainer* Statuses)
	{
		DefenseProfile Defense;
		Defense.Defense = Stats.Derived.Defense;
		Defense.Armor = Stats.Armor;
		Defense.Level = Progress.Level;
		Defense.BlockChance = static_cast<float>(Clamp(Stats.BlockPercent, 0, 100)) / 100.0f;
		const int32 Penalty = Statuses != nullptr ? Statuses->ResistPenalty() : 0;
		for (int Type = 0; Type < DamageTypeCount; ++Type)
		{
			Defense.ResistPercent[Type] = Stats.ResistPercent[Type] - Penalty;
		}
		return Defense;
	}
}
