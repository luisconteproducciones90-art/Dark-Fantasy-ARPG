#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ArpgPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Control clásico de ARPG: clic izquierdo para moverse.
 *  - Clic corto: el personaje navega hasta el punto esquivando obstáculos (NavMesh).
 *  - Clic mantenido: el personaje sigue al cursor de forma directa.
 *  - Rueda: zoom acotado.
 * Las acciones de entrada se crean en código para no depender de assets binarios.
 */
UCLASS()
class ARPGGAME_API AArpgPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArpgPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Por debajo de este tiempo un clic cuenta como "corto" y usa navegación. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ShortPressThreshold = 0.3f;

private:
	void OnMoveStarted();
	void OnMoveHeld();
	void OnMoveReleased();
	void OnZoom(const FInputActionValue& Value);
	void OnRotateCamera(const FInputActionValue& Value);

	bool GetGroundPointUnderCursor(FVector& OutLocation) const;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY()
	TObjectPtr<UInputAction> RotateCameraAction;

	FVector CachedDestination = FVector::ZeroVector;
	float HeldSeconds = 0.f;
	bool bHasDestination = false;
};
