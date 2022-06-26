// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftPropellantSystem.h"

#include "Game/NovaOrbitalSimulationComponent.h"

#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftPropellantSystem::UNovaSpacecraftPropellantSystem()
	: Super()

	, InitialPropellantMass(0)
	, PropellantMass(0)
	, PropellantRate(0)
{
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    System implementation
----------------------------------------------------*/

void UNovaSpacecraftPropellantSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	const UNovaOrbitalSimulationComponent*  Simulation        = UNovaOrbitalSimulationComponent::Get(this);
	const FNovaSpacecraftPropulsionMetrics* PropulsionMetrics = GetPropulsionMetrics();
	const FGuid&                            Identifier        = GetSpacecraftIdentifier();

	// Process usage between start and end time
	if (Simulation && PropulsionMetrics)
	{
		PropellantRate = 0;

		const FNovaTrajectory* Trajectory = Simulation->GetSpacecraftTrajectory(Identifier);
		if (Trajectory)
		{
			PropellantMass = InitialPropellantMass;

			for (const FNovaManeuver& Maneuver : Trajectory->Maneuvers)
			{
				FNovaTime ManeuverEndTime          = FMath::Min(Maneuver.Time + Maneuver.Duration, FinalTime);
				FNovaTime AdjustedManeuverDuration = ManeuverEndTime - Maneuver.Time;
				double    DeltaTimeSeconds         = AdjustedManeuverDuration.AsSeconds();

				if (DeltaTimeSeconds > 0)
				{
					int32 SpacecraftIndex = Simulation->GetSpacecraftTrajectoryIndex(Identifier);
					NCHECK(SpacecraftIndex != INDEX_NONE && SpacecraftIndex >= 0 && SpacecraftIndex < Maneuver.ThrustFactors.Num());

					PropellantRate = PropulsionMetrics->PropellantRate * Maneuver.ThrustFactors[SpacecraftIndex];
					PropellantMass -= PropellantRate * DeltaTimeSeconds;
				}
			}
#if 0
			NLOG("PropellantRate %f, InitialPropellantMass %f, PropellantMass %f", PropellantRate, InitialPropellantMass, PropellantMass);
#endif
		}

		NCHECK(PropellantMass >= 0);
	}
}

void UNovaSpacecraftPropellantSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftPropellantSystem, InitialPropellantMass);
	DOREPLIFETIME(UNovaSpacecraftPropellantSystem, PropellantRate);
	DOREPLIFETIME(UNovaSpacecraftPropellantSystem, PropellantMass);
}
