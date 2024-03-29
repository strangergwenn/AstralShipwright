// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Game/NovaGameState.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

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

	/** Load a system from the spacecraft */
	virtual void Load(const FNovaSpacecraft& Spacecraft)
	{}

	/** Save a system state into the spacecraft */
	virtual void Save(FNovaSpacecraft& Spacecraft)
	{}

	/** Update the simulation between InitialTime and FinalTime */
	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime){};

	/*----------------------------------------------------
	    System helpers
	----------------------------------------------------*/

	/** Get the owning spacecraft's identifier */
	FGuid GetSpacecraftIdentifier() const
	{
		const UActorComponent* ThisComponent = Cast<UActorComponent>(this);
		NCHECK(ThisComponent);
		return ThisComponent->GetOwner<ANovaSpacecraftPawn>()->GetSpacecraftIdentifier();
	}

	/** Get the owning spacecraft */
	const FNovaSpacecraft* GetSpacecraft() const
	{
		const UActorComponent* ThisComponent = Cast<UActorComponent>(this);
		NCHECK(ThisComponent);

		ANovaGameState* GameState = ThisComponent->GetWorld()->GetGameState<ANovaGameState>();

		if (IsValid(GameState))
		{
			return GameState->GetSpacecraft(GetSpacecraftIdentifier());
		}

		return nullptr;
	}

	/** Get the owning spacecraft's propulsion metrics */
	const FNovaSpacecraftPropulsionMetrics* GetPropulsionMetrics() const
	{
		const FNovaSpacecraft* Spacecraft = GetSpacecraft();

		return Spacecraft ? &Spacecraft->GetPropulsionMetrics() : nullptr;
	}

	/** Is the spacecraft docked */
	bool IsSpacecraftDocked() const
	{
		const UActorComponent* ThisComponent = Cast<UActorComponent>(this);
		NCHECK(ThisComponent);
		return ThisComponent->GetOwner<ANovaSpacecraftPawn>()->IsDocked();
	}
};
