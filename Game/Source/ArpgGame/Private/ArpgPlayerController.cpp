#include "ArpgPlayerController.h"

#include "ArpgCombatComponent.h"
#include "ArpgDungeonActor.h"
#include "ArpgGameMode.h"
#include "ArpgHUD.h"
#include "ArpgLootPickup.h"
#include "ArpgMonster.h"
#include "ArpgPlayerCharacter.h"
#include "ArpgText.h"
#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

AArpgPlayerController::AArpgPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	const auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = CreateDefaultSubobject<UInputAction>(Name);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Boolean);
	SkillAction = MakeAction(TEXT("IA_Skill"), EInputActionValueType::Boolean);
	LifePotionAction = MakeAction(TEXT("IA_LifePotion"), EInputActionValueType::Boolean);
	ManaPotionAction = MakeAction(TEXT("IA_ManaPotion"), EInputActionValueType::Boolean);
	InventoryAction = MakeAction(TEXT("IA_Inventory"), EInputActionValueType::Boolean);
	ZoomAction = MakeAction(TEXT("IA_Zoom"), EInputActionValueType::Axis1D);
	RotateCameraAction = MakeAction(TEXT("IA_RotateCamera"), EInputActionValueType::Axis1D);

	MappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("IMC_Default"));
}

void AArpgPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	MappingContext->MapKey(MoveAction, EKeys::LeftMouseButton);
	MappingContext->MapKey(SkillAction, EKeys::RightMouseButton);
	MappingContext->MapKey(LifePotionAction, EKeys::One);
	MappingContext->MapKey(ManaPotionAction, EKeys::Two);
	MappingContext->MapKey(InventoryAction, EKeys::I);
	MappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
	MappingContext->MapKey(RotateCameraAction, EKeys::E);
	FEnhancedActionKeyMapping& RotateLeft = MappingContext->MapKey(RotateCameraAction, EKeys::Q);
	RotateLeft.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));

	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void AArpgPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Input->BindAction(MoveAction, ETriggerEvent::Started, this, &AArpgPlayerController::OnMoveStarted);
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArpgPlayerController::OnMoveHeld);
		Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AArpgPlayerController::OnMoveReleased);
		Input->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AArpgPlayerController::OnMoveReleased);
		Input->BindAction(SkillAction, ETriggerEvent::Started, this, &AArpgPlayerController::OnSkill);
		Input->BindAction(LifePotionAction, ETriggerEvent::Started, this, &AArpgPlayerController::OnLifePotion);
		Input->BindAction(ManaPotionAction, ETriggerEvent::Started, this, &AArpgPlayerController::OnManaPotion);
		Input->BindAction(InventoryAction, ETriggerEvent::Started, this, &AArpgPlayerController::OnToggleInventory);
		Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AArpgPlayerController::OnZoom);
		Input->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &AArpgPlayerController::OnRotateCamera);
	}
}

AArpgPlayerCharacter* AArpgPlayerController::GetArpgCharacter() const
{
	return Cast<AArpgPlayerCharacter>(GetPawn());
}

bool AArpgPlayerController::GetGroundPointUnderCursor(FVector& OutLocation) const
{
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/ false, Hit))
	{
		OutLocation = Hit.Location;
		return true;
	}
	return false;
}

bool AArpgPlayerController::ClickHitsInterface(bool bRightButton) const
{
	float MouseX = 0.f, MouseY = 0.f;
	AArpgHUD* Hud = Cast<AArpgHUD>(GetHUD());
	return Hud != nullptr && GetMousePosition(MouseX, MouseY) && Hud->HandleClick(FVector2D(MouseX, MouseY), bRightButton);
}

void AArpgPlayerController::MoveCharacterTo(const FVector& Destination)
{
	AArpgPlayerCharacter* Hero = GetArpgCharacter();
	if (Hero == nullptr)
	{
		return;
	}
	TArray<FVector> Points;
	const AArpgGameMode* GameMode = GetWorld()->GetAuthGameMode<AArpgGameMode>();
	const AArpgDungeonActor* Dungeon = GameMode != nullptr ? GameMode->GetDungeon() : nullptr;
	if (Dungeon == nullptr || !Dungeon->FindWorldPath(Hero->GetActorLocation(), Destination, Points))
	{
		// Destino no transitable (p. ej. clic sobre un muro): camina hacia allí hasta donde se pueda.
		Points.Reset();
		Points.Add(Destination);
	}
	Hero->FollowPath(Points);
}

// ---------------------------------------------------------------- Bucle

void AArpgPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	FHitResult Hit;
	Hovered = GetHitResultUnderCursor(ECC_Visibility, false, Hit) ? Hit.GetActor() : nullptr;
	if (Hovered.IsValid() && !Hovered->IsA<AArpgMonster>() && !Hovered->IsA<AArpgLootPickup>())
	{
		Hovered = nullptr;
	}

	AttackTimer = FMath::Max(0.f, AttackTimer - DeltaTime);
	RepathTimer = FMath::Max(0.f, RepathTimer - DeltaTime);

	AArpgPlayerCharacter* Hero = GetArpgCharacter();
	if (Hero == nullptr || !Hero->GetCombat()->IsAlive() || !Hero->GetCombat()->Body.Statuses.CanAct())
	{
		return;
	}
	UpdateAttack(Hero, DeltaTime);
	UpdatePickup(Hero);
}

void AArpgPlayerController::UpdateAttack(AArpgPlayerCharacter* Hero, float /*DeltaTime*/)
{
	AArpgMonster* Target = AttackTarget.Get();
	if (Target == nullptr || !Target->GetCombat()->IsAlive())
	{
		AttackTarget = nullptr;
		return;
	}
	const FVector ToTarget = Target->GetActorLocation() - Hero->GetActorLocation();
	if (ToTarget.Size2D() > Hero->GetBasicAttackRange())
	{
		if (RepathTimer <= 0.f)
		{
			RepathTimer = 0.3f;
			MoveCharacterTo(Target->GetActorLocation());
		}
		return;
	}

	Hero->StopMoving();
	Hero->SetActorRotation(ToTarget.GetSafeNormal2D().Rotation());
	if (AttackTimer > 0.f)
	{
		return;
	}
	AttackTimer = AttackInterval;

	const arpg::AttackOutcome Outcome = Target->GetCombat()->ReceiveAttack(Hero->GetCombat()->Attack);
	if (Outcome.DamageDealt > 0 && Hero->Stats.LifeOnHit > 0)
	{
		arpg::Heal(Hero->GetCombat()->Body, Hero->Stats.LifeOnHit);
	}
	if (AArpgHUD* Hud = Cast<AArpgHUD>(GetHUD()))
	{
		Hud->ShowHit(Target->GetActorLocation(), Outcome);
	}
#if ENABLE_DRAW_DEBUG
	DrawDebugLine(GetWorld(), Hero->GetActorLocation(), Target->GetActorLocation(), FColor::White, false, 0.1f, 0, 2.f);
#endif
}

void AArpgPlayerController::UpdatePickup(AArpgPlayerCharacter* Hero)
{
	AArpgLootPickup* Pickup = PickupTarget.Get();
	if (Pickup == nullptr)
	{
		return;
	}
	if (FVector::Dist2D(Pickup->GetActorLocation(), Hero->GetActorLocation()) <= 170.f)
	{
		Hero->StopMoving();
		Hero->TryPickup(Pickup);
		PickupTarget = nullptr;
	}
	else if (!Hero->HasPath())
	{
		MoveCharacterTo(Pickup->GetActorLocation());
	}
}

// ---------------------------------------------------------------- Entrada

void AArpgPlayerController::OnMoveStarted()
{
	HeldSeconds = 0.f;
	bClickOnInterface = ClickHitsInterface(false);
	if (bClickOnInterface)
	{
		return;
	}
	AttackTarget = nullptr;
	PickupTarget = nullptr;

	if (AArpgMonster* Monster = Cast<AArpgMonster>(Hovered.Get()))
	{
		if (Monster->GetCombat()->IsAlive())
		{
			AttackTarget = Monster;
			RepathTimer = 0.f;
			return;
		}
	}
	if (AArpgLootPickup* Pickup = Cast<AArpgLootPickup>(Hovered.Get()))
	{
		PickupTarget = Pickup;
		MoveCharacterTo(Pickup->GetActorLocation());
		return;
	}
	FVector Destination;
	if (GetGroundPointUnderCursor(Destination))
	{
		MoveCharacterTo(Destination);
	}
}

void AArpgPlayerController::OnMoveHeld()
{
	HeldSeconds += GetWorld()->GetDeltaSeconds();
	if (bClickOnInterface || AttackTarget.IsValid() || PickupTarget.IsValid() || HeldSeconds < ShortPressThreshold)
	{
		return;
	}
	// Clic mantenido: seguir al cursor de forma directa.
	AArpgPlayerCharacter* Hero = GetArpgCharacter();
	FVector Destination;
	if (Hero != nullptr && Hero->GetCombat()->IsAlive() && GetGroundPointUnderCursor(Destination))
	{
		Hero->StopMoving();
		Hero->AddMovementInput((Destination - Hero->GetActorLocation()).GetSafeNormal2D());
	}
}

void AArpgPlayerController::OnMoveReleased()
{
	bClickOnInterface = false;
	HeldSeconds = 0.f;
}

void AArpgPlayerController::OnSkill()
{
	if (ClickHitsInterface(true))
	{
		return;
	}
	AArpgPlayerCharacter* Hero = GetArpgCharacter();
	if (Hero != nullptr && Hero->GetCombat()->IsAlive())
	{
		CastSkill(Hero);
	}
}

void AArpgPlayerController::CastSkill(AArpgPlayerCharacter* Hero)
{
	const int32 SkillIndex = Hero->GetSkillIndex();
	const arpg::SkillDef* Skill = arpg::GetSkill(SkillIndex);
	AArpgHUD* Hud = Cast<AArpgHUD>(GetHUD());
	if (Skill == nullptr)
	{
		return;
	}
	const FVector Origin = Hero->GetActorLocation();
	FVector CursorPoint = Origin;
	GetGroundPointUnderCursor(CursorPoint);
	if (const AActor* HoveredActor = Hovered.Get())
	{
		CursorPoint = HoveredActor->GetActorLocation();
	}

	// 1) Elegir objetivos antes de gastar nada.
	TArray<AArpgMonster*> Targets;
	FVector EffectCenter = Origin;
	if (Skill->RadiusCm > 0)
	{
		// Área: alrededor del personaje si es de corto alcance; si no, donde apunta el cursor.
		if (Skill->RangeCm > 300)
		{
			const FVector Offset = (CursorPoint - Origin).GetClampedToMaxSize2D(static_cast<float>(Skill->RangeCm));
			EffectCenter = Origin + FVector(Offset.X, Offset.Y, 0.f);
		}
		for (TActorIterator<AArpgMonster> It(GetWorld()); It; ++It)
		{
			if (It->GetCombat()->IsAlive()
				&& FVector::Dist2D(It->GetActorLocation(), EffectCenter) <= static_cast<float>(Skill->RadiusCm))
			{
				Targets.Add(*It);
			}
		}
	}
	else
	{
		// Un objetivo: el señalado o el más cercano al cursor.
		AArpgMonster* Best = nullptr;
		float BestDistance = 260.f;
		for (TActorIterator<AArpgMonster> It(GetWorld()); It; ++It)
		{
			const float Distance = FVector::Dist2D(It->GetActorLocation(), CursorPoint);
			if (It->GetCombat()->IsAlive() && Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		if (Best == nullptr)
		{
			return;
		}
		if (FVector::Dist2D(Best->GetActorLocation(), Origin) > static_cast<float>(Skill->RangeCm))
		{
			if (Hud != nullptr) { Hud->ShowMessage(TEXT("Fuera de alcance")); }
			return;
		}
		Targets.Add(Best);
		EffectCenter = Best->GetActorLocation();
	}

	// 2) Las reglas deciden si se puede lanzar y cobran el coste.
	const arpg::ESkillUseResult Result = arpg::TryUseSkill(SkillIndex, Hero->GetCombat()->Body, Hero->Cooldowns);
	if (Result != arpg::ESkillUseResult::Ok)
	{
		if (Result == arpg::ESkillUseResult::NotEnoughMana && Hud != nullptr)
		{
			Hud->ShowMessage(TEXT("No hay maná suficiente"));
		}
		return;
	}

	// 3) Aplicar.
	Hero->StopMoving();
	const FVector Facing = (EffectCenter - Origin).GetSafeNormal2D();
	if (!Facing.IsNearlyZero())
	{
		Hero->SetActorRotation(Facing.Rotation());
	}
	const arpg::AttackProfile SkillAttack =
		arpg::BuildSkillAttack(*Skill, Hero->GetCombat()->Attack, Hero->State.Progress.Level);
	for (AArpgMonster* Target : Targets)
	{
		const arpg::AttackOutcome Outcome = Target->GetCombat()->ReceiveAttack(SkillAttack, &Skill->Status);
		if (Hud != nullptr)
		{
			Hud->ShowHit(Target->GetActorLocation(), Outcome);
		}
	}
#if ENABLE_DRAW_DEBUG
	// Efecto provisional hasta tener partículas.
	if (Skill->RadiusCm > 0)
	{
		DrawDebugSphere(GetWorld(), EffectCenter, static_cast<float>(Skill->RadiusCm), 20, FColor::Yellow, false, 0.3f);
	}
	else
	{
		DrawDebugLine(GetWorld(), Origin, EffectCenter, FColor::Cyan, false, 0.25f, 0, 6.f);
	}
#endif
}

void AArpgPlayerController::OnLifePotion()
{
	if (AArpgPlayerCharacter* Hero = GetArpgCharacter())
	{
		Hero->UsePotion(false);
	}
}

void AArpgPlayerController::OnManaPotion()
{
	if (AArpgPlayerCharacter* Hero = GetArpgCharacter())
	{
		Hero->UsePotion(true);
	}
}

void AArpgPlayerController::OnToggleInventory()
{
	if (AArpgHUD* Hud = Cast<AArpgHUD>(GetHUD()))
	{
		Hud->ToggleInventory();
	}
}

void AArpgPlayerController::OnZoom(const FInputActionValue& Value)
{
	if (AArpgPlayerCharacter* Hero = GetArpgCharacter())
	{
		Hero->AddZoomInput(Value.Get<float>());
	}
}

void AArpgPlayerController::OnRotateCamera(const FInputActionValue& Value)
{
	if (AArpgPlayerCharacter* Hero = GetArpgCharacter())
	{
		const float DegreesPerSecond = 90.f;
		Hero->AddCameraYawInput(Value.Get<float>() * DegreesPerSecond * GetWorld()->GetDeltaSeconds());
	}
}

void AArpgPlayerController::ArpgNuevoPersonaje(int32 Clase)
{
	AArpgPlayerCharacter* Hero = GetArpgCharacter();
	AArpgGameMode* GameMode = GetWorld()->GetAuthGameMode<AArpgGameMode>();
	if (Hero == nullptr || GameMode == nullptr || Clase < 0 || Clase >= static_cast<int32>(arpg::ECharacterClass::Count))
	{
		return;
	}
	Hero->ResetCharacter(static_cast<arpg::ECharacterClass>(Clase));
	GameMode->RestartFromState();
}
