#include "ArpgCombatComponent.h"

#include "ArpgText.h"

UArpgCombatComponent::UArpgCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UArpgCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Las reglas trabajan en milisegundos enteros; se conserva la fracción entre fotogramas.
	PendingMs += DeltaTime * 1000.f;
	const int32 WholeMs = FMath::FloorToInt32(PendingMs);
	if (WholeMs <= 0)
	{
		return;
	}
	PendingMs -= static_cast<float>(WholeMs);

	const arpg::TickResult Result = arpg::TickCombatant(Body, WholeMs);
	if (Result.StatusDamage > 0)
	{
		OnDamaged.Broadcast(Result.StatusDamage, false);
	}
	HandleDeathIfNeeded();
}

arpg::AttackOutcome UArpgCombatComponent::ReceiveAttack(const arpg::AttackProfile& Attacker, const arpg::OnHitStatus* Status)
{
	// La maldición baja las resistencias mientras dura.
	arpg::DefenseProfile Effective = Defense;
	const int32 Penalty = Body.Statuses.ResistPenalty();
	for (int32& Resist : Effective.ResistPercent)
	{
		Resist -= Penalty;
	}

	const arpg::AttackOutcome Outcome =
		arpg::PerformAttack(Attacker, Effective, Body, ArpgGlobalRng(), Status, StatusDurationPercent);
	if (Outcome.DamageDealt > 0)
	{
		OnDamaged.Broadcast(Outcome.DamageDealt, Outcome.Hit.Outcome == arpg::EHitOutcome::Critical);
	}
	HandleDeathIfNeeded();
	return Outcome;
}

void UArpgCombatComponent::Kill()
{
	arpg::ApplyDamage(Body, Body.Life);
	HandleDeathIfNeeded();
}

void UArpgCombatComponent::HandleDeathIfNeeded()
{
	if (Body.IsAlive())
	{
		bDeathNotified = false; // permite volver a morir tras reaparecer
		return;
	}
	if (!bDeathNotified)
	{
		bDeathNotified = true;
		OnDied.Broadcast();
	}
}
