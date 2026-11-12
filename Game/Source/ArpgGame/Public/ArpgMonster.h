#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArpgCore/Enemy/Monster.h"
#include "ArpgMonster.generated.h"

class AArpgDungeonActor;
class UArpgCombatComponent;
class UStaticMeshComponent;

/**
 * Monstruo. Cada fotograma reúne lo que percibe, pregunta a ArpgCore qué
 * hacer (arpg::DecideAction) y lo ejecuta. La decisión es la misma que se
 * prueba sin motor; aquí solo hay percepción, movimiento y presentación.
 */
UCLASS()
class ARPGGAME_API AArpgMonster : public ACharacter
{
	GENERATED_BODY()

public:
	AArpgMonster();

	void Init(const arpg::MonsterInstance& InInstance, AArpgDungeonActor* InDungeon);

	virtual void Tick(float DeltaSeconds) override;

	UArpgCombatComponent* GetCombat() const { return Combat; }
	const arpg::MonsterInstance& GetInstance() const { return Instance; }
	FString GetDisplayName() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UArpgCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** Distancia a la que detecta al jugador. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AggroRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackInterval = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float SpecialInterval = 6.f;

private:
	void HandleDied();
	void MoveTowards(const FVector& Target, float DeltaSeconds);
	void AttackTarget(AActor* Target, UArpgCombatComponent* TargetCombat);
	void UseSpecial(AActor* Target, UArpgCombatComponent* TargetCombat);
	void SetRevealed(bool bNowRevealed);

	UPROPERTY()
	TObjectPtr<AArpgDungeonActor> Dungeon;

	arpg::MonsterInstance Instance;
	arpg::EMonsterBehavior Behavior = arpg::EMonsterBehavior::MeleeChaser;
	int32 AttackRangeCm = 150;

	TArray<FVector> Path;
	float RepathTimer = 0.f;
	float AttackTimer = 0.f;
	float SpecialTimer = 0.f;
	float ChargeTimeLeft = 0.f;
	float BaseSpeed = 380.f;
	FVector BaseMeshScale = FVector::OneVector;
	bool bAggro = false;
	bool bRevealed = true;
};
