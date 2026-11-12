#include "ArpgPlayerController.h"

#include "ArpgPlayerCharacter.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

AArpgPlayerController::AArpgPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Boolean;

	ZoomAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Zoom"));
	ZoomAction->ValueType = EInputActionValueType::Axis1D;

	RotateCameraAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_RotateCamera"));
	RotateCameraAction->ValueType = EInputActionValueType::Axis1D;

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
		Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AArpgPlayerController::OnZoom);
		Input->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &AArpgPlayerController::OnRotateCamera);
	}
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

void AArpgPlayerController::OnMoveStarted()
{
	HeldSeconds = 0.f;
	bHasDestination = false;
	StopMovement();
}

void AArpgPlayerController::OnMoveHeld()
{
	HeldSeconds += GetWorld()->GetDeltaSeconds();

	FVector GroundPoint;
	if (!GetGroundPointUnderCursor(GroundPoint))
	{
		return;
	}
	CachedDestination = GroundPoint;
	bHasDestination = true;
	if (APawn* ControlledPawn = GetPawn())
	{
		const FVector Direction = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
		ControlledPawn->AddMovementInput(Direction, 1.f, false);
	}
}

void AArpgPlayerController::OnMoveReleased()
{
	if (bHasDestination && HeldSeconds <= ShortPressThreshold)
	{
		// Clic corto: ruta completa por NavMesh.
		// Fase 12: en red esta orden pasa a ser un RPC validado por el servidor.
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
	}
	HeldSeconds = 0.f;
}

void AArpgPlayerController::OnZoom(const FInputActionValue& Value)
{
	if (AArpgPlayerCharacter* PlayerCharacter = Cast<AArpgPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->AddZoomInput(Value.Get<float>());
	}
}

void AArpgPlayerController::OnRotateCamera(const FInputActionValue& Value)
{
	if (AArpgPlayerCharacter* PlayerCharacter = Cast<AArpgPlayerCharacter>(GetPawn()))
	{
		const float DegreesPerSecond = 90.f;
		PlayerCharacter->AddCameraYawInput(Value.Get<float>() * DegreesPerSecond * GetWorld()->GetDeltaSeconds());
	}
}
