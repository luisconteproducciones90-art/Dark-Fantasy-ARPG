#pragma once

#include "ArpgCore/Core/Types.h"
#include "ArpgCore/Item/Item.h"

#include <vector>

namespace arpg
{
	/**
	 * Inventario clásico por cuadrícula: cada objeto ocupa un rectángulo de
	 * casillas. Sirve para mochila, almacén y tienda. Los objetos se identifican
	 * por un Id estable (> 0) válido mientras sigan dentro del inventario.
	 */
	class GridInventory
	{
	public:
		struct Entry
		{
			int32 Id = 0;
			int32 X = 0;
			int32 Y = 0;
			int32 Width = 0;
			int32 Height = 0;
			ItemInstance Item;
		};

		GridInventory(int32 InWidth, int32 InHeight);

		int32 GetWidth() const { return Width; }
		int32 GetHeight() const { return Height; }
		const std::vector<Entry>& GetEntries() const { return Entries; }

		/** true si el rectángulo cabe y está libre. IgnoreId permite recolocar un objeto sobre sí mismo. */
		bool CanPlace(int32 X, int32 Y, int32 ItemWidth, int32 ItemHeight, int32 IgnoreId = 0) const;

		/** Coloca en una posición concreta. Devuelve el Id o 0 si no se puede. */
		int32 Place(const ItemInstance& Item, int32 X, int32 Y);

		/** Coloca en el primer hueco libre (por columnas, de arriba abajo). Devuelve el Id o 0. */
		int32 AutoPlace(const ItemInstance& Item);

		/** Saca el objeto. Devuelve false si el Id no existe. */
		bool Remove(int32 Id, ItemInstance* OutItem = nullptr);

		/** Mueve dentro del mismo inventario. Si no cabe, no cambia nada. */
		bool Move(int32 Id, int32 NewX, int32 NewY);

		const Entry* Find(int32 Id) const;
		/** Objeto que ocupa una casilla, o nullptr. */
		const Entry* At(int32 X, int32 Y) const;

		int32 CountFreeCells() const;

	private:
		void Stamp(const Entry& E, int32 Value);
		int32 FindIndex(int32 Id) const;

		int32 Width;
		int32 Height;
		int32 NextId = 1;
		std::vector<int32> Cells; // Id del ocupante o 0
		std::vector<Entry> Entries;
	};
}
