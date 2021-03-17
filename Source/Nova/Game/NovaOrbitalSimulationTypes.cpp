// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationTypes.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

float FNovaTrajectory::GetHighestAltitude() const
{
	NCHECK(IsValid());

	float MaximumAltitude = 0;

	for (const FNovaOrbitGeometry& Geometry : TransferOrbits)
	{
		MaximumAltitude = FMath::Max(MaximumAltitude, Geometry.GetHighestAltitude());
	}

	return MaximumAltitude;
}

FNovaOrbit FNovaTrajectory::GetFinalOrbit() const
{
	NCHECK(IsValid());

	// Assume the final maneuver is a circularization burn
	FNovaOrbitGeometry FinalGeometry = TransferOrbits[TransferOrbits.Num() - 1];
	FinalGeometry.StartAltitude      = FinalGeometry.OppositeAltitude;
	FinalGeometry.StartPhase         = FMath::Fmod(FinalGeometry.EndPhase, 360.0);

	// Assume maneuvers are of zero time
	const FNovaManeuver& FinalManeuver = Maneuvers[Maneuvers.Num() - 1];
	const float&         InsertionTime = FinalManeuver.Time;

	return FNovaOrbit(FinalGeometry, InsertionTime);
}

FNovaOrbitalLocation FNovaTrajectory::GetCurrentLocation(double CurrentTime) const
{
	const FNovaManeuver*      Maneuver = nullptr;
	const FNovaOrbitGeometry* Geometry = nullptr;

	// Find the current maneuver
	for (int32 ManeuverIndex = 0; ManeuverIndex < Maneuvers.Num() - 1; ManeuverIndex++)
	{
		if (CurrentTime >= Maneuvers[ManeuverIndex].Time && CurrentTime <= Maneuvers[ManeuverIndex + 1].Time)
		{
			Maneuver = &Maneuvers[ManeuverIndex];
		}
	}

	// Find the current transfer orbit
	if (Maneuver)
	{
		for (const FNovaOrbitGeometry& Transfer : TransferOrbits)
		{
			if (Transfer.StartPhase == Maneuver->Phase)
			{
				Geometry = &Transfer;
			}
		}

		// Compute the current location
		if (Geometry)
		{
			return FNovaOrbitalLocation(*Geometry, Geometry->GetCurrentPhase<false>(CurrentTime - Maneuver->Time));
		}
	}

	return FNovaOrbitalLocation();
}
