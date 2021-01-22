// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "NovaSpacecraftMovementComponent.generated.h"


/** Spacecraft movement component */
UCLASS(ClassGroup = (Nova))
class UNovaSpacecraftMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:

	UNovaSpacecraftMovementComponent();

	/*----------------------------------------------------
		Movement API
	----------------------------------------------------*/

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/*----------------------------------------------------
		Internal movement implementation
	----------------------------------------------------*/

protected:

	/** Process overall movement */
	virtual void ApplyMovement(float DeltaTime);

	/** Apply hit effects */
	virtual void OnHit(const FHitResult& Hit, const FVector& HitVelocity);


	/*----------------------------------------------------
		Properties
	----------------------------------------------------*/
	
public:

	// Maximum linear acceleration rate in m/s²
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float LinearAcceleration;

	// Maximum axis turn acceleration rate in °/s²
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularAcceleration;

	// Slope multiplier on the angular target
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularControlIntensity;

	// Maximum moving velocity in m/s
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float MaxLinearVelocity;

	// Maximum turn rate in °/s (pitch & roll)
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float MaxAngularVelocity;

	// Base restitution coefficient of hits
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float RestitutionCoefficient;

	// Collision shake
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	TSubclassOf<class UCameraShakeBase> HitShake;


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:

	// Movement state
	FVector                                       CurrentDesiredAcceleration;
	FQuat                                         CurrentDesiredRotation;
	FVector                                       CurrentVelocity;
	FVector                                       CurrentAngularVelocity;

	// Historical data
	FVector                                       PreviousVelocity;
	FVector                                       PreviousAngularVelocity;
	FVector                                       MeasuredAcceleration;
	FVector                                       MeasuredAngularAcceleration;


	/*----------------------------------------------------
		Getters
	----------------------------------------------------*/

public:

	inline const FVector GetCurrentVelocity() const
	{
		return CurrentVelocity;
	}

	inline const FVector GetCurrentAngularVelocity() const
	{
		return CurrentAngularVelocity;
	}

	inline const FVector GetThrusterAcceleration() const
	{
		return MeasuredAcceleration;
	}

	inline const FVector GetThrusterAngularAcceleration() const
	{
		return MeasuredAngularAcceleration;
	}

	inline const float GetMainDriveAcceleration() const
	{
		return 0.0f;
	}

};

