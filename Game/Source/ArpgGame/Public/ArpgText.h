#pragma once

#include "CoreMinimal.h"
#include "ArpgCore/Core/Rng.h"
#include "ArpgCore/Enemy/Monster.h"
#include "ArpgCore/Inventory/Equipment.h"
#include "ArpgCore/Item/Item.h"

/** Textos en pantalla. Cuando haya localización pasarán a tablas de cadenas. */
namespace ArpgText
{
	ARPGGAME_API FString ItemName(const arpg::ItemInstance& Item);
	ARPGGAME_API FString AffixLine(const arpg::Affix& Affix);
	ARPGGAME_API FLinearColor RarityColor(arpg::ERarity Rarity);
	ARPGGAME_API FString MonsterName(int32 DefIndex, arpg::EMonsterRank Rank);
	ARPGGAME_API FString SlotName(arpg::EEquipSlot Slot);
	ARPGGAME_API FString ClassName(arpg::ECharacterClass Class);
	ARPGGAME_API FString SkillName(int32 SkillIndex);
}

/** Generador aleatorio de la partida. En multijugador (Fase 12) solo existirá en el servidor. */
ARPGGAME_API arpg::Rng& ArpgGlobalRng();
