#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArpgPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Personaje jugable con cámara isométrica elevada.
 * La cámara no hereda la rotación del personaje: mantiene un ángulo fijo y lo
 * sigue. Zoom y giro están acotados; el giro viene desactivado por defecto.
 */
UCLASS()
class ARPGGAME_API AArpgPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AArpgPlayerCharacter();

	/** Delta > 0 acerca la cámara. Se satura a [MinZoomDistance, MaxZoomDistance]. */
	void AddZoomInput(float Delta);

	/** Gira la cámara alrededor del personaje, limitado a ±MaxYawOffset. */
	void AddCameraYawInput(float DeltaDegrees);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> IsoCamera;

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
	float TargetZoomDistance = 0.f;
	float YawOffset = 0.f;
};
