#include "ArpgCore/Character/CharacterStats.h"

namespace arpg
{
	namespace
	{
		// Orden idéntico a ECharacterClass.
		constexpr ClassDefinition ClassTable[] = {
			// Id            Fue Des Vit Ene   Vida  Maná  V/Nv  M/Nv  V/Vit M/Ene
			{"warrior",     {30, 20, 25, 10}, 55.f, 10.f, 2.5f, 1.0f, 4.0f, 1.0f},
			{"sorcerer",    {10, 25, 15, 35}, 40.f, 35.f, 1.0f, 2.5f, 2.0f, 2.5f},
			{"hunter",      {20, 30, 20, 15}, 50.f, 15.f, 2.0f, 1.5f, 3.0f, 1.5f},
			{"necromancer", {15, 25, 15, 30}, 45.f, 25.f, 1.5f, 2.0f, 2.0f, 2.0f},
			{"assassin",    {20, 30, 20, 15}, 50.f, 20.f, 2.0f, 1.5f, 3.0f, 1.75f},
			{"paladin",     {25, 20, 25, 15}, 55.f, 15.f, 2.0f, 1.5f, 3.0f, 1.5f},
		};
		static_assert(sizeof(ClassTable) / sizeof(ClassTable[0]) == static_cast<unsigned>(ECharacterClass::Count));
	}

	const ClassDefinition& GetClassDefinition(ECharacterClass Class)
	{
		return ClassTable[static_cast<int>(Class)];
	}

	DerivedStats ComputeBaseStats(ECharacterClass Class, int32 Level, const Attributes& Attr)
	{
		const ClassDefinition& Def = GetClassDefinition(Class);
		const int32 Lvl = Clamp(Level, 1, MaxCharacterLevel);
		const float LevelsGained = static_cast<float>(Lvl - 1);

		DerivedStats Out;
		Out.MaxLife = static_cast<int32>(Def.BaseLife + Def.LifePerLevel * LevelsGained
			+ Def.LifePerVitality * static_cast<float>(Attr.Vitality - Def.BaseAttributes.Vitality));
		Out.MaxMana = static_cast<int32>(Def.BaseMana + Def.ManaPerLevel * LevelsGained
			+ Def.ManaPerEnergy * static_cast<float>(Attr.Energy - Def.BaseAttributes.Energy));
		Out.AttackRating = Attr.Dexterity * 5 + Lvl * 4;
		Out.Defense = Attr.Dexterity / 4;
		if (Out.MaxLife < 1) { Out.MaxLife = 1; }
		if (Out.MaxMana < 0) { Out.MaxMana = 0; }
		return Out;
	}

	int64 ExperienceToNextLevel(int32 Level)
	{
		if (Level < 1 || Level >= MaxCharacterLevel)
		{
			return 0;
		}
		const int64 L = Level;
		return 80 * L * L + 120 * L;
	}

	CharacterProgress CreateCharacter(ECharacterClass Class)
	{
		CharacterProgress P;
		P.Class = Class;
		P.Attr = GetClassDefinition(Class).BaseAttributes;
		return P;
	}

	int32 AddExperience(CharacterProgress& Progress, int64 Amount)
	{
		if (Amount <= 0 || Progress.Level >= MaxCharacterLevel)
		{
			return 0;
		}
		int32 Gained = 0;
		Progress.Experience += Amount;
		while (Progress.Level < MaxCharacterLevel)
		{
			const int64 Needed = ExperienceToNextLevel(Progress.Level);
			if (Progress.Experience < Needed)
			{
				break;
			}
			Progress.Experience -= Needed;
			++Progress.Level;
			++Gained;
			Progress.UnspentAttributePoints += AttributePointsPerLevel;
			Progress.UnspentSkillPoints += SkillPointsPerLevel;
		}
		if (Progress.Level >= MaxCharacterLevel)
		{
			Progress.Experience = 0;
		}
		return Gained;
	}

	bool SpendAttributePoints(CharacterProgress& Progress, EAttribute Attribute, int32 Points)
	{
		if (Points <= 0 || Points > Progress.UnspentAttributePoints)
		{
			return false;
		}
		switch (Attribute)
		{
		case EAttribute::Strength:  Progress.Attr.Strength += Points; break;
		case EAttribute::Dexterity: Progress.Attr.Dexterity += Points; break;
		case EAttribute::Vitality:  Progress.Attr.Vitality += Points; break;
		case EAttribute::Energy:    Progress.Attr.Energy += Points; break;
		default: return false;
		}
		Progress.UnspentAttributePoints -= Points;
		return true;
	}
}
