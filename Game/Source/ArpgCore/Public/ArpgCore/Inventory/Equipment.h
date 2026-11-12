#pragma once

#include "ArpgCore/Character/CharacterStats.h"
#include "ArpgCore/Combat/Damage.h"
#include "ArpgCore/Combat/StatusEffects.h"
#include "ArpgCore/Item/Item.h"

namespace arpg
{
	enum class EEquipSlot : uint8
	{
		Helmet, BodyArmor, Gloves, Boots, Belt, MainHand, OffHand, Ring1, Ring2, Amulet, Count
	};
	inline constexpr int EquipSlotCount = static_cast<int>(EEquipSlot::Count);

	/** Tipo de objeto que acepta cada ranura. */
	EItemType GetSlotItemType(EEquipSlot Slot);

	enum class EEquipResult : uint8
	{
		Ok,
		InvalidItem,
		WrongSlot,
		LevelTooLow,
		StrengthTooLow
	};

	/** Objetos equipados de un personaje. */
	class Equipment
	{
	public:
		const ItemInstance* Get(EEquipSlot Slot) const;

		/** Comprueba ranura y requisitos (nivel y Fuerza base, sin contar bonos de equipo). */
		EEquipResult CanEquip(const CharacterProgress& Progress, const ItemInstance& Item, EEquipSlot Slot) const;

		/**
		 * Equipa. Si la ranura estaba ocupada, el objeto anterior sale por
		 * OutPrevious y bOutHadPrevious = true (intercambio). Si falla no cambia nada.
		 */
		EEquipResult Equip(const CharacterProgress& Progress, const ItemInstance& Item, EEquipSlot Slot,
			ItemInstance& OutPrevious, bool& bOutHadPrevious);

		bool Unequip(EEquipSlot Slot, ItemInstance* OutItem = nullptr);

	private:
		ItemInstance Items[EquipSlotCount];
		bool Occupied[EquipSlotCount] = {};
	};

	/** Estadísticas finales del personaje con todo el equipo aplicado. */
	struct EquippedStats
	{
		Attributes Attr;          // atributos con bonos
		DerivedStats Derived;     // vida, maná, índice de ataque, defensa
		int32 Armor = 0;
		DamageRange WeaponDamage; // físico, ya con Fuerza y daño añadido
		int32 ResistPercent[DamageTypeCount] = {};
		int32 CritChancePermille = 50;
		int32 BlockPercent = 0;
		int32 LifeOnHit = 0;
		int32 MagicFind = 0;
	};

	EquippedStats ComputeEquippedStats(const CharacterProgress& Progress, const Equipment& Gear);

	/** Perfil de ataque básico con arma. */
	AttackProfile MakeAttackProfile(const CharacterProgress& Progress, const EquippedStats& Stats);

	/** Perfil defensivo; aplica la penalización de resistencias de la maldición si hay estados. */
	DefenseProfile MakeDefenseProfile(const CharacterProgress& Progress, const EquippedStats& Stats,
		const StatusContainer* Statuses = nullptr);
}
