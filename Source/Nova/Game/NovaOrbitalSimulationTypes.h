// Nova project - Gwennaël Arbona

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
	double GetRadius(float CurrentAltitude) const
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

	FNovaOrbitGeometry(const UNovaCelestialBody* P, float SA, float SP)
		: Body(P), StartAltitude(SA), OppositeAltitude(SA), StartPhase(SP), EndPhase(SP + 360)
	{
		NCHECK(::IsValid(Body));
	}

	FNovaOrbitGeometry(const UNovaCelestialBody* P, float SA, float EA, float SP, float EP)
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
	float GetHighestAltitude() const
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

		const float SemiMajorAxis = 0.5f * (RadiusA + RadiusB);
		return FNovaTime::FromMinutes(2.0 * PI * sqrt(pow(SemiMajorAxis, 3.0) / µ) / 60.0);
	}

	/** Get the current phase on this orbit */
	template <bool Unwind>
	double GetCurrentPhase(FNovaTime DeltaTime) const
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
	float StartAltitude;

	UPROPERTY()
	float OppositeAltitude;

	UPROPERTY()
	float StartPhase;

	UPROPERTY()
	float EndPhase;
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

	/** Get the current phase on this orbit */
	template <bool Unwind>
	double GetCurrentPhase(FNovaTime CurrentTime) const
	{
		return Geometry.GetCurrentPhase<Unwind>(CurrentTime - InsertionTime);
	}

	UPROPERTY()
	FNovaOrbitGeometry Geometry;

	UPROPERTY()
	FNovaTime InsertionTime;
};

/** Current location of a spacecraft or sector, including orbit + phase */
USTRUCT(Atomic)
struct FNovaOrbitalLocation
{
	GENERATED_BODY()

	FNovaOrbitalLocation() : Geometry(), Phase(0)
	{}

	FNovaOrbitalLocation(const FNovaOrbitGeometry& G, float P) : Geometry(G), Phase(P)
	{}

	/** Check for validity */
	bool IsValid() const
	{
		return Geometry.IsValid() && Phase >= 0;
	}

	/** Get the linear distance between this location and another */
	float GetDistanceTo(const FNovaOrbitalLocation& Other) const
	{
		return (GetCartesianLocation() - Other.GetCartesianLocation()).Size();
	}

	/** Get the Cartesian coordinates for this location */
	template <bool AbsolutePosition = true>
	FVector2D GetCartesianLocation() const
	{
		// Extract orbital parameters
		const float SemiMajorAxis     = 0.5f * (Geometry.StartAltitude + Geometry.OppositeAltitude);
		const float SemiMinorAxis     = FMath::Sqrt(Geometry.StartAltitude * Geometry.OppositeAltitude);
		const float Eccentricity      = FMath::Sqrt(1.0f - FMath::Square(SemiMinorAxis) / FMath::Square(SemiMajorAxis));
		const float HalfFocalDistance = SemiMajorAxis * Eccentricity;

		// Process the phase
		float      RelativePhase = -FMath::DegreesToRadians(FMath::UnwindDegrees(Phase - Geometry.StartPhase));
		const bool StartFast     = Geometry.StartAltitude < Geometry.OppositeAltitude;
		if (StartFast)
		{
			RelativePhase = -RelativePhase - PI;
		}

		// Build the Cartesian coordinates
		const float R = (SemiMajorAxis * (1.0f - FMath::Square(Eccentricity))) / (1.0f + Eccentricity * FMath::Cos(RelativePhase));
		const float X = HalfFocalDistance + R * FMath::Cos(RelativePhase);
		const float Y = R * FMath::Sin(RelativePhase);

		// Return the transformed coordinates
		const FVector2D BasePosition = FVector2D(StartFast ? -X : X, Y).GetRotated(-Geometry.StartPhase);
		if (AbsolutePosition)
		{
			const float OriginOffsetSize = SemiMajorAxis - Geometry.OppositeAltitude;
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
	float Phase;
};

/*** Orbit-altering maneuver */
USTRUCT(Atomic)
struct FNovaManeuver
{
	GENERATED_BODY()

	FNovaManeuver() : DeltaV(0), Phase(0)
	{}

	FNovaManeuver(float DV, float P, FNovaTime T, FNovaTime D, const TArray<float>& TF)
		: DeltaV(DV), Phase(P), Time(T), Duration(D), ThrustFactors(TF)
	{}

	UPROPERTY()
	float DeltaV;

	UPROPERTY()
	float Phase;

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

	/** Get the maximum altitude reached by this trajectory */
	float GetHighestAltitude() const;

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

	/** Get the number of remaining maneuvers */
	int32 GetRemainingManeuverCount(FNovaTime CurrentTime) const
	{
		int32 Count = 0;

		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.Time > CurrentTime)
			{
				Count++;
			}
		}

		return Count;
	}

	/** Get the current location in orbit */
	FNovaOrbitalLocation GetCurrentLocation(FNovaTime CurrentTime) const;

	/** Get the current maneuver */
	const FNovaManeuver* GetCurrentManeuver(FNovaTime CurrentTime) const
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

	UPROPERTY() TArray<FNovaOrbit> Transfers;

	UPROPERTY()
	TArray<FNovaManeuver> Maneuvers;

	UPROPERTY()
	FNovaTime TotalTravelDuration;

	UPROPERTY()
	float TotalDeltaV;
};

/*----------------------------------------------------
    Orbit database
----------------------------------------------------*/

/** Replicated orbit information */
USTRUCT()
struct FNovaOrbitDatabaseEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	bool operator==(const FNovaOrbitDatabaseEntry& Other) const
	{
		return Orbit == Other.Orbit && Identifiers == Other.Identifiers;
	}

	UPROPERTY()
	FNovaOrbit Orbit;

	UPROPERTY()
	TArray<FGuid> Identifiers;
};

/** Orbit database with fast array replication and fast lookup */
USTRUCT()
struct FNovaOrbitDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

	bool Add(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaOrbit>& Orbit)
	{
		NCHECK(Orbit.IsValid());
		NCHECK(Orbit->IsValid());

		FNovaOrbitDatabaseEntry TrajectoryData;
		TrajectoryData.Orbit       = *Orbit;
		TrajectoryData.Identifiers = SpacecraftIdentifiers;

		return Cache.Add(*this, Array, TrajectoryData);
	}

	void Remove(const TArray<FGuid>& SpacecraftIdentifiers)
	{
		Cache.Remove(*this, Array, SpacecraftIdentifiers);
	}

	const FNovaOrbit* Get(const FGuid& Identifier) const
	{
		const FNovaOrbitDatabaseEntry* Entry = Cache.Get(Identifier, Array);
		return Entry ? &Entry->Orbit : nullptr;
	}

	void UpdateCache()
	{
		Cache.Update(Array);
	}

	const TArray<FNovaOrbitDatabaseEntry>& Get() const
	{
		return Array;
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaOrbitDatabaseEntry, FNovaOrbitDatabase>(Array, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FNovaOrbitDatabaseEntry> Array;

	TMultiGuidCacheMap<FNovaOrbitDatabaseEntry> Cache;
};

/** Enable fast replication */
template <>
struct TStructOpsTypeTraits<FNovaOrbitDatabase> : public TStructOpsTypeTraitsBase2<FNovaOrbitDatabase>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/*----------------------------------------------------
    Trajectory database
----------------------------------------------------*/

/** Replicated trajectory information */
USTRUCT()
struct FNovaTrajectoryDatabaseEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	bool operator==(const FNovaTrajectoryDatabaseEntry& Other) const
	{
		return Trajectory == Other.Trajectory && Identifiers == Other.Identifiers;
	}

	UPROPERTY()
	FNovaTrajectory Trajectory;

	UPROPERTY()
	TArray<FGuid> Identifiers;
};

/** Trajectory database with fast array replication and fast lookup */
USTRUCT()
struct FNovaTrajectoryDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

	bool Add(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaTrajectory>& Trajectory)
	{
		NCHECK(Trajectory.IsValid());
		NCHECK(Trajectory->IsValid());

		FNovaTrajectoryDatabaseEntry TrajectoryData;
		TrajectoryData.Trajectory  = *Trajectory;
		TrajectoryData.Identifiers = SpacecraftIdentifiers;

		return Cache.Add(*this, Array, TrajectoryData);
	}

	void Remove(const TArray<FGuid>& SpacecraftIdentifiers)
	{
		Cache.Remove(*this, Array, SpacecraftIdentifiers);
	}

	const FNovaTrajectory* Get(const FGuid& Identifier) const
	{
		const FNovaTrajectoryDatabaseEntry* Entry = Cache.Get(Identifier, Array);
		return Entry ? &Entry->Trajectory : nullptr;
	}

	int32 GetSpacecraftIndex(const FGuid& Identifier) const
	{
		const FNovaTrajectoryDatabaseEntry* Entry = Cache.Get(Identifier, Array);

		if (Entry)
		{
			for (int32 Index = 0; Index < Entry->Identifiers.Num(); Index++)
			{
				if (Entry->Identifiers[Index] == Identifier)
				{
					return Index;
				}
			}
		}

		return INDEX_NONE;
	}

	void UpdateCache()
	{
		Cache.Update(Array);
	}

	const TArray<FNovaTrajectoryDatabaseEntry>& Get() const
	{
		return Array;
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaTrajectoryDatabaseEntry, FNovaTrajectoryDatabase>(
			Array, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FNovaTrajectoryDatabaseEntry> Array;

	TMultiGuidCacheMap<FNovaTrajectoryDatabaseEntry> Cache;
};

/** Enable fast replication */
template <>
struct TStructOpsTypeTraits<FNovaTrajectoryDatabase> : public TStructOpsTypeTraitsBase2<FNovaTrajectoryDatabase>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
