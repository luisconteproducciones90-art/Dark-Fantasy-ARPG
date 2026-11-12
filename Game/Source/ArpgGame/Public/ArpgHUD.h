#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArpgCore/Combat/Combatant.h"
#include "ArpgCore/Item/Item.h"
#include "ArpgHUD.generated.h"

class AArpgPlayerCharacter;
class UFont;

/**
 * Interfaz de juego dibujada por código (sin assets):
 *  - esfera roja de vida abajo a la izquierda y azul de maná abajo a la derecha;
 *  - barra central con ataque, habilidad, pociones y experiencia;
 *  - minimapa, zona y objetivo arriba;
 *  - inventario por cuadrícula con equipo, atributos y comparación de objetos.
 * Es funcional, no definitiva: el acabado ornamentado llegará en UMG con arte propio.
 */
UCLASS()
class ARPGGAME_API AArpgHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	void ShowMessage(const FString& Text);
	/** Número flotante sobre un golpe (o "Fallo" / "Bloqueado"). */
	void ShowHit(const FVector& WorldLocation, const arpg::AttackOutcome& Outcome);
	void ToggleInventory() { bInventoryOpen = !bInventoryOpen; }

	/** true si el clic cayó sobre la interfaz (y ya se atendió): el mundo debe ignorarlo. */
	bool HandleClick(const FVector2D& Position, bool bRightButton);

private:
	enum class EUiKind : uint8 { Panel, BagItem, EquipSlot, AttributePlus };

	struct FUiRect
	{
		FBox2D Box;
		EUiKind Kind;
		int32 Id;
	};

	struct FFloatingText
	{
		FVector WorldLocation;
		FString Text;
		FLinearColor Color;
		float Age;
	};

	void Label(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCentered = false);
	void DrawOrb(float CenterX, float CenterY, float Radius, float Fraction, const FLinearColor& Liquid,
		float Flash, float Glow, const FString& Text);
	void DrawBottomBar(AArpgPlayerCharacter* Player);
	void DrawTopInfo(AArpgPlayerCharacter* Player);
	void DrawMinimap(AArpgPlayerCharacter* Player);
	void DrawWorldLabels(AArpgPlayerCharacter* Player);
	void DrawInventory(AArpgPlayerCharacter* Player);
	void DrawItemTooltip(const arpg::ItemInstance& Item, const arpg::ItemInstance* Equipped, float X, float Y);
	float DrawItemLines(const arpg::ItemInstance& Item, float X, float Y, bool bMeasureOnly, float& OutWidth);
	void AddRect(float X, float Y, float W, float H, EUiKind Kind, int32 Id);

	UPROPERTY()
	TObjectPtr<UFont> Font;

	TArray<FUiRect> Rects;
	TArray<FFloatingText> FloatingTexts;
	FString Message;
	float MessageTimeLeft = 0.f;
	float LifeFlash = 0.f;
	float LifeGlow = 0.f;
	float ManaFlash = 0.f;
	float ManaGlow = 0.f;
	int32 PreviousLife = -1;
	int32 PreviousMana = -1;
	bool bInventoryOpen = false;
};
