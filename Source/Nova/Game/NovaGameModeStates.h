// Nova project - Gwennaël Arbona

#include "NovaGameMode.h"

#include "NovaArea.h"
#include "NovaGameWorld.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Nova.h"

/*----------------------------------------------------
    Constants
----------------------------------------------------*/

// Cutscene timing values in seconds
constexpr float DepartureCutsceneDelay    = 5.0;
constexpr float DepartureCutsceneDuration = 5.0;
constexpr float AreaIntroductionDuration  = 5.0;
constexpr float ArrivalCutsceneDuration   = 10.0;

/*----------------------------------------------------
    Game state class
----------------------------------------------------*/

/** Game state */
class FNovaGameState
{
public:
	FNovaGameState() : PC(nullptr), GameWorld(nullptr), OrbitalSimulationComponent(nullptr)
	{}

	virtual ~FNovaGameState()
	{}

	/** Set the required work data */
	void Initialize(const FString& Name, class ANovaPlayerController* P, class ANovaGameMode* GM, class ANovaGameWorld* GW,
		class UNovaOrbitalSimulationComponent* OSC)
	{
		StateName                  = Name;
		PC                         = P;
		GameMode                   = GM;
		GameWorld                  = GW;
		OrbitalSimulationComponent = OSC;
	}

	/** Get the time spent in this state in minutes */
	double GetMinutesInState() const
	{
		return GameWorld->GetCurrentTime() - StateStartTime;
	}

	/** Get the time spent in this state in seconds */
	double GetSecondsInState() const
	{
		return (GameWorld->GetCurrentTime() - StateStartTime) * 60.0;
	}

	/** Enter this state from a previous one */
	virtual void EnterState(ENovaGameStateIdentifier PreviousStateIdentifier)
	{
		NLOG("FNovaGameState::EnterState : entering state '%s'", *StateName);
		StateStartTime = GameWorld->GetCurrentTime();
	}

	/** Run the state and return the desired current state */
	virtual ENovaGameStateIdentifier UpdateState()
	{
		return static_cast<ENovaGameStateIdentifier>(-1);
	}

	/** Prepare to leave this state for a new one */
	virtual void LeaveState(ENovaGameStateIdentifier NewStateIdentifier)
	{}

protected:
	// Local state
	FString StateName;
	double  StateStartTime;

	// Outer objects
	class ANovaPlayerController*           PC;
	class ANovaGameMode*                   GameMode;
	class ANovaGameWorld*                  GameWorld;
	class UNovaOrbitalSimulationComponent* OrbitalSimulationComponent;
};

/*----------------------------------------------------
    Idle states
----------------------------------------------------*/

// Area state : operating locally within an area
class FNovaAreaState : public FNovaGameState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default);
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();

		if (PlayerTrajectory &&
			PlayerTrajectory->GetFirstManeuverStartTime() <= GameWorld->GetCurrentTime() + DepartureCutsceneDelay / 60.0)
		{
			return ENovaGameStateIdentifier::DepartureProximity;
		}
		else
		{
			return ENovaGameStateIdentifier::Area;
		}
	}
};

// Orbit state : operating locally in empty space
class FNovaOrbitState : public FNovaGameState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameMode->ChangeAreaToOrbit();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameState::UpdateState();

		if (OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver() <= 0 && OrbitalSimulationComponent->IsPlayerNearingLastManeuver())
		{
			return ENovaGameStateIdentifier::ArrivalIntro;
		}
		else
		{
			return ENovaGameStateIdentifier::Orbit;
		}
	}
};

// Fast forward state : screen is blacked out, exit state depends on orbital simulation
class FNovaFastForwardState : public FNovaGameState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::FastForward,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameWorld->FastForward();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameState::UpdateState();

		if (GameWorld->IsInFastForward())
		{
			return ENovaGameStateIdentifier::FastForward;
		}
		else if (OrbitalSimulationComponent->IsPlayerPastFirstManeuver())
		{
			if (OrbitalSimulationComponent->IsPlayerNearingLastManeuver())
			{
				return ENovaGameStateIdentifier::ArrivalIntro;
			}
			else
			{
				return ENovaGameStateIdentifier::Orbit;
			}
		}
		else
		{
			return ENovaGameStateIdentifier::Area;
		}
	}
};

/*----------------------------------------------------
    Area departure states
----------------------------------------------------*/

// Departure stage 1 : cutscene focused on spacecraft
class FNovaDepartureProximityState : public FNovaGameState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::CinematicSpacecraft);
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameState::UpdateState();

		if (GetSecondsInState() > DepartureCutsceneDuration)
		{
			return ENovaGameStateIdentifier::DepartureCoast;
		}
		else
		{
			return ENovaGameStateIdentifier::DepartureProximity;
		}
	}
};

// Departure stage 2 : chase cam until fast forward or arrival
class FNovaDepartureCoastState : public FNovaGameState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Chase,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameMode->ChangeAreaToOrbit();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameState::UpdateState();

		if (OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver() <= 0 && OrbitalSimulationComponent->IsPlayerNearingLastManeuver())
		{
			return ENovaGameStateIdentifier::ArrivalIntro;
		}
		else
		{
			return ENovaGameStateIdentifier::DepartureCoast;
		}
	}
};

/*----------------------------------------------------
    Area arrival states
----------------------------------------------------*/

// Arrival stage 1 : level loading and introduction
class FNovaArrivalIntroState : public FNovaGameState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::CinematicEnvironment,
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					TPair<const UNovaArea*, float> NearestAreaAndDistance = OrbitalSimulationComponent->GetPlayerNearestAreaAndDistance();
					GameMode->ChangeArea(NearestAreaAndDistance.Key);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameState::UpdateState();

		if (GetSecondsInState() > AreaIntroductionDuration)
		{
			return ENovaGameStateIdentifier::ArrivalCoast;
		}
		else
		{
			return ENovaGameStateIdentifier::ArrivalIntro;
		}
	}
};

// Arrival stage 2 : coast until proximity
class FNovaArrivalCoastState : public FNovaGameState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Chase);
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
		if (PlayerTrajectory == nullptr ||
			(PlayerTrajectory->GetArrivalTime() - GameWorld->GetCurrentTime()) * 60.0 < ArrivalCutsceneDuration)
		{
			return ENovaGameStateIdentifier::ArrivalProximity;
		}
		else
		{
			return ENovaGameStateIdentifier::ArrivalCoast;
		}
	}
};

// Arrival stage 3 : proximity
class FNovaArrivalProximityState : public FNovaGameState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::CinematicSpacecraft);
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameState::UpdateState();

		if (OrbitalSimulationComponent->GetPlayerTrajectory() == nullptr)
		{
			return ENovaGameStateIdentifier::Area;
		}

		return ENovaGameStateIdentifier::ArrivalProximity;
	}
};
