// Nova project - Gwennaël Arbona

#include "NovaGameMode.h"
#include "NovaGameState.h"

#include "NovaArea.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Nova.h"

/*----------------------------------------------------
    Constants
----------------------------------------------------*/

// Cutscene timing values in seconds
constexpr float DepartureCutsceneDelay    = 2.0;
constexpr float DepartureCutsceneDuration = 5.0;
constexpr float AreaIntroductionDuration  = 5.0;
constexpr float ArrivalCutsceneDuration   = 5.0;

/*----------------------------------------------------
    Game state class
----------------------------------------------------*/

/** Game state */
class FNovaGameModeState
{
public:
	FNovaGameModeState() : PC(nullptr), GameState(nullptr), OrbitalSimulationComponent(nullptr)
	{}

	virtual ~FNovaGameModeState()
	{}

	/** Set the required work data */
	void Initialize(const FString& Name, class ANovaPlayerController* P, class ANovaGameMode* GM, class ANovaGameState* GW,
		class UNovaOrbitalSimulationComponent* OSC)
	{
		StateName                  = Name;
		PC                         = P;
		GameMode                   = GM;
		GameState                  = GW;
		OrbitalSimulationComponent = OSC;
	}

	/** Get the time spent in this state in minutes */
	double GetMinutesInState() const
	{
		return GameState->GetCurrentTime() - StateStartTime;
	}

	/** Get the time spent in this state in seconds */
	double GetSecondsInState() const
	{
		return (GameState->GetCurrentTime() - StateStartTime) * 60.0;
	}

	/** Enter this state from a previous one */
	virtual void EnterState(ENovaGameStateIdentifier PreviousStateIdentifier)
	{
		NLOG("FNovaGameState::EnterState : entering state '%s'", *StateName);
		StateStartTime = GameState->GetCurrentTime();
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
	class ANovaGameState*                  GameState;
	class UNovaOrbitalSimulationComponent* OrbitalSimulationComponent;
};

/*----------------------------------------------------
    Idle states
----------------------------------------------------*/

// Area state : operating locally within an area
class FNovaAreaState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->SetUsingTrajectoryMovement(false);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();

		if (PlayerTrajectory &&
			PlayerTrajectory->GetFirstManeuverStartTime() <= GameState->GetCurrentTime() + DepartureCutsceneDelay / 60.0)
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
class FNovaOrbitState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameMode->ChangeAreaToOrbit();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

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
class FNovaFastForwardState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::FastForward,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->FastForward();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (GameState->IsInFastForward())
		{
			return ENovaGameStateIdentifier::FastForward;
		}
		else if (OrbitalSimulationComponent->IsPlayerPastFirstManeuver())
		{
			GameMode->ResetSpacecraft();

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
			GameMode->ResetSpacecraft();

			return ENovaGameStateIdentifier::Area;
		}
	}
};

/*----------------------------------------------------
    Area departure states
----------------------------------------------------*/

// Departure stage 1 : cutscene focused on spacecraft
class FNovaDepartureProximityState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::CinematicSpacecraft,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->SetUsingTrajectoryMovement(true);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (GetSecondsInState() > DepartureCutsceneDelay + DepartureCutsceneDuration)
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
class FNovaDepartureCoastState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Chase,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->SetUsingTrajectoryMovement(false);
					GameMode->ChangeAreaToOrbit();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

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
class FNovaArrivalIntroState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

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
		FNovaGameModeState::UpdateState();

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
class FNovaArrivalCoastState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Chase);
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
		if (PlayerTrajectory == nullptr ||
			(PlayerTrajectory->GetArrivalTime() - GameState->GetCurrentTime()) * 60.0 < ArrivalCutsceneDuration)
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
class FNovaArrivalProximityState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::CinematicSpacecraft,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->SetUsingTrajectoryMovement(true);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (OrbitalSimulationComponent->GetPlayerTrajectory() == nullptr)
		{
			return ENovaGameStateIdentifier::Area;
		}

		return ENovaGameStateIdentifier::ArrivalProximity;
	}
};
