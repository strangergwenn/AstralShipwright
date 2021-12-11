// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaStaticMeshComponent.h"

#include "NovaStationDockComponent.generated.h"

/** Station dock component that tracks spacecraft hatches as part of a station ring */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaStationDockComponent : public UNovaStaticMeshComponent
{
	GENERATED_BODY()

public:
	UNovaStationDockComponent();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Distance under which we consider stopped
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float LinearDeadDistance;

	// Maximum moving velocity in m/s
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaxLinearVelocity;

	// Maximum moving acceleration in m/s-2
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float LinearAcceleration;

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Socket data
	FVector SocketRelativeLocation;

	// Animation data
	double CurrentLinearVelocity;
};
