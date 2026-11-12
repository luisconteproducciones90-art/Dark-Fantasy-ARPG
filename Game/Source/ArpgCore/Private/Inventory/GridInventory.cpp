#include "ArpgCore/Inventory/GridInventory.h"

namespace arpg
{
	GridInventory::GridInventory(int32 InWidth, int32 InHeight)
		: Width(Clamp(InWidth, 1, 64))
		, Height(Clamp(InHeight, 1, 64))
		, Cells(static_cast<size_t>(Width) * static_cast<size_t>(Height), 0)
	{
	}

	bool GridInventory::CanPlace(int32 X, int32 Y, int32 ItemWidth, int32 ItemHeight, int32 IgnoreId) const
	{
		if (ItemWidth < 1 || ItemHeight < 1 || X < 0 || Y < 0
			|| ItemWidth > Width || ItemHeight > Height
			|| X > Width - ItemWidth || Y > Height - ItemHeight)
		{
			return false;
		}
		for (int32 Cy = Y; Cy < Y + ItemHeight; ++Cy)
		{
			for (int32 Cx = X; Cx < X + ItemWidth; ++Cx)
			{
				const int32 Occupant = Cells[static_cast<size_t>(Cy * Width + Cx)];
				if (Occupant != 0 && Occupant != IgnoreId)
				{
					return false;
				}
			}
		}
		return true;
	}

	int32 GridInventory::Place(const ItemInstance& Item, int32 X, int32 Y)
	{
		const ItemBase* Base = GetItemBase(Item.BaseIndex);
		if (Base == nullptr || !CanPlace(X, Y, Base->Width, Base->Height))
		{
			return 0;
		}
		Entry E;
		E.Id = NextId++;
		E.X = X;
		E.Y = Y;
		E.Width = Base->Width;
		E.Height = Base->Height;
		E.Item = Item;
		Stamp(E, E.Id);
		Entries.push_back(E);
		return E.Id;
	}

	int32 GridInventory::AutoPlace(const ItemInstance& Item)
	{
		const ItemBase* Base = GetItemBase(Item.BaseIndex);
		if (Base == nullptr)
		{
			return 0;
		}
		for (int32 X = 0; X <= Width - Base->Width; ++X)
		{
			for (int32 Y = 0; Y <= Height - Base->Height; ++Y)
			{
				if (CanPlace(X, Y, Base->Width, Base->Height))
				{
					return Place(Item, X, Y);
				}
			}
		}
		return 0;
	}

	bool GridInventory::Remove(int32 Id, ItemInstance* OutItem)
	{
		const int32 Index = FindIndex(Id);
		if (Index < 0)
		{
			return false;
		}
		const Entry Removed = Entries[static_cast<size_t>(Index)];
		Stamp(Removed, 0);
		Entries.erase(Entries.begin() + Index);
		if (OutItem != nullptr)
		{
			*OutItem = Removed.Item;
		}
		return true;
	}

	bool GridInventory::Move(int32 Id, int32 NewX, int32 NewY)
	{
		const int32 Index = FindIndex(Id);
		if (Index < 0)
		{
			return false;
		}
		Entry& E = Entries[static_cast<size_t>(Index)];
		if (!CanPlace(NewX, NewY, E.Width, E.Height, Id))
		{
			return false;
		}
		Stamp(E, 0);
		E.X = NewX;
		E.Y = NewY;
		Stamp(E, Id);
		return true;
	}

	const GridInventory::Entry* GridInventory::Find(int32 Id) const
	{
		const int32 Index = FindIndex(Id);
		return Index < 0 ? nullptr : &Entries[static_cast<size_t>(Index)];
	}

	const GridInventory::Entry* GridInventory::At(int32 X, int32 Y) const
	{
		if (X < 0 || Y < 0 || X >= Width || Y >= Height)
		{
			return nullptr;
		}
		const int32 Occupant = Cells[static_cast<size_t>(Y * Width + X)];
		return Occupant == 0 ? nullptr : Find(Occupant);
	}

	int32 GridInventory::CountFreeCells() const
	{
		int32 Free = 0;
		for (const int32 Cell : Cells)
		{
			Free += Cell == 0 ? 1 : 0;
		}
		return Free;
	}

	void GridInventory::Stamp(const Entry& E, int32 Value)
	{
		for (int32 Cy = E.Y; Cy < E.Y + E.Height; ++Cy)
		{
			for (int32 Cx = E.X; Cx < E.X + E.Width; ++Cx)
			{
				Cells[static_cast<size_t>(Cy * Width + Cx)] = Value;
			}
		}
	}

	int32 GridInventory::FindIndex(int32 Id) const
	{
		if (Id <= 0)
		{
			return -1;
		}
		for (size_t i = 0; i < Entries.size(); ++i)
		{
			if (Entries[i].Id == Id)
			{
				return static_cast<int32>(i);
			}
		}
		return -1;
	}
}
