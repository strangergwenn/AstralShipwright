// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationTypes.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

float FNovaTrajectory::GetHighestAltitude() const
{
	NCHECK(IsValid());

	float MaximumAltitude = 0;

	for (const FNovaOrbit& Orbit : TransferOrbits)
	{
		MaximumAltitude = FMath::Max(MaximumAltitude, Orbit.GetHighestAltitude());
	}

	return MaximumAltitude;
}

FNovaTimedOrbit FNovaTrajectory::GetFinalOrbit() const
{
	NCHECK(IsValid());

	// Assume the final maneuver is a circularization burn
	FNovaOrbit FinalOrbit    = TransferOrbits[TransferOrbits.Num() - 1];
	FinalOrbit.StartAltitude = FinalOrbit.OppositeAltitude;
	FinalOrbit.StartPhase    = FMath::Fmod(FinalOrbit.EndPhase, 360.0);

	// Assume maneuvers are of zero time
	const FNovaManeuver& FinalManeuver = Maneuvers[Maneuvers.Num() - 1];
	const float&         InsertionTime = FinalManeuver.Time;

	return FNovaTimedOrbit(FinalOrbit, InsertionTime);
}
