// Nova project - Gwennaël Arbona

#include "NovaTurntablePawn.h"

#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"
#include "Nova/UI/NovaUITypes.h"
#include "Nova/Nova.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "ANovaTurntablePawn"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaTurntablePawn::ANovaTurntablePawn()
	: Super()
	, ChaseMode(false)
	, CurrentPanTarget(0)
	, CurrentTiltTarget(0)
	, CurrentPanAngle(0)
	, CurrentTiltAngle(0)
	, CurrentPanSpeed(0)
	, CurrentTiltSpeed(0)
	, CurrentOffset(0)
	, PreviousOffset(0)
	, CurrentDistance(10000)
	, CurrentDistanceFactor(1000)
	, CurrentAnimationTime(0)
{
	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create the spring arm component that moves to avoid geometry
	CameraYawComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CameraYawComponent"));
	CameraYawComponent->SetupAttachment(RootComponent);

	// Create the camera container component
	CameraPitchComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CameraPitchComponent"));
	CameraPitchComponent->SetupAttachment(CameraYawComponent);

	// Create camera spring arm
	CameraArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArmComponent"));
	CameraArmComponent->SetupAttachment(CameraPitchComponent);
	CameraArmComponent->TargetArmLength  = 2000;
	CameraArmComponent->bDoCollisionTest = false;

	// Create camera component
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArmComponent, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// Settings
	bAlwaysRelevant = true;
	SetActorTickEnabled(true);
	PrimaryActorTick.bCanEverTick          = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// General presentation defaults
	AnimationDuration           = 0.5f;
	DefaultDistance             = 500.0f;
	DefaultDistanceFactor       = 2.0f;
	DistanceFactorIncrement     = 0.5f;
	NumDistanceFactorIncrements = 1;

	// Camera control defaults
	CameraMinTilt       = -80.0f;
	CameraMaxTilt       = 20.0f;
	CameraMaxChaseAngle = 20.0f;
}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaTurntablePawn::BeginPlay()
{
	NLOG("ANovaTurntableActor::BeginPlay");

	Super::BeginPlay();

	ResetView();
	SetChaseMode(false);
}

void ANovaTurntablePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Extract coordinates
	const TPair<FVector, FVector> OriginExtent = GetTurntableBounds();
	const FVector                 Origin       = OriginExtent.Key;
	const FVector                 Extent       = OriginExtent.Value;

	// Extract distance
	float Distance = CurrentDistanceFactor * Extent.Size();
	if (Distance < KINDA_SMALL_NUMBER)
	{
		Distance = DefaultDistance;
	}

	// Detect offset changes
	if (CurrentAnimationTime >= AnimationDuration && (Origin.X != CurrentOffset || Distance != CurrentDistance))
	{
		PreviousOffset = CurrentOffset;
		CurrentOffset  = Origin.X;

		PreviousDistance = CurrentDistance;
		CurrentDistance  = Distance;

		CurrentAnimationTime = 0;
	}

	// Process interpolation
	CurrentAnimationTime += DeltaTime;
	CurrentAnimationTime       = FMath::Clamp(CurrentAnimationTime, 0.0f, AnimationDuration);
	float Alpha                = CurrentAnimationTime / AnimationDuration;
	float InterpolatedOffset   = FMath::InterpEaseInOut(PreviousOffset, CurrentOffset, Alpha, ENovaUIConstants::EaseStandard);
	float InterpolatedDistance = FMath::InterpEaseInOut(PreviousDistance, CurrentDistance, Alpha, ENovaUIConstants::EaseStandard);

	// Apply values
	CameraYawComponent->SetRelativeLocation(FVector(InterpolatedOffset, 0, 0));
	CameraArmComponent->TargetArmLength = InterpolatedDistance;

	// Update camera
	ProcessCamera(DeltaTime);
}

void ANovaTurntablePawn::ResetView()
{
	CurrentTiltAngle      = ChaseMode ? 0 : -45;
	CurrentPanAngle       = 135;
	CurrentDistance       = DefaultDistance;
	CurrentDistanceFactor = DefaultDistanceFactor;
	CurrentAnimationTime  = AnimationDuration;
}

void ANovaTurntablePawn::SetChaseMode(bool Enabled)
{
	CameraYawComponent->SetAbsolute(false, !Enabled, false);

	if (ChaseMode != Enabled)
	{
		ChaseMode = Enabled;

		ResetView();
	}
}

TPair<FVector, FVector> ANovaTurntablePawn::GetTurntableBounds() const
{
	return TPair<FVector, FVector>(FVector::ZeroVector, FVector::ZeroVector);
}

void ANovaTurntablePawn::PanInput(float Val)
{
	CurrentPanTarget += Val;
}

void ANovaTurntablePawn::TiltInput(float Val)
{
	CurrentTiltTarget += Val;
}

void ANovaTurntablePawn::ZoomIn()
{
	CurrentDistanceFactor -= DistanceFactorIncrement;
	float MinDistanceFactor = DefaultDistanceFactor - NumDistanceFactorIncrements * DistanceFactorIncrement;
	CurrentDistanceFactor   = FMath::Clamp(CurrentDistanceFactor, MinDistanceFactor, DefaultDistanceFactor);
}

void ANovaTurntablePawn::ZoomOut()
{
	CurrentDistanceFactor += DistanceFactorIncrement;
	float MinDistanceFactor = DefaultDistanceFactor - NumDistanceFactorIncrements * DistanceFactorIncrement;
	CurrentDistanceFactor   = FMath::Clamp(CurrentDistanceFactor, MinDistanceFactor, DefaultDistanceFactor);
}

void ANovaTurntablePawn::ResetZoom()
{
	CurrentDistanceFactor = DefaultDistanceFactor;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaTurntablePawn::ProcessCamera(float DeltaTime)
{
	// Apply filter
	bool IsGamepad = GetGameInstance<UNovaGameInstance>()->GetMenuManager()->IsUsingGamepad();
	CameraFilter.ApplyFilter<true>(CurrentPanAngle, CurrentPanSpeed, CurrentPanTarget, DeltaTime, IsGamepad);
	CameraFilter.ApplyFilter<true>(CurrentTiltAngle, CurrentTiltSpeed, CurrentTiltTarget, DeltaTime, IsGamepad);

	// Clamp pitch to legal values
	float ExternalCameraPitchClamped = ChaseMode ? FMath::Clamp(CurrentTiltAngle, -CameraMaxChaseAngle, CameraMaxChaseAngle)
												 : FMath::Clamp(CurrentTiltAngle, CameraMinTilt, CameraMaxTilt);
	if (ExternalCameraPitchClamped != CurrentTiltAngle)
	{
		CurrentTiltSpeed = -CurrentTiltSpeed / 2;
	}
	CurrentTiltAngle = ExternalCameraPitchClamped;

	// Clamp yaw to legal values in chase mode
	if (ChaseMode)
	{
		float ExternalCameraYawClamped = FMath::Clamp(CurrentPanAngle, -CameraMaxChaseAngle, CameraMaxChaseAngle);
		if (ExternalCameraYawClamped != CurrentPanAngle)
		{
			CurrentPanSpeed = -CurrentPanSpeed / 2;
		}
		CurrentPanAngle = ExternalCameraYawClamped;
	}

	// Apply values
	CameraYawComponent->SetRelativeRotation(FRotator(0, CurrentPanAngle, 0));
	CameraPitchComponent->SetRelativeRotation(FRotator(CurrentTiltAngle, 0, 0));
}

#undef LOCTEXT_NAMESPACE
