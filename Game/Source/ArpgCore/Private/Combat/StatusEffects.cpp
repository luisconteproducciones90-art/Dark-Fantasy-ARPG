#include "ArpgCore/Combat/StatusEffects.h"

namespace arpg
{
	namespace
	{
		constexpr int Index(EStatusEffect Type) { return static_cast<int>(Type); }
		constexpr int32 Max(int32 A, int32 B) { return A > B ? A : B; }
	}

	void StatusContainer::Apply(EStatusEffect Type, int32 DurationMs, int32 DamagePerSecond, int32 Magnitude,
		int32 DurationScalePercent)
	{
		if (Type >= EStatusEffect::Count)
		{
			return;
		}
		const int64 Scaled = static_cast<int64>(DurationMs) * Clamp(DurationScalePercent, 0, 1000) / 100;
		if (Scaled <= 0)
		{
			return;
		}
		StatusInstance& Slot = Slots[Index(Type)];
		const bool bWasActive = Slot.RemainingMs > 0;
		if (!bWasActive)
		{
			Slot = StatusInstance{};
		}
		Slot.RemainingMs = Max(Slot.RemainingMs, static_cast<int32>(Clamp<int64>(Scaled, 0, INT32_MAX)));
		Slot.DamagePerSecond = Max(Slot.DamagePerSecond, Max(DamagePerSecond, 0));
		Slot.Magnitude = Max(Slot.Magnitude, Max(Magnitude, 0));
	}

	int32 StatusContainer::Tick(int32 DeltaMs)
	{
		if (DeltaMs <= 0)
		{
			return 0;
		}
		int64 TotalDamage = 0;
		for (StatusInstance& Slot : Slots)
		{
			if (Slot.RemainingMs <= 0)
			{
				continue;
			}
			// Solo cuenta el tiempo que el estado sigue activo dentro del paso.
			const int32 ActiveMs = DeltaMs < Slot.RemainingMs ? DeltaMs : Slot.RemainingMs;
			if (Slot.DamagePerSecond > 0)
			{
				const int64 Milli = static_cast<int64>(Slot.DamagePerSecond) * ActiveMs + Slot.DamageRemainderMilli;
				TotalDamage += Milli / 1000;
				Slot.DamageRemainderMilli = static_cast<int32>(Milli % 1000);
			}
			Slot.RemainingMs -= ActiveMs;
			if (Slot.RemainingMs <= 0)
			{
				Slot = StatusInstance{};
			}
		}
		return static_cast<int32>(Clamp<int64>(TotalDamage, 0, INT32_MAX));
	}

	void StatusContainer::Remove(EStatusEffect Type)
	{
		if (Type < EStatusEffect::Count)
		{
			Slots[Index(Type)] = StatusInstance{};
		}
	}

	void StatusContainer::Clear()
	{
		for (StatusInstance& Slot : Slots)
		{
			Slot = StatusInstance{};
		}
	}

	bool StatusContainer::Has(EStatusEffect Type) const
	{
		return Type < EStatusEffect::Count && Slots[Index(Type)].RemainingMs > 0;
	}

	int32 StatusContainer::RemainingMs(EStatusEffect Type) const
	{
		return Type < EStatusEffect::Count ? Slots[Index(Type)].RemainingMs : 0;
	}

	bool StatusContainer::CanAct() const
	{
		return !Has(EStatusEffect::Stun) && !Has(EStatusEffect::Freeze);
	}

	int32 StatusContainer::MoveSpeedPercent() const
	{
		if (!CanAct())
		{
			return 0;
		}
		if (Has(EStatusEffect::Chill))
		{
			return 100 - Clamp(Slots[Index(EStatusEffect::Chill)].Magnitude, 0, MaxChillSlowPercent);
		}
		return 100;
	}

	bool StatusContainer::BlocksRegeneration() const
	{
		return Has(EStatusEffect::Poison);
	}

	int32 StatusContainer::ResistPenalty() const
	{
		return Has(EStatusEffect::Curse) ? Slots[Index(EStatusEffect::Curse)].Magnitude : 0;
	}
}
