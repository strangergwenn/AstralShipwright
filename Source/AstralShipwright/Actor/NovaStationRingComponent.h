// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "NovaStationRingComponent.generated.h"

/** Station ring component that tracks spacecraft */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaStationRingComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UNovaStationRingComponent();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Check if the ring system is currently active */
	bool IsOperating() const;

	/** Get the target spacecraft */
	const class ANovaSpacecraftPawn* GetCurrentSpacecraft() const
	{
		return AttachedSpacecraft;
	}

	/** Get the target component */
	const class USceneComponent* GetCurrentTarget() const
	{
		return TargetComponent;
	}

	/** Is the current target a hatch ? Necessary because hatch behavior is a secondary component */
	bool IsCurrentTargetHatch() const
	{
		return TargetComponentIsHatch;
	}

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Ring index
	UPROPERTY(Category = Nova, EditAnywhere)
	int32 RingIndex;

	// Distance under which we consider stopped
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float LinearDeadDistance;

	// Maximum moving velocity in m/s
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaxLinearVelocity;

	// Maximum moving acceleration in m/s-2
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float LinearAcceleration;

	// Distance under which we consider stopped
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float AngularDeadDistance;

	// Maximum turn rate in °/s (pitch & roll)
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaxAngularVelocity;

	// Maximum turn acceleration in °/s-2
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float AngularAcceleration;

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	UPROPERTY()
	const class ANovaPlayerStart* AttachedPlayerStart;

	UPROPERTY()
	const class ANovaSpacecraftPawn* AttachedSpacecraft;

	UPROPERTY()
	const class UNovaStationRingComponent* PreviousRing;

	UPROPERTY()
	const class USceneComponent* TargetComponent;

	// Socket data
	FVector  SocketRelativeLocation;
	FRotator SocketRelativeRotation;

	// Animation data
	double CurrentLinearVelocity;
	double CurrentRollVelocity;
	bool   TargetComponentIsHatch;
};
