// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaGameTypes.h"

#include "NovaOrbitalSimulationTypes.generated.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

/** Large number type for planetary values */
USTRUCT()
struct FNovaLargeNumber
{
	GENERATED_BODY()

	double GetValue() const
	{
		return static_cast<double>(Value) * FMath::Pow(10, static_cast<double>(Exponent));
	}

	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Value;

	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Exponent;
};

/** Planet description */
UCLASS(ClassGroup = (Nova))
class UNovaCelestialBody : public UNovaAssetDescription
{
	GENERATED_BODY()

public:

	/** Get the mass of the planet in kg */
	double GetMass() const
	{
		return Mass.GetValue();
	}

	/** Get the gravitational parameter in m3/s² */
	double GetGravitationalParameter() const
	{
		constexpr double GravitationalConstant = 6.674e-11;
		return GravitationalConstant * GetMass();
	}

	/** Get a radius value in meters from altitude above ground in km */
	double GetRadius(double CurrentAltitude) const
	{
		return (static_cast<double>(Radius) + static_cast<double>(CurrentAltitude)) * 1000.0;
	}

public:

	// Orbital map brush
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FSlateBrush Image;

	// Body radius in km
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Radius;

	// Mass in kg with exponent stored separately
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FNovaLargeNumber Mass;

	// Body orbited
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const class UNovaCelestialBody* Body;

	// Altitude in kilometers
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FNovaLargeNumber Altitude;

	// Initial phase on the orbit in degrees
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Phase;

	// Rotation period in minutes
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float RotationPeriod;

	// Name to look for in the planetarium
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FName PlanetariumName;
};

/** Data for a stable orbit that might be a circular, elliptical or Hohmann transfer orbit */
USTRUCT(Atomic)
struct FNovaOrbitGeometry
{
	GENERATED_BODY()

	FNovaOrbitGeometry() : Body(nullptr), StartAltitude(0), OppositeAltitude(0), StartPhase(0), EndPhase(0)
	{}

	FNovaOrbitGeometry(const UNovaCelestialBody* P, double SA, double SP)
		: Body(P), StartAltitude(SA), OppositeAltitude(SA), StartPhase(SP), EndPhase(SP + 360)
	{
		NCHECK(::IsValid(Body));
	}

	FNovaOrbitGeometry(const UNovaCelestialBody* P, double SA, double EA, double SP, double EP)
		: Body(P), StartAltitude(SA), OppositeAltitude(EA), StartPhase(SP), EndPhase(EP)
	{
		NCHECK(::IsValid(Body));
	}

	bool operator==(const FNovaOrbitGeometry& Other) const
	{
		return StartAltitude == Other.StartAltitude && OppositeAltitude == Other.OppositeAltitude && StartPhase == Other.StartPhase &&
		       EndPhase == Other.EndPhase;
	}

	bool operator!=(const FNovaOrbitGeometry& Other) const
	{
		return !operator==(Other);
	}

	/** Check for validity */
	bool IsValid() const
	{
		return ::IsValid(Body) && StartAltitude > 0 && OppositeAltitude > 0 && StartPhase >= 0 && EndPhase > 0 && StartPhase < EndPhase;
	}

	/** Check whether this geometry is circular */
	bool IsCircular() const
	{
		return StartAltitude == OppositeAltitude;
	}

	/** Get the maximum altitude reached by this orbit */
	double GetHighestAltitude() const
	{
		return FMath::Max(StartAltitude, OppositeAltitude);
	}

	/** Compute the period of this orbit geometry */
	FNovaTime GetOrbitalPeriod() const
	{
		NCHECK(::IsValid(Body));
		const double RadiusA = Body->GetRadius(StartAltitude);
		const double RadiusB = Body->GetRadius(OppositeAltitude);
		const double µ       = Body->GetGravitationalParameter();

		const double SemiMajorAxis = 0.5f * (RadiusA + RadiusB);
		return FNovaTime::FromMinutes(2.0 * PI * sqrt(pow(SemiMajorAxis, 3.0) / µ) / 60.0);
	}

	/** Get the current phase on this orbit */
	template <bool Unwind>
	double GetPhase(FNovaTime DeltaTime) const
	{
		NCHECK(Body);
		const double PhaseDelta = (DeltaTime / GetOrbitalPeriod()) * 360;
		double       Result     = StartPhase + (Unwind ? FMath::Fmod(PhaseDelta, 360.0) : PhaseDelta);

		if (Unwind)
		{
			NCHECK(Result < StartPhase + 360);
		}

		return Result;
	}

	UPROPERTY()
	const UNovaCelestialBody* Body;

	UPROPERTY()
	double StartAltitude;

	UPROPERTY()
	double OppositeAltitude;

	UPROPERTY()
	double StartPhase;

	UPROPERTY()
	double EndPhase;
};

/** Current location of a spacecraft or sector, including orbit + phase */
USTRUCT(Atomic)
struct FNovaOrbitalLocation
{
	GENERATED_BODY()

	FNovaOrbitalLocation() : Geometry(), Phase(0)
	{}

	FNovaOrbitalLocation(const FNovaOrbitGeometry& G, double P) : Geometry(G), Phase(P)
	{}

	/** Check for validity */
	bool IsValid() const
	{
		return Geometry.IsValid() && Phase >= 0;
	}

	/** Get the linear distance between this location and another in km */
	double GetDistanceTo(const FNovaOrbitalLocation& Other) const
	{
		return (GetCartesianLocation() - Other.GetCartesianLocation()).Size();
	}

	/** Get the current orbital velocity in m/s */
	FVector2D GetOrbitalVelocity() const
	{
		NCHECK(::IsValid(Geometry.Body));
		const double RadiusA       = Geometry.Body->GetRadius(Geometry.StartAltitude);
		const double RadiusB       = Geometry.Body->GetRadius(Geometry.OppositeAltitude);
		const double µ             = Geometry.Body->GetGravitationalParameter();
		const double SemiMajorAxis = 0.5f * (RadiusA + RadiusB);

		const FVector2D CartesianLocation = GetCartesianLocation();

		return FMath::Sqrt(µ * (2.0 / (CartesianLocation.Size() * 1000.0) - 1.0 / SemiMajorAxis)) *
		       CartesianLocation.GetSafeNormal().GetRotated(90.0);
	}

	/** Get the Cartesian coordinates for this location in km */
	template <bool AbsolutePosition = true>
	FVector2D GetCartesianLocation(double BaseAltitude = 0) const
	{
		if (BaseAltitude == 0)
		{
			BaseAltitude = Geometry.Body->Radius;
		}

		// Extract orbital parameters
		const double SemiMajorAxis     = 0.5 * (2.0 * BaseAltitude + Geometry.StartAltitude + Geometry.OppositeAltitude);
		const double SemiMinorAxis     = FMath::Sqrt((BaseAltitude + Geometry.StartAltitude) * (BaseAltitude + Geometry.OppositeAltitude));
		const double Eccentricity      = FMath::Sqrt(1.0 - FMath::Square(SemiMinorAxis) / FMath::Square(SemiMajorAxis));
		const double HalfFocalDistance = SemiMajorAxis * Eccentricity;

		// Process the phase
		double     RelativePhase = -FMath::DegreesToRadians(FMath::UnwindDegrees(Phase - Geometry.StartPhase));
		const bool StartFast     = Geometry.StartAltitude < Geometry.OppositeAltitude;
		if (StartFast)
		{
			RelativePhase = -RelativePhase - PI;
		}

		// Build the Cartesian coordinates
		const double R = (SemiMajorAxis * (1.0 - FMath::Square(Eccentricity))) / (1.0 + Eccentricity * FMath::Cos(RelativePhase));
		const double X = HalfFocalDistance + R * FMath::Cos(RelativePhase);
		const double Y = R * FMath::Sin(RelativePhase);

		// Return the transformed coordinates
		const FVector2D BasePosition = FVector2D(StartFast ? -X : X, Y).GetRotated(-Geometry.StartPhase);
		if (AbsolutePosition)
		{
			const double OriginOffsetSize = SemiMajorAxis - Geometry.OppositeAltitude - BaseAltitude;
			return BasePosition + FVector2D(OriginOffsetSize, 0).GetRotated(-Geometry.StartPhase);
		}
		else
		{
			return BasePosition;
		}
	}

	UPROPERTY()
	FNovaOrbitGeometry Geometry;

	UPROPERTY()
	double Phase;
};

/** Orbit + time of insertion, allowing prediction of where a spacecraft will be at any time */
USTRUCT(Atomic)
struct FNovaOrbit
{
	GENERATED_BODY()

	FNovaOrbit() : Geometry()
	{}

	FNovaOrbit(const FNovaOrbitGeometry& G, FNovaTime T) : Geometry(G), InsertionTime(T)
	{
		NCHECK(Geometry.Body != nullptr);
	}

	bool operator==(const FNovaOrbit& Other) const
	{
		return Geometry == Other.Geometry && InsertionTime == Other.InsertionTime;
	}

	bool operator!=(const FNovaOrbit& Other) const
	{
		return !operator==(Other);
	}

	/** Check for validity */
	bool IsValid() const
	{
		return Geometry.IsValid() && InsertionTime.IsValid();
	}

	/** Get the current phase on this orbit in degrees */
	template <bool Unwind>
	double GetPhase(FNovaTime CurrentTime) const
	{
		return Geometry.GetPhase<Unwind>(CurrentTime - InsertionTime);
	}

	/** Get the full location on this orbit */
	FNovaOrbitalLocation GetLocation(FNovaTime CurrentTime) const
	{
		double CurrentPhase = GetPhase<true>(CurrentTime);
		return FNovaOrbitalLocation(Geometry, CurrentPhase);
	}

	UPROPERTY()
	FNovaOrbitGeometry Geometry;

	UPROPERTY()
	FNovaTime InsertionTime;
};

/*** Orbit-altering maneuver */
USTRUCT(Atomic)
struct FNovaManeuver
{
	GENERATED_BODY()

	FNovaManeuver() : DeltaV(0), Phase(0)
	{}

	FNovaManeuver(double DV, double P, FNovaTime T, FNovaTime D, const TArray<float>& TF)
		: DeltaV(DV), Phase(P), Time(T), Duration(D), ThrustFactors(TF)
	{}

	UPROPERTY()
	double DeltaV;

	UPROPERTY()
	double Phase;

	UPROPERTY()
	FNovaTime Time;

	UPROPERTY()
	FNovaTime Duration;

	UPROPERTY()
	TArray<float> ThrustFactors;
};

/** Full trajectory data including the last stable orbit */
USTRUCT()
struct FNovaTrajectory
{
	GENERATED_BODY()

	FNovaTrajectory() : TotalDeltaV(0)
	{}

	bool operator==(const FNovaTrajectory& Other) const
	{
		return Transfers.Num() == Other.Transfers.Num() && Maneuvers.Num() == Other.Maneuvers.Num() &&
		       TotalTravelDuration == Other.TotalTravelDuration && TotalDeltaV == Other.TotalDeltaV;
	}

	bool operator!=(const FNovaTrajectory& Other) const
	{
		return !operator==(Other);
	}

	/** Check for validity */
	bool IsValid() const
	{
		if (Transfers.Num() == 0 || Maneuvers.Num() == 0)
		{
			return false;
		}

		for (const FNovaOrbit& Transfer : Transfers)
		{
			if (!Transfer.IsValid())
			{
				return false;
			}
		}

		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.DeltaV == 0)
			{
				return false;
			}
		}

		return true;
	}

	/** Check for validity and a moderate travel time */
	bool IsValidExtended() const
	{
		return IsValid() && FMath::IsFinite(TotalDeltaV) &&
		       TotalTravelDuration < FNovaTime::FromDays(ENovaConstants::MaxTrajectoryDurationDays);
	}

	/** Add a maneuver if it's not zero delta-v */
	bool Add(const FNovaManeuver& Maneuver)
	{
		if (Maneuver.DeltaV != 0)
		{
			Maneuvers.Add(Maneuver);
			return true;
		}
		return false;
	}

	/** Add a transfer if it's not zero angular length */
	bool Add(const FNovaOrbit& Orbit)
	{
		if (Orbit.IsValid())
		{
			Transfers.Add(Orbit);
			return true;
		}
		return false;
	}

	/** Get the maximum altitude reached by this trajectory in km  */
	double GetHighestAltitude() const;

	/** Compute the final orbit this trajectory will put the spacecraft in */
	FNovaOrbit GetFinalOrbit() const;

	/** Get the start time */
	FNovaTime GetStartTime() const
	{
		NCHECK(IsValid());
		return GetArrivalTime() - TotalTravelDuration;
	}

	/** Get the start time of the first maneuver */
	FNovaTime GetFirstManeuverStartTime() const
	{
		NCHECK(IsValid());
		return Maneuvers[0].Time;
	}

	/** Get the start time of the next maneuver */
	FNovaTime GetNextManeuverStartTime(FNovaTime CurrentTime) const
	{
		const FNovaManeuver* Maneuver = GetNextManeuver(CurrentTime);
		return Maneuver ? Maneuver->Time : FNovaTime();
	}

	/** Get the arrival time */
	FNovaTime GetArrivalTime() const
	{
		const FNovaManeuver& FinalManeuver = Maneuvers[Maneuvers.Num() - 1];
		return FinalManeuver.Time + FinalManeuver.Duration;
	}

	/** Get how many tons of propellant will be used */
	double GetTotalPropellantUsed(int32 SpacecraftIndex, const struct FNovaSpacecraftPropulsionMetrics& Metrics) const;

	/** Get the number of remaining maneuvers */
	int32 GetRemainingManeuverCount(FNovaTime CurrentTime) const
	{
		int32 Count = 0;

		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.Time >= CurrentTime)
			{
				Count++;
			}
		}

		return Count;
	}

	/** Get the location in orbit at the current time in km */
	FNovaOrbitalLocation GetLocation(FNovaTime CurrentTime) const;

	/** Get the Cartesian location in orbit at the current time including maneuver smoothing in km */
	FVector2D GetCartesianLocation(FNovaTime CurrentTime) const;

	/** Get the maneuver at the current time */
	const FNovaManeuver* GetManeuver(FNovaTime CurrentTime) const
	{
		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.Time <= CurrentTime && CurrentTime <= Maneuver.Time + Maneuver.Duration)
			{
				return &Maneuver;
			}
		}

		return nullptr;
	}

	/** Get the previous maneuver */
	const FNovaManeuver* GetPreviousManeuver(FNovaTime CurrentTime) const
	{
		const FNovaManeuver* PreviousManeuver = nullptr;

		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.Time > CurrentTime)
			{
				return PreviousManeuver;
			}
			else
			{
				PreviousManeuver = &Maneuver;
			}
		}

		return PreviousManeuver;
	}

	/** Get the next maneuver */
	const FNovaManeuver* GetNextManeuver(FNovaTime CurrentTime) const
	{
		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.Time > CurrentTime)
			{
				return &Maneuver;
			}
		}

		return nullptr;
	}

	/** Get the orbits that a maneuver is going from and to */
	TArray<FNovaOrbit> GetRelevantOrbitsForManeuver(const FNovaManeuver& Maneuver) const;

	UPROPERTY()
	FNovaOrbit InitialOrbit;

	UPROPERTY()
	TArray<FNovaOrbit> Transfers;

	UPROPERTY()
	TArray<FNovaManeuver> Maneuvers;

	UPROPERTY()
	FNovaTime TotalTravelDuration;

	UPROPERTY()
	double TotalDeltaV;
};
