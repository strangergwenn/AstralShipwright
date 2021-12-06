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
	    Inherited
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Ring index
	UPROPERTY(Category = Nova, EditAnywhere)
	int32 RingIndex;

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

	// Socket data
	FVector  SocketRelativeLocation;
	FRotator SocketRelativeRotation;
};
