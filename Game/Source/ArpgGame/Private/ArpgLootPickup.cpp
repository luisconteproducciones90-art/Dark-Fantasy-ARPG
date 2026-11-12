#include "ArpgLootPickup.h"

#include "ArpgText.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AArpgLootPickup::AArpgLootPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetStaticMesh(CubeMesh.Object);
	Mesh->SetWorldScale3D(FVector(0.35f));
	// Solo responde al cursor: no estorba al caminar.
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AArpgLootPickup::InitAsItem(const arpg::ItemInstance& InItem)
{
	Kind = EArpgPickupKind::Item;
	Item = InItem;
	ApplyColor(GetLabelColor());
}

void AArpgLootPickup::InitAsGold(int32 Amount)
{
	Kind = EArpgPickupKind::Gold;
	GoldAmount = FMath::Max(Amount, 1);
	Mesh->SetWorldScale3D(FVector(0.2f));
	ApplyColor(GetLabelColor());
}

void AArpgLootPickup::InitAsPotion(bool bMana)
{
	Kind = bMana ? EArpgPickupKind::ManaPotion : EArpgPickupKind::LifePotion;
	Mesh->SetWorldScale3D(FVector(0.2f, 0.2f, 0.35f));
	ApplyColor(GetLabelColor());
}

FString AArpgLootPickup::GetLabel() const
{
	switch (Kind)
	{
	case EArpgPickupKind::Item:       return ArpgText::ItemName(Item);
	case EArpgPickupKind::Gold:       return FString::Printf(TEXT("%d de oro"), GoldAmount);
	case EArpgPickupKind::LifePotion: return TEXT("Poción de vida");
	default:                          return TEXT("Poción de maná");
	}
}

FLinearColor AArpgLootPickup::GetLabelColor() const
{
	switch (Kind)
	{
	case EArpgPickupKind::Item:       return ArpgText::RarityColor(Item.Rarity);
	case EArpgPickupKind::Gold:       return FLinearColor(1.f, 0.8f, 0.2f);
	case EArpgPickupKind::LifePotion: return FLinearColor(0.9f, 0.1f, 0.1f);
	default:                          return FLinearColor(0.15f, 0.3f, 1.f);
	}
}

void AArpgLootPickup::ApplyColor(const FLinearColor& Color)
{
	if (UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}
