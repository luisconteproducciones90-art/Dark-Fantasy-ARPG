// ArpgCore — núcleo de reglas independiente del motor.
// Regla del módulo: solo C++20 estándar, sin excepciones, sin RTTI y sin
// cabeceras de Unreal. Así el mismo código corre en cliente, servidor
// dedicado y pruebas automáticas sin motor.
#pragma once

#include <cstdint>

namespace arpg
{
	using int32 = std::int32_t;
	using int64 = std::int64_t;
	using uint8 = std::uint8_t;
	using uint32 = std::uint32_t;
	using uint64 = std::uint64_t;

	template <typename T>
	constexpr T Clamp(T Value, T Lo, T Hi)
	{
		return Value < Lo ? Lo : (Value > Hi ? Hi : Value);
	}
}
