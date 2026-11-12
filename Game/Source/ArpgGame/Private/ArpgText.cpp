#include "ArpgText.h"

#include "Misc/DateTime.h"

// Los acentos van como \u00XX para no depender de la codificación con que se compile el archivo.

namespace
{
	struct FNamePair
	{
		const char* Id;
		const TCHAR* Text;
	};

	const FNamePair ItemNames[] = {
		{"leather_cap", TEXT("Gorro de cuero")},
		{"iron_helm", TEXT("Yelmo de hierro")},
		{"padded_coat", TEXT("Jubón acolchado")},
		{"chain_hauberk", TEXT("Cota de malla")},
		{"hide_gloves", TEXT("Guantes de piel")},
		{"worn_boots", TEXT("Botas gastadas")},
		{"rope_belt", TEXT("Cinturón de soga")},
		{"short_sword", TEXT("Espada corta")},
		{"war_axe", TEXT("Hacha de guerra")},
		{"hunting_bow", TEXT("Arco de caza")},
		{"ash_staff", TEXT("Báculo de fresno")},
		{"round_shield", TEXT("Rodela")},
		{"copper_ring", TEXT("Anillo de cobre")},
		{"bone_amulet", TEXT("Amuleto de hueso")},
	};

	const FNamePair MonsterNames[] = {
		{"ash_walker", TEXT("Caminante de ceniza")},
		{"bone_archer", TEXT("Arquero de hueso")},
		{"mire_boar", TEXT("Jabalí del fango")},
		{"grave_caller", TEXT("Llamador de tumbas")},
		{"cinder_acolyte", TEXT("Acólito de la brasa")},
		{"burrower", TEXT("Excavador")},
		{"bloat", TEXT("Hinchado")},
	};

	FString Lookup(const FNamePair* Table, int32 Count, const char* Id)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			if (FCStringAnsi::Strcmp(Table[i].Id, Id) == 0)
			{
				return Table[i].Text;
			}
		}
		return FString(ANSI_TO_TCHAR(Id));
	}
}

FString ArpgText::ItemName(const arpg::ItemInstance& Item)
{
	const arpg::ItemBase* Base = arpg::GetItemBase(Item.BaseIndex);
	if (Base == nullptr)
	{
		return TEXT("?");
	}
	static const TCHAR* const RarityNames[] = {
		TEXT(""), TEXT(" mágico"), TEXT(" raro"), TEXT(" épico"), TEXT(" legendario"), TEXT(" mítico")};
	const int32 RarityIndex = FMath::Clamp(static_cast<int32>(Item.Rarity), 0, 5);
	return Lookup(ItemNames, static_cast<int32>(UE_ARRAY_COUNT(ItemNames)), Base->Id) + RarityNames[RarityIndex];
}

FString ArpgText::AffixLine(const arpg::Affix& Affix)
{
	// Mismo orden que arpg::EAffixStat.
	static const TCHAR* const Suffixes[] = {
		TEXT(" Fuerza"), TEXT(" Destreza"), TEXT(" Vitalidad"), TEXT(" Energ\u00EDa"),
		TEXT(" Vida"), TEXT(" Man\u00E1"), TEXT(" \u00CDndice de ataque"), TEXT(" Defensa"), TEXT(" Armadura"),
		TEXT("% Res. fuego"), TEXT("% Res. fr\u00EDo"), TEXT("% Res. rayo"), TEXT("% Res. veneno"), TEXT("% Res. sombra"),
		TEXT("\u2030 Cr\u00EDtico"), TEXT(" Da\u00F1o f\u00EDsico"), TEXT(" Vida por golpe"), TEXT("% Hallazgo m\u00E1gico")};
	const int32 Index = static_cast<int32>(Affix.Stat);
	if (Index < 0 || Index >= static_cast<int32>(UE_ARRAY_COUNT(Suffixes)))
	{
		return TEXT("?");
	}
	return FString::Printf(TEXT("+%d"), Affix.Value) + Suffixes[Index];
}

FLinearColor ArpgText::RarityColor(arpg::ERarity Rarity)
{
	switch (Rarity)
	{
	case arpg::ERarity::Magic:     return FLinearColor(0.35f, 0.5f, 1.f);
	case arpg::ERarity::Rare:      return FLinearColor(1.f, 0.9f, 0.3f);
	case arpg::ERarity::Epic:      return FLinearColor(0.75f, 0.35f, 1.f);
	case arpg::ERarity::Legendary: return FLinearColor(1.f, 0.5f, 0.1f);
	case arpg::ERarity::Mythic:    return FLinearColor(1.f, 0.2f, 0.2f);
	default:                       return FLinearColor(0.85f, 0.85f, 0.85f);
	}
}

FString ArpgText::MonsterName(int32 DefIndex, arpg::EMonsterRank Rank)
{
	const arpg::MonsterDef* Def = arpg::GetMonsterDef(DefIndex);
	if (Def == nullptr)
	{
		return TEXT("?");
	}
	const FString Name = Lookup(MonsterNames, static_cast<int32>(UE_ARRAY_COUNT(MonsterNames)), Def->Id);
	switch (Rank)
	{
	case arpg::EMonsterRank::Champion: return Name + TEXT(" (campeón)");
	case arpg::EMonsterRank::Elite:    return Name + TEXT(" (élite)");
	case arpg::EMonsterRank::Boss:     return Name + TEXT(" (JEFE)");
	default:                           return Name;
	}
}

FString ArpgText::SlotName(arpg::EEquipSlot Slot)
{
	static const TCHAR* const Names[] = {
		TEXT("Casco"), TEXT("Armadura"), TEXT("Guantes"), TEXT("Botas"), TEXT("Cinturón"),
		TEXT("Arma"), TEXT("Escudo"), TEXT("Anillo 1"), TEXT("Anillo 2"), TEXT("Amuleto")};
	const int32 Index = static_cast<int32>(Slot);
	return Index >= 0 && Index < static_cast<int32>(UE_ARRAY_COUNT(Names)) ? Names[Index] : TEXT("?");
}

FString ArpgText::ClassName(arpg::ECharacterClass Class)
{
	static const TCHAR* const Names[] = {
		TEXT("Guerrero"), TEXT("Hechicero"), TEXT("Cazador"), TEXT("Nigromante"), TEXT("Asesino"), TEXT("Paladín")};
	const int32 Index = static_cast<int32>(Class);
	return Index >= 0 && Index < static_cast<int32>(UE_ARRAY_COUNT(Names)) ? Names[Index] : TEXT("?");
}

FString ArpgText::SkillName(int32 SkillIndex)
{
	// Mismo orden que la tabla de habilidades de ArpgCore.
	static const TCHAR* const Names[] = {
		TEXT("Tajo amplio"), TEXT("Dardo de brasa"), TEXT("Disparo perforante"),
		TEXT("P\u00FAa de hueso"), TEXT("Golpe venenoso"), TEXT("Juicio")};
	return SkillIndex >= 0 && SkillIndex < static_cast<int32>(UE_ARRAY_COUNT(Names)) ? Names[SkillIndex] : TEXT("-");
}

arpg::Rng& ArpgGlobalRng()
{
	static arpg::Rng Instance(static_cast<arpg::uint64>(FDateTime::UtcNow().GetTicks()));
	return Instance;
}
