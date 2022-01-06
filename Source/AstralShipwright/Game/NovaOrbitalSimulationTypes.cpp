// Astral Shipwright - Gwennaël Arbona

#include "NovaOrbitalSimulationTypes.h"
#include "Spacecraft/NovaSpacecraft.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

double FNovaTrajectory::GetHighestAltitude() const
{
	NCHECK(IsValid());

	double MaximumAltitude = 0;

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
	FinalGeometry.StartPhase         = FMath::Fmod(FinalGeometry.EndPhase, 360.0f);

	return FNovaOrbit(FinalGeometry, GetArrivalTime());
}

double FNovaTrajectory::GetTotalPropellantUsed(int32 SpacecraftIndex, const FNovaSpacecraftPropulsionMetrics& Metrics) const
{
	double PropellantUsed = 0;

	for (const FNovaManeuver& Maneuver : Maneuvers)
	{
		NCHECK(SpacecraftIndex >= 0 && SpacecraftIndex < Maneuver.ThrustFactors.Num());
		PropellantUsed += Maneuver.Duration.AsSeconds() * Metrics.PropellantRate * Maneuver.ThrustFactors[SpacecraftIndex];
	}

	return PropellantUsed;
}

FNovaOrbitalLocation FNovaTrajectory::GetLocation(FNovaTime CurrentTime) const
{
	FNovaOrbit CurrentTransfer = InitialOrbit;
	for (const FNovaOrbit& Transfer : Transfers)
	{
		if (Transfer.InsertionTime <= CurrentTime)
		{
			CurrentTransfer = Transfer;
		}
	}

	return CurrentTransfer.IsValid() ? CurrentTransfer.GetLocation(CurrentTime) : FNovaOrbitalLocation();
}

FVector2D FNovaTrajectory::GetCartesianLocation(FNovaTime CurrentTime) const
{
	FNovaOrbit PreviousTransfer;
	FNovaOrbit CurrentTransfer       = InitialOrbit;
	bool       HasFoundValidTransfer = false;
	bool       IsOnLastTransfer      = false;

	// Find the current transfer
	for (int32 TransferIndex = 0; TransferIndex < Transfers.Num(); TransferIndex++)
	{
		const FNovaOrbit& Transfer = Transfers[TransferIndex];

		if (Transfer.InsertionTime <= CurrentTime)
		{
			PreviousTransfer      = CurrentTransfer;
			CurrentTransfer       = Transfer;
			HasFoundValidTransfer = true;

			if (TransferIndex == Transfers.Num() - 1)
			{
				IsOnLastTransfer = true;
			}
		}
	}

	if (HasFoundValidTransfer)
	{
		FNovaOrbitalLocation CurrentSimulatedLocation = CurrentTransfer.GetLocation(CurrentTime);
		const FNovaManeuver* CurrentManeuver          = GetManeuver(CurrentTime);

		// Trajectories are computed as a series of transfers with instantaneous maneuvers between them.
		// This is accurate enough for simulation, but is not acceptable for movement, which this method is responsible for.
		// As a result, physical acceleration is injected here after the fact by blending the simulation with the previous orbit.
		if (CurrentManeuver)
		{
			FVector2D StartLocation;
			FVector2D EndLocation;

			if (IsOnLastTransfer)
			{
				StartLocation = CurrentSimulatedLocation.GetCartesianLocation();
				EndLocation   = GetFinalOrbit().GetLocation(CurrentTime).GetCartesianLocation();
			}
			else
			{
				StartLocation = PreviousTransfer.GetLocation(CurrentTime).GetCartesianLocation();
				EndLocation   = CurrentSimulatedLocation.GetCartesianLocation();
			}

			const double Alpha = (CurrentTime - CurrentManeuver->Time) / CurrentManeuver->Duration;
			return FMath::Lerp(StartLocation, EndLocation, Alpha);
		}
		else
		{
			return CurrentSimulatedLocation.GetCartesianLocation();
		}
	}

	return InitialOrbit.GetLocation(CurrentTime).GetCartesianLocation();
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
