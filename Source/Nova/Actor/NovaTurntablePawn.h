// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "NovaTurntablePawn.generated.h"

/** Camera control pawn for turntable menus */
UCLASS(ClassGroup = (Nova))
class ANovaTurntablePawn : public APawn
{
	GENERATED_BODY()

public:
	ANovaTurntablePawn();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	/** Reset the view */
	void ResetView();

	/** Set whether to use the chase mode or not */
	void SetChaseMode(bool Enabled);

	/** Get the current bounds of the actor */
	virtual TPair<FVector, FVector> GetTurntableBounds() const;

	/** Pan (yaw) input */
	void PanInput(float Val);

	/** Tilt (pitch) input */
	void TiltInput(float Val);

	/** Zoom into the actor */
	void ZoomIn();

	/** Zoom out of the actor */
	void ZoomOut();

	/** Reset to the default actor zoom */
	void ResetZoom();

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Apply the pan & tilt inputs to the camera */
	virtual void ProcessCamera(float DeltaTime);

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Time in seconds for camera distance interpolation
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float AnimationDuration;

	// Default distance value
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float DefaultDistance;

	// Distance multiplier (zoom)
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float DefaultDistanceFactor;

	// Distance factor stepping value
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float DistanceFactorIncrement;

	// Number of distance increment steps
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	int32 NumDistanceFactorIncrements;

	// Minimum allowed tilt in degrees (down)
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraMinTilt;

	// Maximum allowed tilt in degrees (up)
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraMaxTilt;

	// Maximum allowed off-axis angle in degrees during chase
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraMaxChaseAngle;

	// Angular velocity of camera in °/s
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraVelocity;

	// Angular velocity of camera in °/s (gamepad)
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraGamepadVelocity;

	// Camera acceleration force in °/s²
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraAcceleration;

	// Camera acceleration force as a multiplier of velocity
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraResistance;

	// Power function applied to inputs
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraInputPower;

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Camera pitch scene container
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USceneComponent* CameraPitchComponent;

	// Camera yaw scene container
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USceneComponent* CameraYawComponent;

	// Camera spring arm
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USpringArmComponent* CameraArmComponent;

	// Actual camera component
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UCameraComponent* Camera;

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Local state
	bool ChaseMode;

	// Input data
	float CurrentPanTarget;
	float CurrentTiltTarget;
	float CurrentPanAngle;
	float CurrentTiltAngle;
	float CurrentPanSpeed;
	float CurrentTiltSpeed;

	// Animation data
	float CurrentOffset;
	float PreviousOffset;
	float CurrentDistance;
	float CurrentDistanceFactor;
	float PreviousDistance;
	float CurrentAnimationTime;
};
