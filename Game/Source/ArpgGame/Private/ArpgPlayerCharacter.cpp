#include "ArpgPlayerCharacter.h"

#include "ArpgCombatComponent.h"
#include "ArpgGameMode.h"
#include "ArpgHUD.h"
#include "ArpgLootPickup.h"
#include "ArpgText.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AArpgPlayerCharacter::AArpgPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// El personaje mira hacia donde camina; el mando no lo rota.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 720.f, 0.f);
	Movement->MaxWalkSpeed = BaseWalkSpeed;
	// Arranque y frenada casi instantáneos: movimiento directo y preciso.
	Movement->MaxAcceleration = 8192.f;
	Movement->BrakingDecelerationWalking = 8192.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // no hereda la rotación del personaje
	CameraBoom->bDoCollisionTest = false;       // los muros se ocultan por material, no acercando la cámara
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	IsoCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("IsoCamera"));
	IsoCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	IsoCamera->bUsePawnControlRotation = false;

	Combat = CreateDefaultSubobject<UArpgCombatComponent>(TEXT("Combat"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetStaticMesh(CylinderMesh.Object);
	BodyMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.8f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // los clics lo atraviesan

	Torch = CreateDefaultSubobject<UPointLightComponent>(TEXT("Torch"));
	Torch->SetupAttachment(RootComponent);
	Torch->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	Torch->SetLightColor(FLinearColor(1.f, 0.72f, 0.42f));
	Torch->SetIntensity(9000.f);
	Torch->SetAttenuationRadius(1700.f);
	Torch->SetCastShadows(true);

	State.Progress = arpg::CreateCharacter(arpg::ECharacterClass::Warrior);
}

void AArpgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	TargetZoomDistance = FMath::Clamp(DefaultZoomDistance, MinZoomDistance, MaxZoomDistance);
	CameraBoom->TargetArmLength = TargetZoomDistance;
	CameraBoom->SetWorldRotation(FRotator(-CameraPitch, CameraBaseYaw, 0.f));
	IsoCamera->SetFieldOfView(CameraFieldOfView);

	Combat->OnDied.AddUObject(this, &AArpgPlayerCharacter::HandleDied);
}

void AArpgPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength, TargetZoomDistance, DeltaSeconds, ZoomInterpSpeed);

	Cooldowns.Tick(FMath::RoundToInt32(DeltaSeconds * 1000.f));

	if (!Combat->IsAlive())
	{
		return;
	}
	const int32 SpeedPercent = Combat->Body.Statuses.MoveSpeedPercent();
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * static_cast<float>(SpeedPercent) / 100.f;

	while (Path.Num() > 0 && FVector::Dist2D(Path[0], GetActorLocation()) < 35.f)
	{
		Path.RemoveAt(0);
	}
	if (Path.Num() > 0)
	{
		AddMovementInput((Path[0] - GetActorLocation()).GetSafeNormal2D());
	}
}

// ---------------------------------------------------------------- Cámara

void AArpgPlayerCharacter::AddZoomInput(float Delta)
{
	TargetZoomDistance = FMath::Clamp(TargetZoomDistance - Delta * ZoomStep, MinZoomDistance, MaxZoomDistance);
}

void AArpgPlayerCharacter::AddCameraYawInput(float DeltaDegrees)
{
	if (!bAllowCameraRotation)
	{
		return;
	}
	YawOffset = FMath::Clamp(YawOffset + DeltaDegrees, -MaxYawOffset, MaxYawOffset);
	CameraBoom->SetWorldRotation(FRotator(-CameraPitch, CameraBaseYaw + YawOffset, 0.f));
}

// ---------------------------------------------------------------- Movimiento

void AArpgPlayerCharacter::FollowPath(const TArray<FVector>& Points)
{
	Path = Points;
}

void AArpgPlayerCharacter::StopMoving()
{
	Path.Reset();
}

float AArpgPlayerCharacter::GetBasicAttackRange() const
{
	switch (State.Progress.Class)
	{
	case arpg::ECharacterClass::Hunter:      return 1200.f;
	case arpg::ECharacterClass::Sorcerer:
	case arpg::ECharacterClass::Necromancer: return 900.f;
	default:                                 return 210.f;
	}
}

// ---------------------------------------------------------------- Estado de juego

void AArpgPlayerCharacter::ResetCharacter(arpg::ECharacterClass Class)
{
	State = arpg::CharacterSave{};
	State.Progress = arpg::CreateCharacter(Class);
	State.LifePotions = 3;
	State.ManaPotions = 2;
	State.WorldSeed = static_cast<arpg::uint64>(ArpgGlobalRng().NextU32()) + 1;
	Cooldowns = arpg::SkillCooldowns{};

	// Equipo inicial: un arma acorde a la clase.
	const char* StartingWeapon = "short_sword";
	if (Class == arpg::ECharacterClass::Hunter) { StartingWeapon = "hunting_bow"; }
	if (Class == arpg::ECharacterClass::Sorcerer || Class == arpg::ECharacterClass::Necromancer) { StartingWeapon = "ash_staff"; }
	const arpg::ItemInstance Weapon =
		arpg::GenerateItem(arpg::FindItemBase(StartingWeapon), 1, arpg::ERarity::Common, ArpgGlobalRng());
	arpg::ItemInstance Previous;
	bool bHadPrevious = false;
	State.Gear.Equip(State.Progress, Weapon, arpg::EEquipSlot::MainHand, Previous, bHadPrevious);

	RefreshStats(true);
}

void AArpgPlayerCharacter::ApplyLoadedState(const arpg::CharacterSave& Loaded)
{
	State = Loaded;
	Cooldowns = arpg::SkillCooldowns{};
	RefreshStats(true);
}

void AArpgPlayerCharacter::RefreshStats(bool bRefill)
{
	Stats = arpg::ComputeEquippedStats(State.Progress, State.Gear);

	if (bRefill)
	{
		arpg::InitCombatant(Combat->Body, Stats.Derived.MaxLife, Stats.Derived.MaxMana);
	}
	else
	{
		arpg::SetMaxValues(Combat->Body, Stats.Derived.MaxLife, Stats.Derived.MaxMana);
	}
	// Regeneración base: 0,6 % de la vida y 1,5 % del maná por segundo.
	Combat->Body.LifeRegenMilliPerSecond = Stats.Derived.MaxLife * 6;
	Combat->Body.ManaRegenMilliPerSecond = Stats.Derived.MaxMana * 15;
	Combat->Attack = arpg::MakeAttackProfile(State.Progress, Stats);
	Combat->Defense = arpg::MakeDefenseProfile(State.Progress, Stats);

	if (BodyMesh != nullptr)
	{
		static const FLinearColor ClassColors[] = {
			FLinearColor(0.55f, 0.25f, 0.15f), FLinearColor(0.2f, 0.35f, 0.8f), FLinearColor(0.2f, 0.5f, 0.2f),
			FLinearColor(0.35f, 0.15f, 0.45f), FLinearColor(0.15f, 0.15f, 0.18f), FLinearColor(0.8f, 0.7f, 0.35f)};
		const int32 ClassIndex = FMath::Clamp(static_cast<int32>(State.Progress.Class), 0, 5);
		if (UMaterialInstanceDynamic* Material = BodyMesh->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), ClassColors[ClassIndex]);
		}
	}
}

void AArpgPlayerCharacter::GainExperience(int64 Amount)
{
	const int32 LevelsGained = arpg::AddExperience(State.Progress, Amount);
	if (LevelsGained > 0)
	{
		RefreshStats(true); // subir de nivel cura por completo
		Announce(FString::Printf(TEXT("¡Nivel %d!"), State.Progress.Level));
	}
}

bool AArpgPlayerCharacter::SpendAttributePoint(arpg::EAttribute Attribute)
{
	if (!arpg::SpendAttributePoints(State.Progress, Attribute, 1))
	{
		return false;
	}
	RefreshStats(false);
	return true;
}

bool AArpgPlayerCharacter::UsePotion(bool bMana)
{
	if (!Combat->IsAlive())
	{
		return false;
	}
	int32& Count = bMana ? State.ManaPotions : State.LifePotions;
	if (Count <= 0)
	{
		return false;
	}
	// Restaura el 40 % del máximo. Si ya está lleno no se gasta.
	const int32 Restored = bMana
		? arpg::RestoreMana(Combat->Body, Combat->Body.MaxMana * 2 / 5)
		: arpg::Heal(Combat->Body, Combat->Body.MaxLife * 2 / 5);
	if (Restored <= 0)
	{
		return false;
	}
	--Count;
	return true;
}

bool AArpgPlayerCharacter::TryPickup(AArpgLootPickup* Pickup)
{
	if (Pickup == nullptr || !Combat->IsAlive())
	{
		return false;
	}
	switch (Pickup->Kind)
	{
	case EArpgPickupKind::Gold:
		State.Gold += Pickup->GoldAmount;
		break;

	case EArpgPickupKind::LifePotion:
	case EArpgPickupKind::ManaPotion:
	{
		int32& Count = Pickup->Kind == EArpgPickupKind::ManaPotion ? State.ManaPotions : State.LifePotions;
		if (Count >= arpg::MaxPotions)
		{
			return false;
		}
		++Count;
		break;
	}

	default:
		if (State.Bag.AutoPlace(Pickup->Item) == 0)
		{
			Announce(TEXT("Inventario lleno"));
			return false;
		}
		break;
	}
	Pickup->Destroy();
	return true;
}

bool AArpgPlayerCharacter::EquipFromBag(int32 EntryId)
{
	const arpg::GridInventory::Entry* Entry = State.Bag.Find(EntryId);
	if (Entry == nullptr)
	{
		return false;
	}
	const arpg::ItemInstance Item = Entry->Item;
	const arpg::ItemBase* Base = arpg::GetItemBase(Item.BaseIndex);
	if (Base == nullptr)
	{
		return false;
	}

	// Ranura según el tipo; un anillo va a la primera ranura de anillo libre.
	arpg::EEquipSlot Slot = arpg::EEquipSlot::Count;
	for (int32 i = 0; i < arpg::EquipSlotCount; ++i)
	{
		const arpg::EEquipSlot Candidate = static_cast<arpg::EEquipSlot>(i);
		if (arpg::GetSlotItemType(Candidate) == Base->Type)
		{
			if (Slot == arpg::EEquipSlot::Count || State.Gear.Get(Candidate) == nullptr)
			{
				Slot = Candidate;
			}
			if (State.Gear.Get(Candidate) == nullptr)
			{
				break;
			}
		}
	}

	switch (State.Gear.CanEquip(State.Progress, Item, Slot))
	{
	case arpg::EEquipResult::Ok: break;
	case arpg::EEquipResult::LevelTooLow:
		Announce(FString::Printf(TEXT("Requiere nivel %d"), Base->RequiredLevel));
		return false;
	case arpg::EEquipResult::StrengthTooLow:
		Announce(FString::Printf(TEXT("Requiere %d de Fuerza"), Base->RequiredStrength));
		return false;
	default:
		return false;
	}

	State.Bag.Remove(EntryId);
	arpg::ItemInstance Previous;
	bool bHadPrevious = false;
	State.Gear.Equip(State.Progress, Item, Slot, Previous, bHadPrevious);
	if (bHadPrevious && State.Bag.AutoPlace(Previous) == 0)
	{
		DropItemAtFeet(Previous); // no cabe en la mochila: al suelo, nunca se pierde
	}
	RefreshStats(false);
	return true;
}

bool AArpgPlayerCharacter::UnequipToBag(arpg::EEquipSlot Slot)
{
	const arpg::ItemInstance* Equipped = State.Gear.Get(Slot);
	if (Equipped == nullptr)
	{
		return false;
	}
	if (State.Bag.AutoPlace(*Equipped) == 0)
	{
		Announce(TEXT("Inventario lleno"));
		return false;
	}
	State.Gear.Unequip(Slot);
	RefreshStats(false);
	return true;
}

bool AArpgPlayerCharacter::DropFromBag(int32 EntryId)
{
	arpg::ItemInstance Item;
	if (!State.Bag.Remove(EntryId, &Item))
	{
		return false;
	}
	DropItemAtFeet(Item);
	return true;
}

void AArpgPlayerCharacter::DropItemAtFeet(const arpg::ItemInstance& Item) const
{
	if (AArpgGameMode* GameMode = GetWorld()->GetAuthGameMode<AArpgGameMode>())
	{
		GameMode->SpawnItemPickup(Item, GetActorLocation());
	}
}

void AArpgPlayerCharacter::Respawn(const FVector& Location)
{
	StopMoving();
	SetActorLocation(Location + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	RefreshStats(true);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void AArpgPlayerCharacter::HandleDied()
{
	StopMoving();
	GetCharacterMovement()->StopMovementImmediately();
	if (AArpgGameMode* GameMode = GetWorld()->GetAuthGameMode<AArpgGameMode>())
	{
		GameMode->NotifyPlayerDied(this);
	}
}

void AArpgPlayerCharacter::Announce(const FString& Text) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (AArpgHUD* Hud = PlayerController != nullptr ? Cast<AArpgHUD>(PlayerController->GetHUD()) : nullptr)
	{
		Hud->ShowMessage(Text);
	}
}
