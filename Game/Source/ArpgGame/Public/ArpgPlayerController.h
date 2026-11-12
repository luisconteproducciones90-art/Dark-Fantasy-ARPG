#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ArpgPlayerController.generated.h"

class AArpgLootPickup;
class AArpgMonster;
class AArpgPlayerCharacter;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Control clásico de ARPG.
 *  - Clic izquierdo: mover, atacar al monstruo señalado o recoger el botín señalado.
 *    Clic corto = ruta esquivando muros; clic mantenido = seguir al cursor.
 *  - Clic derecho: habilidad de la clase.
 *  - 1 / 2: poción de vida / maná.  I: inventario.  Rueda: zoom.
 * Las acciones de entrada se crean en código para no depender de assets binarios.
 */
UCLASS()
class ARPGGAME_API AArpgPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArpgPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	/** Actor bajo el cursor (monstruo o botín), o nullptr. */
	AActor* GetHoveredActor() const { return Hovered.Get(); }

	/** Consola: crea un personaje nuevo. 0 Guerrero, 1 Hechicero, 2 Cazador, 3 Nigromante, 4 Asesino, 5 Paladín. */
	UFUNCTION(Exec)
	void ArpgNuevoPersonaje(int32 Clase);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Por debajo de este tiempo un clic cuenta como "corto". */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ShortPressThreshold = 0.3f;

	/** Segundos entre ataques básicos. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AttackInterval = 0.7f;

private:
	void OnMoveStarted();
	void OnMoveHeld();
	void OnMoveReleased();
	void OnSkill();
	void OnLifePotion();
	void OnManaPotion();
	void OnToggleInventory();
	void OnZoom(const FInputActionValue& Value);
	void OnRotateCamera(const FInputActionValue& Value);

	AArpgPlayerCharacter* GetArpgCharacter() const;
	bool GetGroundPointUnderCursor(FVector& OutLocation) const;
	bool ClickHitsInterface(bool bRightButton) const;
	void MoveCharacterTo(const FVector& Destination);
	void UpdateAttack(AArpgPlayerCharacter* Hero, float DeltaTime);
	void UpdatePickup(AArpgPlayerCharacter* Hero);
	void CastSkill(AArpgPlayerCharacter* Hero);

	UPROPERTY()
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> SkillAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LifePotionAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ManaPotionAction;

	UPROPERTY()
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY()
	TObjectPtr<UInputAction> RotateCameraAction;

	TWeakObjectPtr<AActor> Hovered;
	TWeakObjectPtr<AArpgMonster> AttackTarget;
	TWeakObjectPtr<AArpgLootPickup> PickupTarget;
	float HeldSeconds = 0.f;
	float AttackTimer = 0.f;
	float RepathTimer = 0.f;
	bool bClickOnInterface = false;
};
