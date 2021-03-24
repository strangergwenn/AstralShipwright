// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationTypes.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

float FNovaTrajectory::GetHighestAltitude() const
{
	NCHECK(IsValid());

	float MaximumAltitude = 0;

	for (const FNovaOrbit& Transfer : Transfers)
	{
		MaximumAltitude = FMath::Max(MaximumAltitude, Transfer.Geometry.GetHighestAltitude());
	}

	return MaximumAltitude;
}

FNovaOrbit FNovaTrajectory::GetFinalOrbit() const
{
	NCHECK(IsValid());

	// Assume the final maneuver is a circularization burn
	FNovaOrbitGeometry FinalGeometry = Transfers[Transfers.Num() - 1].Geometry;
	FinalGeometry.StartAltitude      = FinalGeometry.OppositeAltitude;
	FinalGeometry.StartPhase         = FMath::Fmod(FinalGeometry.EndPhase, 360.0);

	return FNovaOrbit(FinalGeometry, GetArrivalTime());
}

FNovaOrbitalLocation FNovaTrajectory::GetCurrentLocation(double CurrentTime) const
{
	FNovaOrbit LastValidTransfer;

	for (const FNovaOrbit& Transfer : Transfers)
	{
		if (Transfer.InsertionTime <= CurrentTime)
		{
			LastValidTransfer = Transfer;
		}
	}

	if (LastValidTransfer.IsValid())
	{
		return FNovaOrbitalLocation(LastValidTransfer.Geometry, LastValidTransfer.GetCurrentPhase<false>(CurrentTime));
	}
	else
	{
		return FNovaOrbitalLocation();
	}
}

TArray<FNovaOrbit> FNovaTrajectory::GetRelevantOrbitsForManeuver(const FNovaManeuver& Maneuver) const
{
	FNovaOrbit OriginTransfer;
	FNovaOrbit CurrentTransfer;

	for (const FNovaOrbit& Transfer : Transfers)
	{
		if (Transfer.InsertionTime <= Maneuver.Time)
		{
			OriginTransfer = Transfer;
		}
		else if (Transfer.InsertionTime <= Maneuver.Time + Maneuver.Duration)
		{
			CurrentTransfer = Transfer;
		}
	}

	TArray<FNovaOrbit> Result;
	if (OriginTransfer.IsValid())
	{
		Result.Add(OriginTransfer);
	}
	if (CurrentTransfer.IsValid())
	{
		Result.Add(CurrentTransfer);
	}

	return Result;
}
