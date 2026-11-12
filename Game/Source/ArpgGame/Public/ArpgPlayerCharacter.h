#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArpgCore/Save/SaveGame.h"
#include "ArpgCore/Skill/Skill.h"
#include "ArpgPlayerCharacter.generated.h"

class AArpgLootPickup;
class UArpgCombatComponent;
class UCameraComponent;
class UPointLightComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/**
 * Personaje jugable con cámara isométrica elevada.
 * La cámara no hereda la rotación del personaje: mantiene un ángulo fijo y lo
 * sigue. Zoom y giro están acotados; el giro viene desactivado por defecto.
 * Todo su estado de juego (progreso, equipo, mochila, oro) vive en State, que
 * es exactamente lo que se guarda en disco.
 */
UCLASS()
class ARPGGAME_API AArpgPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AArpgPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;

	// ---- Cámara ----
	/** Delta > 0 acerca la cámara. Se satura a [MinZoomDistance, MaxZoomDistance]. */
	void AddZoomInput(float Delta);
	/** Gira la cámara alrededor del personaje, limitado a ±MaxYawOffset. */
	void AddCameraYawInput(float DeltaDegrees);

	// ---- Estado de juego ----
	UArpgCombatComponent* GetCombat() const { return Combat; }
	void ResetCharacter(arpg::ECharacterClass Class);
	void ApplyLoadedState(const arpg::CharacterSave& Loaded);
	/** Recalcula estadísticas tras cambiar nivel, atributos o equipo. bRefill deja vida y maná al máximo. */
	void RefreshStats(bool bRefill);
	void GainExperience(int64 Amount);
	bool SpendAttributePoint(arpg::EAttribute Attribute);
	bool UsePotion(bool bMana);
	bool TryPickup(AArpgLootPickup* Pickup);
	bool EquipFromBag(int32 EntryId);
	bool UnequipToBag(arpg::EEquipSlot Slot);
	bool DropFromBag(int32 EntryId);
	void Respawn(const FVector& Location);

	int32 GetSkillIndex() const { return arpg::GetClassSkill(State.Progress.Class); }
	float GetBasicAttackRange() const;

	// ---- Movimiento ----
	void FollowPath(const TArray<FVector>& Points);
	void StopMoving();
	bool HasPath() const { return Path.Num() > 0; }

	arpg::CharacterSave State;
	arpg::EquippedStats Stats;
	arpg::SkillCooldowns Cooldowns;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> IsoCamera;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UArpgCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** Antorcha: la única luz cálida de la escena. */
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UPointLightComponent> Torch;

	/** Inclinación de la cámara respecto al suelo, en grados. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "35", ClampMax = "70"))
	float CameraPitch = 52.f;

	/** Orientación base: 45° da la lectura isométrica clásica. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraBaseYaw = 45.f;

	/** FOV estrecho + distancia larga = perspectiva casi isométrica. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "20", ClampMax = "60"))
	float CameraFieldOfView = 35.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float DefaultZoomDistance = 2100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float MinZoomDistance = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float MaxZoomDistance = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float ZoomStep = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float ZoomInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Rotation")
	bool bAllowCameraRotation = false;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Rotation", meta = (ClampMin = "0", ClampMax = "90"))
	float MaxYawOffset = 45.f;

private:
	void HandleDied();
	void Announce(const FString& Text) const;
	void DropItemAtFeet(const arpg::ItemInstance& Item) const;

	TArray<FVector> Path;
	float TargetZoomDistance = 0.f;
	float YawOffset = 0.f;
	float BaseWalkSpeed = 600.f;
};
