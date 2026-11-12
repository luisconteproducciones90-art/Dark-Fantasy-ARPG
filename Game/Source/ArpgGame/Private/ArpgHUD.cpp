#include "ArpgHUD.h"

#include "ArpgCombatComponent.h"
#include "ArpgDungeonActor.h"
#include "ArpgGameMode.h"
#include "ArpgLootPickup.h"
#include "ArpgMonster.h"
#include "ArpgPlayerCharacter.h"
#include "ArpgPlayerController.h"
#include "ArpgText.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	const FLinearColor PanelColor(0.03f, 0.025f, 0.02f, 0.92f);
	const FLinearColor FrameColor(0.32f, 0.26f, 0.16f, 1.f); // hierro envejecido
	const FLinearColor TextColor(0.86f, 0.82f, 0.72f, 1.f);
	const FLinearColor DimColor(0.55f, 0.52f, 0.46f, 1.f);
	const FLinearColor LifeColor(0.75f, 0.04f, 0.03f, 1.f);
	const FLinearColor ManaColor(0.05f, 0.18f, 0.85f, 1.f);

	constexpr float CellSize = 36.f;
}

void AArpgHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCentered)
{
	if (bCentered)
	{
		float Width = 0.f, Height = 0.f;
		GetTextSize(Text, Width, Height, Font, Scale);
		X -= Width * 0.5f;
	}
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A), X + 1.f, Y + 1.f, Font, Scale); // sombra para legibilidad
	DrawText(Text, Color, X, Y, Font, Scale);
}

void AArpgHUD::AddRect(float X, float Y, float W, float H, EUiKind Kind, int32 Id)
{
	Rects.Add({FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)), Kind, Id});
}

void AArpgHUD::ShowMessage(const FString& Text)
{
	Message = Text;
	MessageTimeLeft = 2.5f;
}

void AArpgHUD::ShowHit(const FVector& WorldLocation, const arpg::AttackOutcome& Outcome)
{
	FFloatingText Entry;
	Entry.WorldLocation = WorldLocation + FVector(0.f, 0.f, 200.f);
	Entry.Age = 0.f;
	switch (Outcome.Hit.Outcome)
	{
	case arpg::EHitOutcome::Miss:
		Entry.Text = TEXT("Fallo");
		Entry.Color = DimColor;
		break;
	case arpg::EHitOutcome::Blocked:
		Entry.Text = TEXT("Bloqueado");
		Entry.Color = DimColor;
		break;
	case arpg::EHitOutcome::Critical:
		Entry.Text = FString::Printf(TEXT("%d!"), Outcome.DamageDealt);
		Entry.Color = FLinearColor(1.f, 0.85f, 0.2f);
		break;
	default:
		Entry.Text = FString::FromInt(Outcome.DamageDealt);
		Entry.Color = FLinearColor::White;
		break;
	}
	FloatingTexts.Add(Entry);
}

bool AArpgHUD::HandleClick(const FVector2D& Position, bool bRightButton)
{
	AArpgPlayerCharacter* Player = Cast<AArpgPlayerCharacter>(GetOwningPawn());
	// Los últimos rectángulos dibujados quedan encima: se comprueban primero.
	for (int32 i = Rects.Num() - 1; i >= 0; --i)
	{
		const FUiRect& Rect = Rects[i];
		if (!Rect.Box.IsInside(Position))
		{
			continue;
		}
		if (Player != nullptr)
		{
			switch (Rect.Kind)
			{
			case EUiKind::BagItem:
				if (bRightButton) { Player->DropFromBag(Rect.Id); }
				else { Player->EquipFromBag(Rect.Id); }
				break;
			case EUiKind::EquipSlot:
				if (!bRightButton) { Player->UnequipToBag(static_cast<arpg::EEquipSlot>(Rect.Id)); }
				break;
			case EUiKind::AttributePlus:
				if (!bRightButton) { Player->SpendAttributePoint(static_cast<arpg::EAttribute>(Rect.Id)); }
				break;
			default:
				break;
			}
		}
		return true;
	}
	return false;
}

void AArpgHUD::DrawHUD()
{
	Super::DrawHUD();

	if (Canvas == nullptr)
	{
		return;
	}
	if (Font == nullptr && GEngine != nullptr)
	{
		Font = GEngine->GetMediumFont();
	}
	Rects.Reset();

	AArpgPlayerCharacter* Player = Cast<AArpgPlayerCharacter>(GetOwningPawn());
	if (Player == nullptr)
	{
		return;
	}
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const float Width = static_cast<float>(Canvas->SizeX);
	const float Height = static_cast<float>(Canvas->SizeY);
	const arpg::Combatant& Body = Player->GetCombat()->Body;

	// Destellos de las esferas: se detectan comparando con el fotograma anterior.
	if (PreviousLife >= 0 && Body.Life < PreviousLife) { LifeFlash = 0.3f; }
	if (PreviousLife >= 0 && Body.Life > PreviousLife) { LifeGlow = 0.6f; }
	if (PreviousMana >= 0 && Body.Mana < PreviousMana) { ManaFlash = 0.3f; }
	if (PreviousMana >= 0 && Body.Mana > PreviousMana) { ManaGlow = 0.6f; }
	PreviousLife = Body.Life;
	PreviousMana = Body.Mana;
	LifeFlash = FMath::Max(0.f, LifeFlash - DeltaSeconds);
	LifeGlow = FMath::Max(0.f, LifeGlow - DeltaSeconds);
	ManaFlash = FMath::Max(0.f, ManaFlash - DeltaSeconds);
	ManaGlow = FMath::Max(0.f, ManaGlow - DeltaSeconds);

	DrawWorldLabels(Player);

	// Vida crítica: viñeta roja que late en los bordes.
	const float LifeFraction = Player->GetCombat()->GetLifeFraction();
	if (Body.IsAlive() && LifeFraction < 0.25f)
	{
		const float Pulse = 0.18f + 0.14f * FMath::Sin(GetWorld()->GetTimeSeconds() * 7.f);
		const FLinearColor Vignette(0.7f, 0.f, 0.f, Pulse);
		const float Band = 46.f;
		DrawRect(Vignette, 0.f, 0.f, Width, Band);
		DrawRect(Vignette, 0.f, Height - Band, Width, Band);
		DrawRect(Vignette, 0.f, Band, Band, Height - 2.f * Band);
		DrawRect(Vignette, Width - Band, Band, Band, Height - 2.f * Band);
	}

	DrawTopInfo(Player);
	DrawMinimap(Player);
	DrawBottomBar(Player);
	if (bInventoryOpen)
	{
		DrawInventory(Player);
	}

	if (MessageTimeLeft > 0.f)
	{
		MessageTimeLeft -= DeltaSeconds;
		const float Alpha = FMath::Clamp(MessageTimeLeft, 0.f, 1.f);
		Label(Message, Width * 0.5f, Height * 0.3f, FLinearColor(1.f, 0.9f, 0.6f, Alpha), 2.f, true);
	}
	if (!Body.IsAlive())
	{
		DrawRect(FLinearColor(0.15f, 0.f, 0.f, 0.45f), 0.f, 0.f, Width, Height);
		Label(TEXT("Has caído. El juramento te devuelve a la entrada..."), Width * 0.5f, Height * 0.42f,
			FLinearColor(0.9f, 0.2f, 0.15f), 2.2f, true);
	}
}

// ---------------------------------------------------------------- Esferas y barra inferior

void AArpgHUD::DrawOrb(float CenterX, float CenterY, float Radius, float Fraction, const FLinearColor& Liquid,
	float Flash, float Glow, const FString& Text)
{
	const float Time = GetWorld()->GetTimeSeconds();
	const float Step = 2.f;

	// Aro metálico.
	const float Outer = Radius + 6.f;
	for (float Y = -Outer; Y < Outer; Y += Step)
	{
		const float Half = FMath::Sqrt(FMath::Max(0.f, Outer * Outer - Y * Y));
		DrawRect(FrameColor, CenterX - Half, CenterY + Y, Half * 2.f, Step);
	}

	// Nivel del líquido con un vaivén suave de la superficie.
	const float Clamped = FMath::Clamp(Fraction, 0.f, 1.f);
	const float Wave = Clamped > 0.f && Clamped < 1.f ? FMath::Sin(Time * 2.4f) * 2.f : 0.f;
	const float SurfaceY = Radius - 2.f * Radius * Clamped + Wave; // relativo al centro

	const FLinearColor Empty(0.02f, 0.02f, 0.03f, 1.f);
	const FLinearColor FlashColor(1.f, 0.9f, 0.85f, 1.f);
	for (float Y = -Radius; Y < Radius; Y += Step)
	{
		const float Half = FMath::Sqrt(FMath::Max(0.f, Radius * Radius - Y * Y));
		FLinearColor Color = Empty;
		if (Y >= SurfaceY)
		{
			// Más oscuro hacia el fondo, con una franja clara en la superficie y ondas internas.
			const float Depth = (Y - SurfaceY) / (2.f * Radius);
			const float Ripple = 0.06f * FMath::Sin(Time * 3.f + Y * 0.22f);
			const float Shade = FMath::Clamp(1.15f - Depth * 0.75f + Ripple, 0.35f, 1.3f);
			Color = Liquid * Shade;
			if (Y - SurfaceY < 4.f)
			{
				Color = FMath::Lerp(Color, FLinearColor::White, 0.3f + Glow * 0.6f);
			}
			Color = FMath::Lerp(Color, FlashColor, FMath::Clamp(Flash * 2.5f, 0.f, 0.8f));
			Color.A = 1.f;
		}
		DrawRect(Color, CenterX - Half, CenterY + Y, Half * 2.f, Step);
	}

	// Reflejo del cristal.
	for (float Y = -Radius * 0.75f; Y < -Radius * 0.35f; Y += Step)
	{
		const float T = (Y + Radius * 0.75f) / (Radius * 0.4f);
		const float Half = Radius * 0.22f * FMath::Sin(T * PI);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.10f), CenterX - Radius * 0.35f - Half, CenterY + Y, Half * 2.f, Step);
	}

	Label(Text, CenterX, CenterY - 9.f, FLinearColor::White, 1.2f, true);
}

void AArpgHUD::DrawBottomBar(AArpgPlayerCharacter* Player)
{
	const float Width = static_cast<float>(Canvas->SizeX);
	const float Height = static_cast<float>(Canvas->SizeY);
	const arpg::Combatant& Body = Player->GetCombat()->Body;
	const float Radius = FMath::Clamp(Height * 0.085f, 56.f, 96.f);
	const float Margin = 26.f;

	// Vida crítica: el líquido late.
	FLinearColor Life = LifeColor;
	const float LifeFraction = Player->GetCombat()->GetLifeFraction();
	if (Body.IsAlive() && LifeFraction < 0.25f)
	{
		Life = Life * (1.f + 0.45f * FMath::Sin(GetWorld()->GetTimeSeconds() * 8.f));
		Life.A = 1.f;
	}
	DrawOrb(Margin + Radius, Height - Margin - Radius, Radius, LifeFraction, Life, LifeFlash, LifeGlow,
		FString::Printf(TEXT("%d / %d"), Body.Life, Body.MaxLife));
	DrawOrb(Width - Margin - Radius, Height - Margin - Radius, Radius, Player->GetCombat()->GetManaFraction(),
		ManaColor, ManaFlash, ManaGlow, FString::Printf(TEXT("%d / %d"), Body.Mana, Body.MaxMana));

	// Barra central.
	const float SlotSize = 64.f;
	const float Gap = 8.f;
	const int32 SlotCount = 5;
	const float BarWidth = SlotCount * SlotSize + (SlotCount + 1) * Gap;
	const float BarX = (Width - BarWidth) * 0.5f;
	const float BarY = Height - SlotSize - 2.f * Gap - 14.f;
	DrawRect(FrameColor, BarX - 3.f, BarY - 17.f, BarWidth + 6.f, SlotSize + 2.f * Gap + 20.f);
	DrawRect(PanelColor, BarX, BarY - 14.f, BarWidth, SlotSize + 2.f * Gap + 14.f);
	AddRect(BarX, BarY - 14.f, BarWidth, SlotSize + 2.f * Gap + 14.f, EUiKind::Panel, 0);

	// Experiencia.
	const arpg::int64 Needed = arpg::ExperienceToNextLevel(Player->State.Progress.Level);
	const float XpFraction = Needed > 0 ? static_cast<float>(Player->State.Progress.Experience) / static_cast<float>(Needed) : 1.f;
	DrawRect(FLinearColor(0.08f, 0.07f, 0.05f), BarX + Gap, BarY - 9.f, BarWidth - 2.f * Gap, 6.f);
	DrawRect(FLinearColor(0.85f, 0.65f, 0.15f), BarX + Gap, BarY - 9.f, (BarWidth - 2.f * Gap) * FMath::Clamp(XpFraction, 0.f, 1.f), 6.f);

	const int32 SkillIndex = Player->GetSkillIndex();
	const arpg::SkillDef* Skill = arpg::GetSkill(SkillIndex);
	const float CooldownFraction = Skill != nullptr && Skill->CooldownMs > 0
		? static_cast<float>(Player->Cooldowns.RemainingMs[SkillIndex]) / static_cast<float>(Skill->CooldownMs) : 0.f;
	const bool bCanAfford = Skill != nullptr && Body.Mana >= Skill->ManaCost;

	struct FSlot { FString Key; FString Name; FLinearColor Color; float Cooldown; bool bEnabled; };
	const FSlot Slots[] = {
		{TEXT("Clic izq."), TEXT("Atacar"), FLinearColor(0.35f, 0.3f, 0.25f), 0.f, true},
		{TEXT("Clic der."), ArpgText::SkillName(SkillIndex), FLinearColor(0.15f, 0.2f, 0.45f), CooldownFraction, bCanAfford},
		{TEXT("1"), FString::Printf(TEXT("Vida x%d"), Player->State.LifePotions), FLinearColor(0.4f, 0.05f, 0.05f), 0.f, Player->State.LifePotions > 0},
		{TEXT("2"), FString::Printf(TEXT("Maná x%d"), Player->State.ManaPotions), FLinearColor(0.05f, 0.1f, 0.4f), 0.f, Player->State.ManaPotions > 0},
		{TEXT("I"), TEXT("Inventario"), FLinearColor(0.25f, 0.2f, 0.12f), 0.f, true},
	};
	for (int32 i = 0; i < SlotCount; ++i)
	{
		const float X = BarX + Gap + static_cast<float>(i) * (SlotSize + Gap);
		const float Y = BarY + Gap;
		DrawRect(FrameColor, X - 2.f, Y - 2.f, SlotSize + 4.f, SlotSize + 4.f);
		DrawRect(Slots[i].bEnabled ? Slots[i].Color : Slots[i].Color * 0.35f, X, Y, SlotSize, SlotSize);
		if (Slots[i].Cooldown > 0.f)
		{
			// El enfriamiento se vacía de arriba abajo.
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), X, Y, SlotSize, SlotSize * FMath::Clamp(Slots[i].Cooldown, 0.f, 1.f));
		}
		Label(Slots[i].Key, X + 3.f, Y + 2.f, DimColor, 0.8f);
		Label(Slots[i].Name, X + SlotSize * 0.5f, Y + SlotSize - 20.f, Slots[i].bEnabled ? TextColor : DimColor, 0.8f, true);
	}
}

// ---------------------------------------------------------------- Parte superior

void AArpgHUD::DrawTopInfo(AArpgPlayerCharacter* Player)
{
	const AArpgGameMode* GameMode = GetWorld()->GetAuthGameMode<AArpgGameMode>();
	const int32 Depth = GameMode != nullptr ? GameMode->GetDepth() : 1;

	DrawRect(PanelColor, 16.f, 16.f, 330.f, 86.f);
	AddRect(16.f, 16.f, 330.f, 86.f, EUiKind::Panel, 0);
	Label(FString::Printf(TEXT("Catacumbas de Mirel — profundidad %d"), Depth), 26.f, 22.f, TextColor, 1.1f);
	Label(TEXT("Objetivo: encuentra y derrota al jefe"), 26.f, 44.f, FLinearColor(0.95f, 0.8f, 0.35f));
	Label(FString::Printf(TEXT("%s nivel %d   Oro: %lld"), *ArpgText::ClassName(Player->State.Progress.Class),
		Player->State.Progress.Level, static_cast<long long>(Player->State.Gold)), 26.f, 64.f, DimColor);
	if (Player->State.Progress.UnspentAttributePoints > 0)
	{
		Label(FString::Printf(TEXT("¡%d puntos de atributo! (I)"), Player->State.Progress.UnspentAttributePoints),
			26.f, 82.f, FLinearColor(0.4f, 1.f, 0.4f));
	}
}

void AArpgHUD::DrawMinimap(AArpgPlayerCharacter* Player)
{
	const AArpgGameMode* GameMode = GetWorld()->GetAuthGameMode<AArpgGameMode>();
	const AArpgDungeonActor* Dungeon = GameMode != nullptr ? GameMode->GetDungeon() : nullptr;
	if (Dungeon == nullptr || bInventoryOpen)
	{
		return;
	}
	const arpg::Dungeon& Map = Dungeon->GetMap();
	const float Size = 190.f;
	const float Pixel = 3.f;
	const int32 Reach = FMath::FloorToInt32(Size / Pixel / 2.f);
	const float X0 = static_cast<float>(Canvas->SizeX) - Size - 16.f;
	const float Y0 = 16.f;

	DrawRect(FrameColor, X0 - 3.f, Y0 - 3.f, Size + 6.f, Size + 6.f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.8f), X0, Y0, Size, Size);

	int32 PlayerX = 0, PlayerY = 0;
	Dungeon->WorldToTile(Player->GetActorLocation(), PlayerX, PlayerY);

	// Un rectángulo por tramo horizontal de suelo, no por casilla.
	const FLinearColor FloorColor(0.42f, 0.38f, 0.3f, 0.9f);
	for (int32 Dy = -Reach; Dy < Reach; ++Dy)
	{
		int32 RunStart = 0;
		bool bInRun = false;
		for (int32 Dx = -Reach; Dx <= Reach; ++Dx)
		{
			const bool bFloor = Dx < Reach && Map.At(PlayerX + Dx, PlayerY + Dy) == arpg::ETile::Floor;
			if (bFloor && !bInRun)
			{
				RunStart = Dx;
				bInRun = true;
			}
			else if (!bFloor && bInRun)
			{
				bInRun = false;
				DrawRect(FloorColor, X0 + static_cast<float>(RunStart + Reach) * Pixel,
					Y0 + static_cast<float>(Dy + Reach) * Pixel, static_cast<float>(Dx - RunStart) * Pixel, Pixel);
			}
		}
	}

	const auto Dot = [&](const FVector& Location, const FLinearColor& Color, float DotSize)
	{
		int32 Tx = 0, Ty = 0;
		Dungeon->WorldToTile(Location, Tx, Ty);
		const int32 Dx = Tx - PlayerX, Dy = Ty - PlayerY;
		if (Dx >= -Reach && Dx < Reach && Dy >= -Reach && Dy < Reach)
		{
			DrawRect(Color, X0 + static_cast<float>(Dx + Reach) * Pixel - DotSize * 0.5f,
				Y0 + static_cast<float>(Dy + Reach) * Pixel - DotSize * 0.5f, DotSize, DotSize);
		}
	};
	for (TActorIterator<AArpgMonster> It(GetWorld()); It; ++It)
	{
		if (It->GetCombat()->IsAlive())
		{
			const bool bBoss = It->GetInstance().Rank == arpg::EMonsterRank::Boss;
			Dot(It->GetActorLocation(), bBoss ? FLinearColor(1.f, 0.5f, 0.f) : FLinearColor(0.9f, 0.1f, 0.1f), bBoss ? 8.f : 4.f);
		}
	}
	Dot(Player->GetActorLocation(), FLinearColor(0.3f, 1.f, 0.3f), 6.f);
}

void AArpgHUD::DrawWorldLabels(AArpgPlayerCharacter* Player)
{
	const AArpgPlayerController* PlayerController = Cast<AArpgPlayerController>(GetOwningPlayerController());
	const AActor* Hovered = PlayerController != nullptr ? PlayerController->GetHoveredActor() : nullptr;

	// Nombres del botín en el suelo.
	for (TActorIterator<AArpgLootPickup> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist2D(It->GetActorLocation(), Player->GetActorLocation()) > 2200.f)
		{
			continue;
		}
		const FVector Screen = Project(It->GetActorLocation() + FVector(0.f, 0.f, 60.f));
		if (Screen.Z > 0.f)
		{
			const bool bHovered = *It == Hovered;
			FLinearColor Color = It->GetLabelColor();
			Color.A = bHovered ? 1.f : 0.85f;
			Label(It->GetLabel(), Screen.X, Screen.Y - 10.f, Color, bHovered ? 1.15f : 0.95f, true);
		}
	}

	// Vida y nombre del monstruo señalado.
	if (const AArpgMonster* Monster = Cast<AArpgMonster>(Hovered))
	{
		const float Width = static_cast<float>(Canvas->SizeX);
		const float BarWidth = 360.f;
		const float X = (Width - BarWidth) * 0.5f;
		DrawRect(FrameColor, X - 2.f, 20.f, BarWidth + 4.f, 26.f);
		DrawRect(FLinearColor(0.05f, 0.f, 0.f), X, 22.f, BarWidth, 22.f);
		DrawRect(LifeColor, X, 22.f, BarWidth * Monster->GetCombat()->GetLifeFraction(), 22.f);
		Label(FString::Printf(TEXT("%s  —  nivel %d"), *Monster->GetDisplayName(), Monster->GetInstance().Level),
			Width * 0.5f, 25.f, FLinearColor::White, 1.f, true);
	}

	// Números de daño.
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	for (int32 i = FloatingTexts.Num() - 1; i >= 0; --i)
	{
		FFloatingText& Entry = FloatingTexts[i];
		Entry.Age += DeltaSeconds;
		if (Entry.Age > 0.9f)
		{
			FloatingTexts.RemoveAtSwap(i);
			continue;
		}
		const FVector Screen = Project(Entry.WorldLocation + FVector(0.f, 0.f, Entry.Age * 120.f));
		if (Screen.Z > 0.f)
		{
			FLinearColor Color = Entry.Color;
			Color.A = 1.f - Entry.Age / 0.9f;
			Label(Entry.Text, Screen.X, Screen.Y, Color, 1.3f, true);
		}
	}
}

// ---------------------------------------------------------------- Inventario

float AArpgHUD::DrawItemLines(const arpg::ItemInstance& Item, float X, float Y, bool bMeasureOnly, float& OutWidth)
{
	const arpg::ItemBase* Base = arpg::GetItemBase(Item.BaseIndex);
	if (Base == nullptr)
	{
		return Y;
	}
	const float LineHeight = 17.f;
	const auto Line = [&](const FString& Text, const FLinearColor& Color)
	{
		float W = 0.f, H = 0.f;
		GetTextSize(Text, W, H, Font, 1.f);
		OutWidth = FMath::Max(OutWidth, W);
		if (!bMeasureOnly)
		{
			Label(Text, X, Y, Color);
		}
		Y += LineHeight;
	};

	Line(ArpgText::ItemName(Item), ArpgText::RarityColor(Item.Rarity));
	Line(FString::Printf(TEXT("Nivel de objeto %d"), Item.ItemLevel), DimColor);
	if (Base->Damage.Max > 0) { Line(FString::Printf(TEXT("Daño %d-%d"), Base->Damage.Min, Base->Damage.Max), TextColor); }
	if (Base->Armor > 0) { Line(FString::Printf(TEXT("Armadura %d"), Base->Armor), TextColor); }
	if (Base->BlockPercent > 0) { Line(FString::Printf(TEXT("Bloqueo %d%%"), Base->BlockPercent), TextColor); }
	for (int32 i = 0; i < Item.AffixCount && i < arpg::MaxAffixes; ++i)
	{
		Line(ArpgText::AffixLine(Item.Affixes[i]), FLinearColor(0.5f, 0.65f, 1.f));
	}
	if (Base->RequiredLevel > 1) { Line(FString::Printf(TEXT("Requiere nivel %d"), Base->RequiredLevel), DimColor); }
	if (Base->RequiredStrength > 0) { Line(FString::Printf(TEXT("Requiere %d de Fuerza"), Base->RequiredStrength), DimColor); }
	Line(FString::Printf(TEXT("Venta: %d de oro"), arpg::ComputeSellValue(Item)), FLinearColor(1.f, 0.8f, 0.2f));
	return Y;
}

void AArpgHUD::DrawItemTooltip(const arpg::ItemInstance& Item, const arpg::ItemInstance* Equipped, float X, float Y)
{
	// Primera pasada: medir. Segunda: dibujar sobre un fondo del tamaño justo.
	float TextWidth = 0.f;
	float Bottom = DrawItemLines(Item, 0.f, 0.f, true, TextWidth);
	if (Equipped != nullptr)
	{
		Bottom = DrawItemLines(*Equipped, 0.f, Bottom + 24.f, true, TextWidth);
	}
	const float BoxWidth = TextWidth + 20.f;
	const float BoxHeight = Bottom + 16.f;
	X = FMath::Clamp(X - BoxWidth - 12.f, 4.f, static_cast<float>(Canvas->SizeX) - BoxWidth - 4.f);
	Y = FMath::Clamp(Y, 4.f, static_cast<float>(Canvas->SizeY) - BoxHeight - 4.f);

	DrawRect(FrameColor, X - 2.f, Y - 2.f, BoxWidth + 4.f, BoxHeight + 4.f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.95f), X, Y, BoxWidth, BoxHeight);
	float Cursor = DrawItemLines(Item, X + 10.f, Y + 8.f, false, TextWidth);
	if (Equipped != nullptr)
	{
		Label(TEXT("— Equipado ahora —"), X + 10.f, Cursor + 4.f, DimColor);
		DrawItemLines(*Equipped, X + 10.f, Cursor + 24.f, false, TextWidth);
	}
}

void AArpgHUD::DrawInventory(AArpgPlayerCharacter* Player)
{
	const arpg::CharacterSave& State = Player->State;
	const arpg::EquippedStats& Stats = Player->Stats;
	const float PanelWidth = 400.f;
	const float PanelHeight = 600.f;
	const float X0 = static_cast<float>(Canvas->SizeX) - PanelWidth - 16.f;
	const float Y0 = 16.f;
	const float Left = X0 + 20.f;

	DrawRect(FrameColor, X0 - 3.f, Y0 - 3.f, PanelWidth + 6.f, PanelHeight + 6.f);
	DrawRect(PanelColor, X0, Y0, PanelWidth, PanelHeight);
	AddRect(X0, Y0, PanelWidth, PanelHeight, EUiKind::Panel, 0);

	float Y = Y0 + 12.f;
	Label(FString::Printf(TEXT("%s — nivel %d"), *ArpgText::ClassName(State.Progress.Class), State.Progress.Level),
		Left, Y, TextColor, 1.3f);
	Y += 30.f;

	// Atributos, con botón [+] si hay puntos libres.
	const TCHAR* const AttributeNames[] = {TEXT("Fuerza"), TEXT("Destreza"), TEXT("Vitalidad"), TEXT("Energía")};
	const int32 AttributeValues[] = {Stats.Attr.Strength, Stats.Attr.Dexterity, Stats.Attr.Vitality, Stats.Attr.Energy};
	for (int32 i = 0; i < 4; ++i)
	{
		Label(FString::Printf(TEXT("%s: %d"), AttributeNames[i], AttributeValues[i]), Left, Y, TextColor);
		if (State.Progress.UnspentAttributePoints > 0)
		{
			DrawRect(FLinearColor(0.15f, 0.45f, 0.15f), Left + 150.f, Y - 1.f, 20.f, 18.f);
			Label(TEXT("+"), Left + 160.f, Y - 1.f, FLinearColor::White, 1.f, true);
			AddRect(Left + 150.f, Y - 1.f, 20.f, 18.f, EUiKind::AttributePlus, i);
		}
		Y += 19.f;
	}
	const float StatsX = Left + 190.f;
	float StatsY = Y0 + 42.f;
	Label(FString::Printf(TEXT("Daño: %d-%d"), Stats.WeaponDamage.Min, Stats.WeaponDamage.Max), StatsX, StatsY, TextColor); StatsY += 19.f;
	Label(FString::Printf(TEXT("Índ. ataque: %d"), Stats.Derived.AttackRating), StatsX, StatsY, TextColor); StatsY += 19.f;
	Label(FString::Printf(TEXT("Defensa: %d   Armadura: %d"), Stats.Derived.Defense, Stats.Armor), StatsX, StatsY, TextColor); StatsY += 19.f;
	Label(FString::Printf(TEXT("Puntos libres: %d"), State.Progress.UnspentAttributePoints), StatsX, StatsY,
		State.Progress.UnspentAttributePoints > 0 ? FLinearColor(0.4f, 1.f, 0.4f) : DimColor);
	Y += 10.f;

	float MouseX = 0.f, MouseY = 0.f;
	const bool bHasMouse = GetOwningPlayerController() != nullptr && GetOwningPlayerController()->GetMousePosition(MouseX, MouseY);
	const FVector2D Mouse(MouseX, MouseY);
	const arpg::ItemInstance* TooltipItem = nullptr;
	const arpg::ItemInstance* TooltipCompare = nullptr;

	// Equipo: clic para quitar.
	Label(TEXT("Equipo (clic para quitar)"), Left, Y, DimColor);
	Y += 20.f;
	for (int32 i = 0; i < arpg::EquipSlotCount; ++i)
	{
		const arpg::EEquipSlot Slot = static_cast<arpg::EEquipSlot>(i);
		const arpg::ItemInstance* Item = State.Gear.Get(Slot);
		const FString Name = Item != nullptr ? ArpgText::ItemName(*Item) : FString(TEXT("—"));
		Label(ArpgText::SlotName(Slot) + TEXT(":"), Left, Y, DimColor);
		Label(Name, Left + 90.f, Y, Item != nullptr ? ArpgText::RarityColor(Item->Rarity) : DimColor);
		if (Item != nullptr)
		{
			AddRect(Left, Y, PanelWidth - 40.f, 18.f, EUiKind::EquipSlot, i);
			if (bHasMouse && Rects.Last().Box.IsInside(Mouse))
			{
				TooltipItem = Item;
			}
		}
		Y += 19.f;
	}
	Y += 12.f;

	// Mochila: clic equipa, clic derecho suelta.
	Label(TEXT("Mochila (clic: equipar · clic der.: soltar)"), Left, Y, DimColor);
	Y += 20.f;
	const float GridX = Left;
	const float GridY = Y;
	const float GridWidth = static_cast<float>(State.Bag.GetWidth()) * CellSize;
	const float GridHeight = static_cast<float>(State.Bag.GetHeight()) * CellSize;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), GridX, GridY, GridWidth, GridHeight);
	for (int32 Column = 0; Column <= State.Bag.GetWidth(); ++Column)
	{
		const float LineX = GridX + static_cast<float>(Column) * CellSize;
		DrawLine(LineX, GridY, LineX, GridY + GridHeight, FrameColor, 1.f);
	}
	for (int32 Row = 0; Row <= State.Bag.GetHeight(); ++Row)
	{
		const float LineY = GridY + static_cast<float>(Row) * CellSize;
		DrawLine(GridX, LineY, GridX + GridWidth, LineY, FrameColor, 1.f);
	}
	for (const arpg::GridInventory::Entry& Entry : State.Bag.GetEntries())
	{
		const float ItemX = GridX + static_cast<float>(Entry.X) * CellSize + 2.f;
		const float ItemY = GridY + static_cast<float>(Entry.Y) * CellSize + 2.f;
		const float ItemW = static_cast<float>(Entry.Width) * CellSize - 4.f;
		const float ItemH = static_cast<float>(Entry.Height) * CellSize - 4.f;
		FLinearColor Fill = ArpgText::RarityColor(Entry.Item.Rarity) * 0.35f;
		Fill.A = 1.f;
		DrawRect(Fill, ItemX, ItemY, ItemW, ItemH);
		Label(ArpgText::ItemName(Entry.Item).Left(3), ItemX + ItemW * 0.5f, ItemY + ItemH * 0.5f - 8.f, FLinearColor::White, 0.85f, true);
		AddRect(ItemX, ItemY, ItemW, ItemH, EUiKind::BagItem, Entry.Id);
		if (bHasMouse && Rects.Last().Box.IsInside(Mouse))
		{
			TooltipItem = &Entry.Item;
			// Comparación: lo que lleva puesto en la ranura de ese tipo.
			const arpg::ItemBase* Base = arpg::GetItemBase(Entry.Item.BaseIndex);
			for (int32 i = 0; Base != nullptr && i < arpg::EquipSlotCount && TooltipCompare == nullptr; ++i)
			{
				if (arpg::GetSlotItemType(static_cast<arpg::EEquipSlot>(i)) == Base->Type)
				{
					TooltipCompare = State.Gear.Get(static_cast<arpg::EEquipSlot>(i));
				}
			}
		}
	}
	Label(FString::Printf(TEXT("Oro: %lld"), static_cast<long long>(State.Gold)), Left, GridY + GridHeight + 10.f,
		FLinearColor(1.f, 0.8f, 0.2f));

	if (TooltipItem != nullptr)
	{
		DrawItemTooltip(*TooltipItem, TooltipCompare, MouseX, MouseY);
	}
}
