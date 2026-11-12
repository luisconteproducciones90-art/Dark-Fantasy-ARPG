#include "ArpgPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
	Movement->MaxWalkSpeed = 600.f;
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
}

void AArpgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	TargetZoomDistance = FMath::Clamp(DefaultZoomDistance, MinZoomDistance, MaxZoomDistance);
	CameraBoom->TargetArmLength = TargetZoomDistance;
	CameraBoom->SetWorldRotation(FRotator(-CameraPitch, CameraBaseYaw, 0.f));
	IsoCamera->SetFieldOfView(CameraFieldOfView);
}

void AArpgPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength, TargetZoomDistance, DeltaSeconds, ZoomInterpSpeed);
}

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
