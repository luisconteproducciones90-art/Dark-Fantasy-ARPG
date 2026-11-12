#pragma once

#include "ArpgCore/Character/CharacterStats.h"
#include "ArpgCore/Enemy/Monster.h"
#include "ArpgCore/Inventory/Equipment.h"
#include "ArpgCore/Inventory/GridInventory.h"

#include <cstddef>
#include <vector>

namespace arpg
{
	inline constexpr int32 BagWidth = 10, BagHeight = 4;
	inline constexpr int32 StashWidth = 10, StashHeight = 10;
	inline constexpr int MaxCharacterNameLength = 23;
	inline constexpr int32 MaxDepth = 9999;
	inline constexpr int32 MaxPotions = 99;

	/** Todo lo que se guarda de un personaje. */
	struct CharacterSave
	{
		char Name[MaxCharacterNameLength + 1] = {};
		CharacterProgress Progress;
		int64 Gold = 0;
		EDifficulty Difficulty = EDifficulty::Pilgrim;
		uint64 WorldSeed = 0;
		int32 Depth = 1;          // nivel de mazmorra alcanzado
		int32 LifePotions = 0;
		int32 ManaPotions = 0;
		Equipment Gear;
		GridInventory Bag{BagWidth, BagHeight};
		GridInventory Stash{StashWidth, StashHeight};
	};

	enum class ESaveResult : uint8
	{
		Ok,
		TooShort,       // archivo truncado
		BadMagic,       // no es un guardado del juego
		NewerVersion,   // creado por una versión más nueva
		ChecksumFailed, // archivo dañado
		Corrupt         // contenido inválido (valores imposibles u objetos desconocidos)
	};

	/**
	 * Formato binario propio, little-endian, con versión y CRC-32.
	 * Los objetos se guardan por Id de texto, no por índice de tabla, para que
	 * añadir objetos al juego no rompa guardados antiguos.
	 * Escribir el archivo en disco (de forma atómica y con copia de respaldo)
	 * es responsabilidad de la capa de motor.
	 */
	void SerializeCharacter(const CharacterSave& Save, std::vector<uint8>& OutBytes);

	/** Valida todo antes de aceptar. Si no devuelve Ok, Out queda sin especificar y no debe usarse. */
	ESaveResult DeserializeCharacter(const uint8* Data, std::size_t Size, CharacterSave& Out);

	inline constexpr uint32 SaveFormatVersion = 1;
}
