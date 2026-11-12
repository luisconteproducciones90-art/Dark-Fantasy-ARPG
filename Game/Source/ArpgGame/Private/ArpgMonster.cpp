#include "ArpgMonster.h"

#include "ArpgCombatComponent.h"
#include "ArpgDungeonActor.h"
#include "ArpgGameMode.h"
#include "ArpgText.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AArpgMonster::AArpgMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// Se mueve con su propia lógica; no necesita controlador de IA del motor.
	AutoPossessAI = EAutoPossessAI::Disabled;
	bUseControllerRotationYaw = false;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bRunPhysicsWithNoController = true;
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
	Movement->MaxAcceleration = 4096.f;
	Movement->BrakingDecelerationWalking = 4096.f;

	Combat = CreateDefaultSubobject<UArpgCombatComponent>(TEXT("Combat"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetStaticMesh(ConeMesh.Object);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	// El cuerpo visible solo responde al cursor; quien colisiona es la cápsula.
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AArpgMonster::Init(const arpg::MonsterInstance& InInstance, AArpgDungeonActor* InDungeon)
{
	Instance = InInstance;
	Dungeon = InDungeon;

	Combat->Body = Instance.Body;
	Combat->Attack = Instance.Attack;
	Combat->Defense = Instance.Defense;
	Combat->StatusDurationPercent = Instance.StatusDurationPercent;
	Combat->OnDied.AddUObject(this, &AArpgMonster::HandleDied);

	if (const arpg::MonsterDef* Def = arpg::GetMonsterDef(Instance.DefIndex))
	{
		Behavior = Def->Behavior;
		AttackRangeCm = Def->AttackRangeCm;
	}

	// Tamaño y color por rango: se distinguen de un vistazo.
	float Scale = 1.f;
	FLinearColor Color(0.45f, 0.42f, 0.38f);
	switch (Instance.Rank)
	{
	case arpg::EMonsterRank::Champion: Scale = 1.25f; Color = FLinearColor(0.2f, 0.35f, 0.9f); break;
	case arpg::EMonsterRank::Elite:    Scale = 1.5f;  Color = FLinearColor(0.9f, 0.75f, 0.1f); break;
	case arpg::EMonsterRank::Boss:     Scale = 2.4f;  Color = FLinearColor(0.7f, 0.05f, 0.05f); break;
	default: break;
	}
	BaseMeshScale = FVector(0.8f * Scale, 0.8f * Scale, 1.8f * Scale);
	BodyMesh->SetRelativeScale3D(BaseMeshScale);
	if (UMaterialInstanceDynamic* Material = BodyMesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}

	BaseSpeed = Behavior == arpg::EMonsterBehavior::Exploder ? 480.f : 380.f;
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	SpecialTimer = Behavior == arpg::EMonsterBehavior::Charger ? 1.f : 0.f;

	if (Behavior == arpg::EMonsterBehavior::Ambusher)
	{
		SetRevealed(false);
	}
}

FString AArpgMonster::GetDisplayName() const
{
	return ArpgText::MonsterName(Instance.DefIndex, Instance.Rank);
}

void AArpgMonster::SetRevealed(bool bNowRevealed)
{
	bRevealed = bNowRevealed;
	// Enterrado: apenas asoma del suelo.
	BodyMesh->SetRelativeScale3D(bRevealed ? BaseMeshScale : FVector(BaseMeshScale.X, BaseMeshScale.Y, 0.15f));
}

void AArpgMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Combat->IsAlive())
	{
		return;
	}
	AttackTimer = FMath::Max(0.f, AttackTimer - DeltaSeconds);
	SpecialTimer = FMath::Max(0.f, SpecialTimer - DeltaSeconds);
	RepathTimer = FMath::Max(0.f, RepathTimer - DeltaSeconds);
	ChargeTimeLeft = FMath::Max(0.f, ChargeTimeLeft - DeltaSeconds);

	// Escarcha, congelación y aturdimiento salen de las reglas, no de aquí.
	const int32 SpeedPercent = Combat->Body.Statuses.MoveSpeedPercent();
	const float ChargeBoost = ChargeTimeLeft > 0.f ? 2.6f : 1.f;
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * ChargeBoost * static_cast<float>(SpeedPercent) / 100.f;
	if (!Combat->Body.Statuses.CanAct())
	{
		return;
	}

	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* Player = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	UArpgCombatComponent* PlayerCombat = Player != nullptr ? Player->FindComponentByClass<UArpgCombatComponent>() : nullptr;

	arpg::AiContext Context;
	Context.AttackRangeCm = AttackRangeCm;
	Context.LifePercent = FMath::RoundToInt32(Combat->GetLifeFraction() * 100.f);
	Context.bSpecialReady = SpecialTimer <= 0.f;
	Context.bRevealed = bRevealed;
	if (Player != nullptr && PlayerCombat != nullptr && PlayerCombat->IsAlive())
	{
		const float Distance = FVector::Dist2D(Player->GetActorLocation(), GetActorLocation());
		// Una vez que ve al jugador lo persigue hasta el doble de distancia.
		bAggro = Distance <= (bAggro ? AggroRange * 2.f : AggroRange);
		Context.bHasTarget = bAggro;
		Context.DistanceCm = FMath::RoundToInt32(Distance);
	}
	else
	{
		bAggro = false;
	}

	switch (arpg::DecideAction(Behavior, Context))
	{
	case arpg::EAiAction::Approach:
		MoveTowards(Player->GetActorLocation(), DeltaSeconds);
		break;

	case arpg::EAiAction::Retreat:
		AddMovementInput((GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D());
		break;

	case arpg::EAiAction::Attack:
		Path.Reset();
		AttackTarget(Player, PlayerCombat);
		break;

	case arpg::EAiAction::UseSpecial:
		UseSpecial(Player, PlayerCombat);
		break;

	case arpg::EAiAction::StayHidden:
	case arpg::EAiAction::Idle:
	default:
		Path.Reset();
		break;
	}
}

void AArpgMonster::MoveTowards(const FVector& Target, float DeltaSeconds)
{
	if (RepathTimer <= 0.f || Path.Num() == 0)
	{
		RepathTimer = 0.4f;
		if (Dungeon == nullptr || !Dungeon->FindWorldPath(GetActorLocation(), Target, Path))
		{
			Path.Reset();
			Path.Add(Target); // sin camino conocido: en línea recta
		}
	}
	while (Path.Num() > 0 && FVector::Dist2D(Path[0], GetActorLocation()) < 60.f)
	{
		Path.RemoveAt(0);
	}
	if (Path.Num() > 0)
	{
		AddMovementInput((Path[0] - GetActorLocation()).GetSafeNormal2D());
	}
}

void AArpgMonster::AttackTarget(AActor* Target, UArpgCombatComponent* TargetCombat)
{
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(ToTarget.Rotation());
	}
	if (AttackTimer > 0.f)
	{
		return;
	}
	AttackTimer = AttackInterval;
	TargetCombat->ReceiveAttack(Combat->Attack);
#if ENABLE_DRAW_DEBUG
	// Provisional hasta tener animaciones y proyectiles: una línea marca el ataque.
	DrawDebugLine(GetWorld(), GetActorLocation(), Target->GetActorLocation(), FColor::Red, false, 0.15f, 0, 3.f);
#endif
}

void AArpgMonster::UseSpecial(AActor* Target, UArpgCombatComponent* TargetCombat)
{
	switch (Behavior)
	{
	case arpg::EMonsterBehavior::Ambusher:
		SetRevealed(true);
		AttackTimer = 0.4f; // un instante para reaccionar antes del primer golpe
		break;

	case arpg::EMonsterBehavior::Charger:
		ChargeTimeLeft = 1.2f;
		SpecialTimer = SpecialInterval;
		break;

	case arpg::EMonsterBehavior::Exploder:
	{
		AttackTimer = 0.f;
		AttackTarget(Target, TargetCombat);
#if ENABLE_DRAW_DEBUG
		DrawDebugSphere(GetWorld(), GetActorLocation(), 220.f, 16, FColor::Orange, false, 0.4f);
#endif
		Combat->Kill();
		break;
	}

	default:
		// Invocar y curar aliados llegan con la Fase 8; de momento atacan.
		SpecialTimer = SpecialInterval;
		AttackTarget(Target, TargetCombat);
		break;
	}
}

void AArpgMonster::HandleDied()
{
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();
	BodyMesh->SetRelativeScale3D(FVector(BaseMeshScale.X * 1.3f, BaseMeshScale.Y * 1.3f, 0.1f)); // cae al suelo
	SetLifeSpan(2.f);

	if (AArpgGameMode* GameMode = GetWorld()->GetAuthGameMode<AArpgGameMode>())
	{
		GameMode->NotifyMonsterDied(this);
	}
}
