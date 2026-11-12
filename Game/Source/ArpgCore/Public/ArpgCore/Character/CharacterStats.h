#pragma once

#include "ArpgCore/Core/Types.h"

namespace arpg
{
	enum class ECharacterClass : uint8
	{
		Warrior,     // Guerrero
		Sorcerer,    // Hechicero
		Hunter,      // Cazador
		Necromancer, // Nigromante
		Assassin,    // Asesino
		Paladin,     // Paladín
		Count
	};

	struct Attributes
	{
		int32 Strength = 0;
		int32 Dexterity = 0;
		int32 Vitality = 0;
		int32 Energy = 0;
	};

	/** Datos de equilibrio de una clase. En producción se cargan de tablas de datos. */
	struct ClassDefinition
	{
		const char* Id;
		Attributes BaseAttributes;
		float BaseLife;
		float BaseMana;
		float LifePerLevel;
		float ManaPerLevel;
		float LifePerVitality;
		float ManaPerEnergy;
	};

	struct DerivedStats
	{
		int32 MaxLife = 0;
		int32 MaxMana = 0;
		int32 AttackRating = 0;
		int32 Defense = 0;
	};

	inline constexpr int32 MaxCharacterLevel = 80;
	inline constexpr int32 AttributePointsPerLevel = 5;
	inline constexpr int32 SkillPointsPerLevel = 1;

	/** Devuelve la definición de la clase. Class debe ser < Count. */
	const ClassDefinition& GetClassDefinition(ECharacterClass Class);

	/** Estadísticas derivadas sin equipo (el equipo se suma en ItemSystem). */
	DerivedStats ComputeBaseStats(ECharacterClass Class, int32 Level, const Attributes& Attr);

	/** Experiencia necesaria para pasar de Level a Level + 1. 0 en nivel máximo. */
	int64 ExperienceToNextLevel(int32 Level);

	/** Estado de progresión de un personaje. Es lo que se guarda y replica. */
	struct CharacterProgress
	{
		ECharacterClass Class = ECharacterClass::Warrior;
		int32 Level = 1;
		int64 Experience = 0; // acumulada dentro del nivel actual
		Attributes Attr;
		int32 UnspentAttributePoints = 0;
		int32 UnspentSkillPoints = 0;
	};

	CharacterProgress CreateCharacter(ECharacterClass Class);

	/** Suma experiencia y aplica las subidas de nivel. Devuelve niveles ganados. */
	int32 AddExperience(CharacterProgress& Progress, int64 Amount);

	enum class EAttribute : uint8 { Strength, Dexterity, Vitality, Energy };

	/** Gasta puntos libres. Devuelve false (sin cambios) si no alcanzan o Points <= 0. */
	bool SpendAttributePoints(CharacterProgress& Progress, EAttribute Attribute, int32 Points);
}
