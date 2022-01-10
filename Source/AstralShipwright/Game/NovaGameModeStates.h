// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaGameMode.h"
#include "NovaGameState.h"

#include "NovaArea.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Player/NovaPlayerController.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"
#include "System/NovaAssetManager.h"

#include "Nova.h"

/*----------------------------------------------------
    Constants
----------------------------------------------------*/

// Cutscene timing values in seconds
namespace ENovaGameModeStateTiming
{
constexpr float CommonCutsceneDelay       = 2.0;
constexpr float DepartureCutsceneDuration = 5.0;
constexpr float AreaIntroductionDuration  = 5.0;
constexpr float ArrivalCutsceneDuration   = 5.0;
}    // namespace ENovaGameModeStateTiming

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

	/** Get how long of a warning to get before a maneuver */
	virtual FNovaTime GetManeuverWarningTime() const
	{
		return FNovaTime::FromSeconds(ENovaGameModeStateTiming::CommonCutsceneDelay);
	}

	/** Get how long of a warning to get before arriving at a destination */
	virtual FNovaTime GetArrivalWarningTime() const
	{
		if (IsArrivingInSpace())
		{
			return FNovaTime::FromSeconds(ENovaGameModeStateTiming::ArrivalCutsceneDuration);
		}
		else
		{
			return FNovaTime::FromSeconds(
				ENovaGameModeStateTiming::AreaIntroductionDuration + ENovaGameModeStateTiming::ArrivalCutsceneDuration);
		}
	}

	/** Can we fast-forward in this state? */
	virtual bool CanFastForward() const
	{
		return false;
	}

protected:
	/** Get the time spent in this state in minutes */
	double GetMinutesInState() const
	{
		return (GameState->GetCurrentTime() - StateStartTime).AsMinutes();
	}

	/** Get the time spent in this state in seconds */
	double GetSecondsInState() const
	{
		return (GameState->GetCurrentTime() - StateStartTime).AsSeconds();
	}

	/** Check if we are going to a space based destination */
	bool IsArrivingInSpace() const
	{
		auto NearestAreaAndDistance = OrbitalSimulationComponent->GetPlayerNearestAreaAndDistanceAtArrival();
		return NearestAreaAndDistance.Value > ENovaConstants::TrajectoryDistanceError;
	}

protected:
	// Local state
	FString   StateName;
	FNovaTime StateStartTime;

	// Outer objects
	class ANovaPlayerController*           PC;
	class ANovaGameMode*                   GameMode;
	class ANovaGameState*                  GameState;
	class UNovaOrbitalSimulationComponent* OrbitalSimulationComponent;
};

/*----------------------------------------------------
    Idle states
----------------------------------------------------*/

// Area state : exit just before a new trajectory starts
class FNovaAreaState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[this]()
				{
					GameState->SetUsingTrajectoryMovement(false);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();

		if (PlayerTrajectory && PlayerTrajectory->GetFirstManeuverStartTime() <=
									GameState->GetCurrentTime() + FNovaTime::FromSeconds(ENovaGameModeStateTiming::CommonCutsceneDelay))
		{
			return ENovaGameStateIdentifier::DepartureProximity;
		}
		else
		{
			return ENovaGameStateIdentifier::Area;
		}
	}

	virtual bool CanFastForward() const override
	{
		return true;
	}
};

// Orbit state : operating locally in empty space until arrival or fast forward
class FNovaOrbitState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[this]()
				{
					GameState->SetUsingTrajectoryMovement(false);
					GameMode->ChangeAreaToOrbit();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver(GameMode->GetManeuverWarnTime()) <= FNovaTime() &&
			OrbitalSimulationComponent->IsPlayerNearingLastManeuver())
		{
			auto NearestAreaAndDistance = OrbitalSimulationComponent->GetPlayerNearestAreaAndDistanceAtArrival();
			if (NearestAreaAndDistance.Value > ENovaConstants::TrajectoryDistanceError)
			{
				return ENovaGameStateIdentifier::ArrivalProximity;
			}
			else
			{
				return ENovaGameStateIdentifier::ArrivalIntro;
			}
		}
		else
		{
			return ENovaGameStateIdentifier::Orbit;
		}
	}

	virtual FNovaTime GetManeuverWarningTime() const override
	{
		return OrbitalSimulationComponent->IsPlayerNearingLastManeuver() ? GetArrivalWarningTime() : FNovaTime();
	}

	virtual bool CanFastForward() const override
	{
		return true;
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
				[this]()
				{
					GameState->FastForward();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();

		// Only the game state is allowed to clear this state
		if (GameState->IsInFastForward())
		{
			return ENovaGameStateIdentifier::FastForward;
		}

		// We are no longer under fast forward, and on a started trajectory
		else if (OrbitalSimulationComponent->IsPlayerPastFirstManeuver())
		{
			if (OrbitalSimulationComponent->IsPlayerNearingLastManeuver())
			{
				if (IsArrivingInSpace())
				{
					return ENovaGameStateIdentifier::ArrivalProximity;
				}
				else
				{
					return ENovaGameStateIdentifier::ArrivalIntro;
				}
			}
			else
			{
				return ENovaGameStateIdentifier::Orbit;
			}
		}

		// We are no longer under fast forward, and a trajectory is going to start from a station
		else if (OrbitalSimulationComponent->GetTimeLeftUntilPlayerTrajectoryStart() < ENovaGameModeStateTiming::CommonCutsceneDelay &&
				 GameState->GetCurrentArea() != GameMode->OrbitArea)
		{
			return ENovaGameStateIdentifier::DepartureProximity;
		}

		// We are neither under fast forward not on any trajectory
		else
		{
			return ENovaGameStateIdentifier::Area;
		}
	}

	virtual FNovaTime GetManeuverWarningTime() const override
	{
		return OrbitalSimulationComponent->IsPlayerNearingLastManeuver() ? FNovaTime() : FNovaGameModeState::GetManeuverWarningTime();
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
				[this]()
				{
					GameState->SetUsingTrajectoryMovement(true);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (GetSecondsInState() > ENovaGameModeStateTiming::CommonCutsceneDelay + ENovaGameModeStateTiming::DepartureCutsceneDuration)
		{
			return ENovaGameStateIdentifier::Orbit;
		}
		else
		{
			return ENovaGameStateIdentifier::DepartureProximity;
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

		const UNovaArea* NearestArea = OrbitalSimulationComponent->GetPlayerNearestAreaAndDistanceAtArrival().Key;

		PC->SharedTransition(ENovaPlayerCameraState::CinematicEnvironment,    //
			FNovaAsyncAction::CreateLambda(
				[this, NearestArea]()
				{
					GameMode->ChangeArea(NearestArea);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
		if (PlayerTrajectory == nullptr || (PlayerTrajectory->GetArrivalTime() - GameState->GetCurrentTime() <
											   FNovaTime::FromSeconds(ENovaGameModeStateTiming::ArrivalCutsceneDuration)))
		{
			return ENovaGameStateIdentifier::ArrivalProximity;
		}
		else
		{
			return ENovaGameStateIdentifier::ArrivalIntro;
		}
	}

private:
	bool IsStillInSpace;
};

// Arrival stage 2 : proximity
class FNovaArrivalProximityState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(IsArrivingInSpace() ? ENovaPlayerCameraState::CinematicOrbit : ENovaPlayerCameraState::CinematicSpacecraft,
			FNovaAsyncAction::CreateLambda(
				[this]()
				{
					GameMode->SetCurrentAreaVisible(true);
					GameState->SetUsingTrajectoryMovement(true);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (GetSecondsInState() > ENovaGameModeStateTiming::ArrivalCutsceneDuration)
		{
			return IsArrivingInSpace() ? ENovaGameStateIdentifier::Orbit : ENovaGameStateIdentifier::Area;
		}
		else
		{
			return ENovaGameStateIdentifier::ArrivalProximity;
		}
	}
};
