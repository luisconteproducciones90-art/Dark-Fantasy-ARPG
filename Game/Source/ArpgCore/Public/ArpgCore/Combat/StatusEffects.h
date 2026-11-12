#pragma once

#include "ArpgCore/Core/Types.h"

namespace arpg
{
	enum class EStatusEffect : uint8
	{
		Burn,   // Quemadura: daño en el tiempo
		Chill,  // Escarcha: ralentiza (Magnitude = % de ralentización)
		Freeze, // Congelación: no puede actuar
		Poison, // Veneno: daño en el tiempo y anula la regeneración
		Stun,   // Aturdimiento: no puede actuar
		Shock,  // Electrocución: el siguiente golpe salta (lo consume CombatSystem)
		Curse,  // Maldición: baja resistencias (Magnitude = puntos de resistencia)
		Count
	};
	inline constexpr int StatusEffectCount = static_cast<int>(EStatusEffect::Count);

	struct StatusInstance
	{
		int32 RemainingMs = 0;
		int32 DamagePerSecond = 0;
		int32 Magnitude = 0;
		int32 DamageRemainderMilli = 0; // fracción de daño pendiente entre ticks
	};

	inline constexpr int32 MaxChillSlowPercent = 80;

	/**
	 * Estados activos de una criatura. Un estado por tipo: al reaplicar gana el
	 * más fuerte en cada campo (duración, daño y magnitud), no se acumulan.
	 * El tiempo va en milisegundos enteros para que el resultado no dependa de
	 * la tasa de fotogramas.
	 */
	class StatusContainer
	{
	public:
		/**
		 * DurationScalePercent permite a jefes acortar estados (p. ej. 40).
		 * No hace nada si la duración resultante es <= 0.
		 */
		void Apply(EStatusEffect Type, int32 DurationMs, int32 DamagePerSecond = 0, int32 Magnitude = 0,
			int32 DurationScalePercent = 100);

		/** Avanza el tiempo y devuelve el daño en el tiempo causado en este paso. */
		int32 Tick(int32 DeltaMs);

		void Remove(EStatusEffect Type);
		void Clear();

		bool Has(EStatusEffect Type) const;
		int32 RemainingMs(EStatusEffect Type) const;

		/** false si está aturdido o congelado. */
		bool CanAct() const;
		/** 100 = velocidad normal; 0 si no puede actuar. */
		int32 MoveSpeedPercent() const;
		bool BlocksRegeneration() const;
		/** Puntos que se restan a todas las resistencias elementales. */
		int32 ResistPenalty() const;

	private:
		StatusInstance Slots[StatusEffectCount];
	};
}
