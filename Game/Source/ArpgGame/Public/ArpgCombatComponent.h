#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArpgCore/Combat/Combatant.h"
#include "ArpgCombatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FArpgOnDamaged, int32 /*Amount*/, bool /*bCritical*/);
DECLARE_MULTICAST_DELEGATE(FArpgOnDied);

/**
 * Puente entre un actor y las reglas de ArpgCore: guarda la criatura de
 * combate y sus perfiles, avanza estados y regeneración, y avisa de daño y muerte.
 * Toda decisión de daño la toma ArpgCore; aquí no hay fórmulas.
 */
UCLASS()
class ARPGGAME_API UArpgCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArpgCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool IsAlive() const { return Body.IsAlive(); }
	float GetLifeFraction() const { return Body.MaxLife > 0 ? static_cast<float>(Body.Life) / static_cast<float>(Body.MaxLife) : 0.f; }
	float GetManaFraction() const { return Body.MaxMana > 0 ? static_cast<float>(Body.Mana) / static_cast<float>(Body.MaxMana) : 0.f; }

	/** Recibe un ataque completo. Devuelve lo ocurrido para que el atacante muestre efectos. */
	arpg::AttackOutcome ReceiveAttack(const arpg::AttackProfile& Attacker, const arpg::OnHitStatus* Status = nullptr);

	/** Mata sin pasar por defensas (p. ej. un monstruo que estalla). */
	void Kill();

	arpg::Combatant Body;
	arpg::AttackProfile Attack;
	arpg::DefenseProfile Defense;
	/** % de duración de estados que sufre (los jefes menos de 100). */
	int32 StatusDurationPercent = 100;

	FArpgOnDamaged OnDamaged;
	FArpgOnDied OnDied;

private:
	void HandleDeathIfNeeded();

	float PendingMs = 0.f;
	bool bDeathNotified = false;
};
