// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"

#include "NovaSpacecraftSystemInterface.generated.h"

/** Interface wrapper */
UINTERFACE(MinimalAPI, Blueprintable)
class UNovaSpacecraftSystemInterface : public UInterface
{
	GENERATED_BODY()
};

/** Base system interface for spacecraft feature components */
class INovaSpacecraftSystemInterface
{
	GENERATED_BODY()

public:
	/*----------------------------------------------------
	    System interface
	----------------------------------------------------*/

	/** Update the simulation between InitialTime and FinalTime */
	virtual void Update(double InitialTime, double FinalTime) = 0;

	/*----------------------------------------------------
	    System helpers
	----------------------------------------------------*/

	/** Get the owning spacecraft's identifier */
	FGuid GetSpacecraftIdentifier() const
	{
		const UActorComponent* ThisComponent = Cast<UActorComponent>(this);
		NCHECK(ThisComponent);
		return Cast<ANovaSpacecraftPawn>(ThisComponent->GetOwner())->GetSpacecraftIdentifier();
	}

	/** Get the owning spacecraft */
	const TSharedPtr<FNovaSpacecraft>& GetSpacecraft() const
	{
		const UActorComponent* ThisComponent = Cast<UActorComponent>(this);
		NCHECK(ThisComponent);
		return Cast<ANovaSpacecraftPawn>(ThisComponent->GetOwner())->GetSpacecraft();
	}

	/** Get the owning spacecraft's propulsion metrics */
	const FNovaSpacecraftPropulsionMetrics* GetPropulsionMetrics() const
	{
		const TSharedPtr<FNovaSpacecraft>& Spacecraft = GetSpacecraft();

		return Spacecraft.IsValid() ? &Spacecraft->GetPropulsionMetrics() : nullptr;
	}
};
