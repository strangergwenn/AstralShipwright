// Nova project - Gwennaël Arbona

#include "NovaSpacecraftPropellantSystem.h"

#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftPropellantSystem::UNovaSpacecraftPropellantSystem()
	: Super()

	, PropellantRate(0)
	, PropellantAmount(0)
{
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    System implementation
----------------------------------------------------*/

void UNovaSpacecraftPropellantSystem::Update(double InitialTime, double FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	const UNovaOrbitalSimulationComponent*  Simulation        = UNovaOrbitalSimulationComponent::Get(this);
	const FNovaSpacecraftPropulsionMetrics* PropulsionMetrics = GetPropulsionMetrics();
	const FGuid&                            Identifier        = GetSpacecraftIdentifier();

	// Process usage between start and end time
	if (Simulation && PropulsionMetrics)
	{
		const FNovaTrajectory* Trajectory         = Simulation->GetSpacecraftTrajectory(Identifier);
		const float            FullPropellantRate = PropulsionMetrics->PropellantRate;
		double                 CurrentTime        = InitialTime;

		PropellantRate = 0;

		if (Trajectory)
		{
			for (const FNovaManeuver& Maneuver : Trajectory->Maneuvers)
			{
				double ManeuverEndTime = FMath::Min(Maneuver.Time + Maneuver.Duration, FinalTime);

				if (CurrentTime >= Maneuver.Time && CurrentTime <= ManeuverEndTime)
				{
					int32 SpacecraftIndex = Simulation->GetSpacecraftTrajectoryIndex(Identifier);
					NCHECK(SpacecraftIndex != INDEX_NONE && SpacecraftIndex >= 0 && SpacecraftIndex < Maneuver.ThrustFactors.Num());
					PropellantRate = FullPropellantRate * Maneuver.ThrustFactors[SpacecraftIndex];

					double DeltaTimeSeconds = (ManeuverEndTime - CurrentTime) * 60;

					PropellantAmount -= PropellantRate * DeltaTimeSeconds;

#if 0
					NLOG("Rate %f, dt %f, current consumed %f", CurrentRate, DeltaTimeSeconds, ConsumedAmount);
#endif

					CurrentTime = ManeuverEndTime;
				}
			}
		}

		NCHECK(PropellantAmount >= 0);
	}
}

void UNovaSpacecraftPropellantSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(UNovaSpacecraftPropellantSystem, PropellantRate);
	DOREPLIFETIME(UNovaSpacecraftPropellantSystem, PropellantAmount);
}
