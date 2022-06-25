// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"

#include "NovaPlayerStart.generated.h"

/** Player start actor */
UCLASS(ClassGroup = (Nova))
class ANovaPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:

	ANovaPlayerStart(const FObjectInitializer& ObjectInitializer);

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/
public:

#if WITH_EDITOR

	virtual void Tick(float DeltaTime) override;

	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	}

#endif

	/** Get the world location of the dock waiting point */
	FVector GetWaitingPointLocation() const
	{
		return WaitingPoint->GetComponentLocation();
	}

	/** Get the world direction of the area interface point */
	static FVector GetInterfacePointDirection(float DeltaV)
	{
		return (DeltaV > 0 ? 1 : -1) * FVector(0, 1, 0);
	}

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:

	// Waiting point for spacecraft that just left dock or is waiting for docking
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	USceneComponent* WaitingPoint;
};
