#include "ArpgCore/Save/SaveGame.h"

#include <cstring>

namespace arpg
{
	namespace
	{
		constexpr uint8 Magic[4] = {'V', 'H', 'S', 'V'};
		constexpr std::size_t HeaderSize = 12; // magia + versión + tamaño del contenido
		constexpr std::size_t FooterSize = 4;  // CRC-32 del contenido

		uint32 Crc32(const uint8* Data, std::size_t Size)
		{
			uint32 Crc = 0xFFFFFFFFu;
			for (std::size_t i = 0; i < Size; ++i)
			{
				Crc ^= Data[i];
				for (int Bit = 0; Bit < 8; ++Bit)
				{
					Crc = (Crc >> 1) ^ (0xEDB88320u & (0u - (Crc & 1u)));
				}
			}
			return ~Crc;
		}

		struct Writer
		{
			std::vector<uint8>& Bytes;

			void U8(uint8 V) { Bytes.push_back(V); }
			void U32(uint32 V)
			{
				for (int i = 0; i < 4; ++i) { Bytes.push_back(static_cast<uint8>(V >> (8 * i))); }
			}
			void U64(uint64 V)
			{
				for (int i = 0; i < 8; ++i) { Bytes.push_back(static_cast<uint8>(V >> (8 * i))); }
			}
			void I32(int32 V) { U32(static_cast<uint32>(V)); }
			void I64(int64 V) { U64(static_cast<uint64>(V)); }
			void Str(const char* Text)
			{
				std::size_t Length = std::strlen(Text);
				if (Length > 255) { Length = 255; }
				U8(static_cast<uint8>(Length));
				for (std::size_t i = 0; i < Length; ++i) { U8(static_cast<uint8>(Text[i])); }
			}
		};

		struct Reader
		{
			const uint8* Data;
			std::size_t Size;
			std::size_t Pos = 0;
			bool bOk = true;

			bool Need(std::size_t Count)
			{
				if (!bOk || Size - Pos < Count)
				{
					bOk = false;
					return false;
				}
				return true;
			}
			uint8 U8() { return Need(1) ? Data[Pos++] : uint8{0}; }
			uint32 U32()
			{
				if (!Need(4)) { return 0; }
				uint32 V = 0;
				for (int i = 0; i < 4; ++i) { V |= static_cast<uint32>(Data[Pos++]) << (8 * i); }
				return V;
			}
			uint64 U64()
			{
				if (!Need(8)) { return 0; }
				uint64 V = 0;
				for (int i = 0; i < 8; ++i) { V |= static_cast<uint64>(Data[Pos++]) << (8 * i); }
				return V;
			}
			int32 I32() { return static_cast<int32>(U32()); }
			int64 I64() { return static_cast<int64>(U64()); }
			/** Lee una cadena corta en Out (con terminador). false si no cabe. */
			bool Str(char* Out, std::size_t Capacity)
			{
				const std::size_t Length = U8();
				if (!Need(Length) || Length + 1 > Capacity)
				{
					bOk = false;
					return false;
				}
				std::memcpy(Out, Data + Pos, Length);
				Out[Length] = '\0';
				Pos += Length;
				return true;
			}
		};

		void WriteItem(Writer& W, const ItemInstance& Item)
		{
			const ItemBase* Base = GetItemBase(Item.BaseIndex);
			W.Str(Base != nullptr ? Base->Id : "");
			W.U8(static_cast<uint8>(Item.Rarity));
			W.U8(static_cast<uint8>(Clamp(Item.ItemLevel, 1, MaxItemLevel)));
			const int32 Count = Clamp(Item.AffixCount, 0, MaxAffixes);
			W.U8(static_cast<uint8>(Count));
			for (int32 i = 0; i < Count; ++i)
			{
				W.U8(static_cast<uint8>(Item.Affixes[i].Stat));
				W.I32(Item.Affixes[i].Value);
			}
		}

		bool ReadItem(Reader& R, ItemInstance& Out)
		{
			char Id[64];
			if (!R.Str(Id, sizeof(Id)))
			{
				return false;
			}
			Out = ItemInstance{};
			Out.BaseIndex = FindItemBase(Id);
			const uint8 Rarity = R.U8();
			Out.ItemLevel = R.U8();
			Out.AffixCount = R.U8();
			if (!R.bOk || Out.BaseIndex < 0 || Rarity >= static_cast<uint8>(ERarity::Count)
				|| Out.ItemLevel < 1 || Out.ItemLevel > MaxItemLevel || Out.AffixCount > MaxAffixes)
			{
				return false;
			}
			Out.Rarity = static_cast<ERarity>(Rarity);
			for (int32 i = 0; i < Out.AffixCount; ++i)
			{
				const uint8 Stat = R.U8();
				const int32 Value = R.I32();
				if (!R.bOk || Stat >= static_cast<uint8>(EAffixStat::Count) || Value < -1000000 || Value > 1000000)
				{
					return false;
				}
				Out.Affixes[i] = {static_cast<EAffixStat>(Stat), Value};
			}
			return true;
		}

		void WriteGrid(Writer& W, const GridInventory& Grid)
		{
			W.U32(static_cast<uint32>(Grid.GetEntries().size()));
			for (const GridInventory::Entry& E : Grid.GetEntries())
			{
				W.U8(static_cast<uint8>(E.X));
				W.U8(static_cast<uint8>(E.Y));
				WriteItem(W, E.Item);
			}
		}

		bool ReadGrid(Reader& R, GridInventory& Grid)
		{
			const uint32 Count = R.U32();
			if (!R.bOk || Count > static_cast<uint32>(Grid.GetWidth() * Grid.GetHeight()))
			{
				return false;
			}
			for (uint32 i = 0; i < Count; ++i)
			{
				const int32 X = R.U8();
				const int32 Y = R.U8();
				ItemInstance Item;
				// Place rechaza posiciones fuera del inventario y solapamientos.
				if (!ReadItem(R, Item) || Grid.Place(Item, X, Y) == 0)
				{
					return false;
				}
			}
			return true;
		}

		bool ReadPayload(Reader& R, CharacterSave& Out)
		{
			if (!R.Str(Out.Name, sizeof(Out.Name)))
			{
				return false;
			}
			CharacterProgress& P = Out.Progress;
			const uint8 Class = R.U8();
			P.Level = R.I32();
			P.Experience = R.I64();
			P.Attr.Strength = R.I32();
			P.Attr.Dexterity = R.I32();
			P.Attr.Vitality = R.I32();
			P.Attr.Energy = R.I32();
			P.UnspentAttributePoints = R.I32();
			P.UnspentSkillPoints = R.I32();
			Out.Gold = R.I64();
			const uint8 Difficulty = R.U8();
			Out.WorldSeed = R.U64();
			if (!R.bOk || Class >= static_cast<uint8>(ECharacterClass::Count)
				|| Difficulty >= static_cast<uint8>(EDifficulty::Count)
				|| P.Level < 1 || P.Level > MaxCharacterLevel || Out.Gold < 0)
			{
				return false;
			}
			P.Class = static_cast<ECharacterClass>(Class);
			Out.Difficulty = static_cast<EDifficulty>(Difficulty);

			const int64 Needed = ExperienceToNextLevel(P.Level);
			const bool bExperienceOk = P.Experience >= 0 && (Needed == 0 ? P.Experience == 0 : P.Experience < Needed);
			constexpr int32 MaxAttribute = 100000;
			const auto InRange = [](int32 V) { return V >= 0 && V <= MaxAttribute; };
			if (!bExperienceOk || !InRange(P.Attr.Strength) || !InRange(P.Attr.Dexterity)
				|| !InRange(P.Attr.Vitality) || !InRange(P.Attr.Energy)
				|| !InRange(P.UnspentAttributePoints) || !InRange(P.UnspentSkillPoints))
			{
				return false;
			}

			for (int Slot = 0; Slot < EquipSlotCount; ++Slot)
			{
				const uint8 Present = R.U8();
				if (!R.bOk || Present > 1)
				{
					return false;
				}
				if (Present == 1)
				{
					ItemInstance Item, Previous;
					bool bHadPrevious = false;
					if (!ReadItem(R, Item)
						|| Out.Gear.Equip(P, Item, static_cast<EEquipSlot>(Slot), Previous, bHadPrevious) != EEquipResult::Ok)
					{
						return false;
					}
				}
			}
			return ReadGrid(R, Out.Bag) && ReadGrid(R, Out.Stash) && R.bOk && R.Pos == R.Size;
		}
	}

	void SerializeCharacter(const CharacterSave& Save, std::vector<uint8>& OutBytes)
	{
		std::vector<uint8> Payload;
		Writer W{Payload};

		char SafeName[MaxCharacterNameLength + 1];
		std::memcpy(SafeName, Save.Name, sizeof(SafeName));
		SafeName[MaxCharacterNameLength] = '\0';
		W.Str(SafeName);

		const CharacterProgress& P = Save.Progress;
		W.U8(static_cast<uint8>(P.Class));
		W.I32(P.Level);
		W.I64(P.Experience);
		W.I32(P.Attr.Strength);
		W.I32(P.Attr.Dexterity);
		W.I32(P.Attr.Vitality);
		W.I32(P.Attr.Energy);
		W.I32(P.UnspentAttributePoints);
		W.I32(P.UnspentSkillPoints);
		W.I64(Save.Gold);
		W.U8(static_cast<uint8>(Save.Difficulty));
		W.U64(Save.WorldSeed);

		for (int Slot = 0; Slot < EquipSlotCount; ++Slot)
		{
			const ItemInstance* Item = Save.Gear.Get(static_cast<EEquipSlot>(Slot));
			W.U8(Item != nullptr ? uint8{1} : uint8{0});
			if (Item != nullptr)
			{
				WriteItem(W, *Item);
			}
		}
		WriteGrid(W, Save.Bag);
		WriteGrid(W, Save.Stash);

		OutBytes.clear();
		Writer Out{OutBytes};
		for (const uint8 Byte : Magic) { Out.U8(Byte); }
		Out.U32(SaveFormatVersion);
		Out.U32(static_cast<uint32>(Payload.size()));
		OutBytes.insert(OutBytes.end(), Payload.begin(), Payload.end());
		Out.U32(Crc32(Payload.data(), Payload.size()));
	}

	ESaveResult DeserializeCharacter(const uint8* Data, std::size_t Size, CharacterSave& Out)
	{
		if (Data == nullptr || Size < HeaderSize + FooterSize)
		{
			return ESaveResult::TooShort;
		}
		if (std::memcmp(Data, Magic, sizeof(Magic)) != 0)
		{
			return ESaveResult::BadMagic;
		}
		Reader Header{Data + 4, 8};
		const uint32 Version = Header.U32();
		const uint32 PayloadSize = Header.U32();
		if (Version == 0)
		{
			return ESaveResult::Corrupt;
		}
		if (Version > SaveFormatVersion)
		{
			return ESaveResult::NewerVersion;
		}
		const std::size_t Available = Size - HeaderSize - FooterSize;
		if (PayloadSize > Available)
		{
			return ESaveResult::TooShort;
		}
		if (PayloadSize != Available)
		{
			return ESaveResult::Corrupt;
		}
		const uint8* Payload = Data + HeaderSize;
		Reader Footer{Payload + PayloadSize, FooterSize};
		if (Footer.U32() != Crc32(Payload, PayloadSize))
		{
			return ESaveResult::ChecksumFailed;
		}

		Out = CharacterSave{};
		Reader R{Payload, PayloadSize};
		return ReadPayload(R, Out) ? ESaveResult::Ok : ESaveResult::Corrupt;
	}
}
