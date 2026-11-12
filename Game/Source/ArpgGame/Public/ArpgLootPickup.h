#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArpgCore/Item/Item.h"
#include "ArpgLootPickup.generated.h"

class UStaticMeshComponent;

UENUM()
enum class EArpgPickupKind : uint8
{
	Item,
	Gold,
	LifePotion,
	ManaPotion
};

/** Botín en el suelo. Se recoge con clic. */
UCLASS()
class ARPGGAME_API AArpgLootPickup : public AActor
{
	GENERATED_BODY()

public:
	AArpgLootPickup();

	void InitAsItem(const arpg::ItemInstance& InItem);
	void InitAsGold(int32 Amount);
	void InitAsPotion(bool bMana);

	FString GetLabel() const;
	FLinearColor GetLabelColor() const;

	EArpgPickupKind Kind = EArpgPickupKind::Gold;
	arpg::ItemInstance Item;
	int32 GoldAmount = 0;

private:
	void ApplyColor(const FLinearColor& Color);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;
};
