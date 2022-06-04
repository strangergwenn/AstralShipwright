// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Neutron/Actor/NeutronStaticMeshComponent.h"

#include "NovaStationDockComponent.generated.h"

/** Station dock component that tracks spacecraft hatches as part of a station ring */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaStationDockComponent : public UNeutronStaticMeshComponent
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

	// Size of the collision box around cameras in units
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float CameraBoxExtent;

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Socket data
	FVector SocketRelativeLocation;

	// Animation data
	double CurrentLinearVelocity;
};
