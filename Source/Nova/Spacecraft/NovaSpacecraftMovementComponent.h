// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NovaSpacecraftMovementComponent.generated.h"


/** Spacecraft movement component */
UCLASS(ClassGroup = (Nova))
class UNovaSpacecraftMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UNovaSpacecraftMovementComponent();

	/*----------------------------------------------------
		Movement API
	----------------------------------------------------*/

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:

	// Movement state
	FVector                                       CurrentVelocity;
	FVector                                       CurrentAngularVelocity;
	FVector                                       CurrentAcceleration;
	FVector                                       CurrentAngularAcceleration;


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

	inline const FVector GetMeasuredAcceleration() const
	{
		return CurrentAcceleration;
	}

	inline const FVector GetMeasuredAngularAcceleration() const
	{
		return CurrentAngularAcceleration;
	}

};

