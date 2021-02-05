// Nova project - Gwennaël Arbona

#include "NovaTurntablePawn.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Game/NovaGameInstance.h"

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
	CameraYawComponent->SetAbsolute(false, true, false);

	// Create the camera container component
	CameraPitchComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CameraPitchComponent"));
	CameraPitchComponent->SetupAttachment(CameraYawComponent);

	// Create camera spring arm
	CameraArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArmComponent"));
	CameraArmComponent->SetupAttachment(CameraPitchComponent);
	CameraArmComponent->TargetArmLength = 2000;
	CameraArmComponent->bDoCollisionTest = false;

	// Create camera component
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArmComponent, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// Settings
	bAlwaysRelevant = true;
	SetActorTickEnabled(true);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Default camera parameters
	AnimationDuration = 0.5f;
	DefaultDistance = 500.0f;
	DefaultDistanceFactor = 2.0f;
	MinDistanceFactor = 0.15f;
	DistanceFactorIncrement = 0.15f;
	CameraMinTilt = -80.0f;
	CameraMaxTilt = 20.0f;
	CameraVelocity = 200.0f;
	CameraGamepadVelocity = 100.0f;
	CameraAcceleration = 300.0f;
	CameraResistance = 1 / 360.0f;
	CameraInputPower = 3.0f;
}


/*----------------------------------------------------
	Gameplay
----------------------------------------------------*/

void ANovaTurntablePawn::BeginPlay()
{
	NLOG("ANovaTurntableActor::BeginPlay");

	Super::BeginPlay();

	ResetView();
}

void ANovaTurntablePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Extract coordinates
	const TPair<FVector, FVector> OriginExtent = GetTurntableBounds();
	const FVector Origin = OriginExtent.Key;
	const FVector Extent = OriginExtent.Value;

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
		CurrentOffset = Origin.X;

		PreviousDistance = CurrentDistance;
		CurrentDistance = Distance;

		CurrentAnimationTime = 0;
	}

	// Process interpolation
	CurrentAnimationTime += DeltaTime;
	CurrentAnimationTime = FMath::Clamp(CurrentAnimationTime, 0.0f, AnimationDuration);
	float Alpha = CurrentAnimationTime / AnimationDuration;
	float InterpolatedOffset = FMath::InterpEaseInOut(PreviousOffset, CurrentOffset, Alpha, ENovaUIConstants::EaseStandard);
	float InterpolatedDistance = FMath::InterpEaseInOut(PreviousDistance, CurrentDistance, Alpha, ENovaUIConstants::EaseStandard);

	// Apply values
	CameraYawComponent->SetRelativeLocation(FVector(InterpolatedOffset, 0, 0));
	CameraArmComponent->TargetArmLength = InterpolatedDistance;

	// Update camera
	ProcessCamera(DeltaTime);
}

void ANovaTurntablePawn::ResetView()
{
	CurrentTiltAngle = -45;
	CurrentPanAngle = 135;
	CurrentDistance = DefaultDistance;
	CurrentDistanceFactor = DefaultDistanceFactor;
	CurrentAnimationTime = AnimationDuration;
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
	CurrentDistanceFactor = FMath::Clamp(CurrentDistanceFactor, MinDistanceFactor, DefaultDistanceFactor);
}

void ANovaTurntablePawn::ZoomOut()
{
	CurrentDistanceFactor += DistanceFactorIncrement;
	CurrentDistanceFactor = FMath::Clamp(CurrentDistanceFactor, MinDistanceFactor, DefaultDistanceFactor);
}


/*----------------------------------------------------
	Internals
----------------------------------------------------*/

void ANovaTurntablePawn::ProcessCamera(float DeltaTime)
{
	auto ApplyInputFilter = [&](float& CurrentAngle, float& CurrentSpeed, float& TargetAngle, const float MaximumSpeed)
	{
		float Brake = 2.f;
		float Brake2 = 4.f;

		// Compute acceleration and resistance
		float Acc = FMath::Pow(TargetAngle, CameraInputPower) * CameraAcceleration;
		float Res = FMath::Sign(CurrentSpeed) * (CameraResistance * FMath::Square(CurrentSpeed)
			+ (Acc == 0 ? Brake2 + Brake * FMath::Abs(CurrentSpeed) : 0));
		float MaxResDeltaSpeed = CurrentSpeed;
		float AccDeltaSpeed = Acc * DeltaTime;
		float ResDeltaSpeed = -(FMath::Abs(Res * DeltaTime) > FMath::Abs(MaxResDeltaSpeed) ? MaxResDeltaSpeed : Res * DeltaTime);

		// Update velocity, integrate the current angle and consume the input
		CurrentSpeed += AccDeltaSpeed + ResDeltaSpeed;
		CurrentSpeed = FMath::Clamp(CurrentSpeed, -MaximumSpeed, MaximumSpeed);
		CurrentAngle += CurrentSpeed * DeltaTime;
		CurrentAngle = FMath::UnwindDegrees(CurrentAngle);
		TargetAngle = 0;
	};

	// Regularize framerate
	float FramerateMultiplier = 1.0f;
	bool IsGamepad = GetGameInstance<UNovaGameInstance>()->GetMenuManager()->IsUsingGamepad();
	if (IsGamepad)
	{
		FramerateMultiplier = DeltaTime / (1.0f / 60.0f);
	}

	// Apply filters
	ApplyInputFilter(CurrentPanAngle, CurrentPanSpeed, CurrentPanTarget, IsGamepad ? CameraGamepadVelocity : CameraVelocity);
	ApplyInputFilter(CurrentTiltAngle, CurrentTiltSpeed, CurrentTiltTarget, IsGamepad ? CameraGamepadVelocity : CameraVelocity);

	// Clamp pitch to legal values
	float ExternalCameraPitchClamped = FMath::Clamp(CurrentTiltAngle, CameraMinTilt, CameraMaxTilt);
	if (ExternalCameraPitchClamped != CurrentTiltAngle)
	{
		CurrentTiltSpeed = -CurrentTiltSpeed / 2;
	}
	CurrentTiltAngle = ExternalCameraPitchClamped;

	// Apply values
	CameraYawComponent->SetRelativeRotation(FRotator(0, CurrentPanAngle, 0));
	CameraPitchComponent->SetRelativeRotation(FRotator(CurrentTiltAngle, 0, 0));
}


#undef LOCTEXT_NAMESPACE
