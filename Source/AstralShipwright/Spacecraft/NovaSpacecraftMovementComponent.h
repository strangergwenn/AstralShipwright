// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/MovementComponent.h"
#include "Game/NovaGameTypes.h"
#include "NovaSpacecraftMovementComponent.generated.h"

/** Movement state */
UENUM()
enum class ENovaMovementState : uint8
{
	Idle,
	Stopping,

	Docking,
	Docked,
	Undocking,
	Orbiting,

	ExitingOrbit,

	Anchoring,
	Anchored,
	ExitingAnchor
};

/** Initialization parameters */
USTRUCT(Atomic)
struct FNovaMovementDockState
{
	GENERATED_BODY()

	FNovaMovementDockState() : Actor(nullptr), IsDocked(true)
	{}

	UPROPERTY()
	const class ANovaPlayerStart* Actor;

	UPROPERTY()
	bool IsDocked;
};

/** High level movement command sent by a player */
USTRUCT(Atomic)
struct FNovaMovementCommand
{
	GENERATED_BODY()

	FNovaMovementCommand() : State(ENovaMovementState::Idle), Target(nullptr), Dirty(false)
	{}

	FNovaMovementCommand(ENovaMovementState S, const class AActor* A = nullptr) : State(S), Target(A), Dirty(false)
	{}

	UPROPERTY()
	ENovaMovementState State;

	UPROPERTY()
	const class AActor* Target;

	bool Dirty;
};

/** Replicated attitude command */
USTRUCT()
struct FNovaAttitudeCommand
{
	GENERATED_BODY()

	FNovaAttitudeCommand() : Location(FVector::ZeroVector), Velocity(FVector::ZeroVector), Direction(FVector::ZeroVector), Roll(0)
	{}

	UPROPERTY()
	FVector_NetQuantize10 Location;

	UPROPERTY()
	FVector_NetQuantize10 Velocity;

	UPROPERTY()
	FVector_NetQuantize10 Direction;

	UPROPERTY()
	float Roll;
};

/** Spacecraft movement component */
UCLASS(ClassGroup = (Nova))
class UNovaSpacecraftMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	UNovaSpacecraftMovementComponent();

	/*----------------------------------------------------
	    Movement API
	----------------------------------------------------*/

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Initialize the component with a starting point */
	void Initialize(const class ANovaPlayerStart* Start);

	/** Check if this component is initialized */
	bool IsInitialized() const;

	/** Get the player start currently in use */
	const class ANovaPlayerStart* GetPlayerStart() const
	{
		return DockState.Actor;
	}

	/** Signal that the area changed */
	void Reset();

	/*** Can we dock */
	bool CanDock() const;

	/*** Can we undock */
	bool CanUndock() const
	{
		return IsInitialized() && GetState() == ENovaMovementState::Docked;
	}

	/** Check if the spacecraft is docked */
	bool IsDocked() const
	{
		return IsInitialized() && GetState() == ENovaMovementState::Docked;
	}

	/** Check if the spacecraft is docking or undocking */
	bool IsDockingUndocking() const
	{
		return IsInitialized() && (GetState() == ENovaMovementState::Docking || GetState() == ENovaMovementState::Undocking);
	}

	/** Check if the spacecraft is orbiting */
	bool IsOrbiting() const
	{
		return IsInitialized() && GetState() == ENovaMovementState::Orbiting;
	}

	/** Check if the spacecraft is anchored */
	bool IsAnchored() const
	{
		return IsInitialized() && GetState() == ENovaMovementState::Anchored;
	}

	/** Check if the ship is idle */
	bool IsIdle() const
	{
		return LinearAttitudeIdle && AngularAttitudeIdle;
	}

	/** Check if the spacecraft is aligned to the next maneuver */
	bool IsAlignedToManeuver() const;

	/** Check if the main drive is enabled */
	bool CanManeuver() const;

	/** Dock at a particular location */
	void Dock(FSimpleDelegate Callback = FSimpleDelegate());

	/** Undock from the current dock */
	void Undock(FSimpleDelegate Callback = FSimpleDelegate());

	/** Stop right there with no particular target */
	void Stop(FSimpleDelegate Callback = FSimpleDelegate());

	/** Start orbiting the nearest asteroid */
	void StartOrbiting();

	/** Stop orbiting and exit to the entry point */
	void StopOrbiting(FSimpleDelegate Callback = FSimpleDelegate());

	/** Anchor to the nearest asteroid */
	void Anchor(FSimpleDelegate Callback = FSimpleDelegate());

	/** Release the anchor and go back to orbit */
	void ExitAnchor(FSimpleDelegate Callback = FSimpleDelegate());

	/*----------------------------------------------------
	    High level movement
	----------------------------------------------------*/

protected:
	/** Run the high level state machine */
	void ProcessState();

	/** Signal completion to the user */
	void SignalCompletion();

	UFUNCTION(Client, Reliable)
	void ClientSignalCompletion();

	/*----------------------------------------------------
	    Networking
	----------------------------------------------------*/

protected:
	/** Signal the flight controller to use this command */
	void RequestMovement(const FNovaMovementCommand& Command);

	UFUNCTION(Server, Reliable)
	void ServerRequestMovement(const FNovaMovementCommand& Command);

	/*----------------------------------------------------
	    Internal movement implementation
	----------------------------------------------------*/

protected:
	/** Start point replication event */
	UFUNCTION()
	void OnDockStateReplicated(const FNovaMovementDockState& PreviousDockState);

	/** Reset the local state */
	void ResetState();

	/** Measure velocities and accelerations */
	void ProcessMeasurementsBeforeAttitude(float DeltaTime);

	/** Process attitude changes */
	void ProcessMeasurementsAfterAttitude(float DeltaTime);

	/** Run attitude control on linear velocity */
	void ProcessLinearAttitude(float DeltaTime);

	/** Run attitude control on angular velocity */
	void ProcessAngularAttitude(float DeltaTime);

	/** Process overall movement */
	void ProcessMovement(float DeltaTime);

	/** Process trajectory movement */
	void ProcessOrbitalMovement(float DeltaTime);

	/** Apply hit effects */
	virtual void OnHit(const FHitResult& Hit, const FVector& HitVelocity);

	/** Get the next maneuver */
	const struct FNovaManeuver* GetNextManeuver() const;

	/** Get the direction for the upcoming maneuver */
	FVector GetManeuverDirection() const;

	/** Get the transform to use when a new dock actor is ready */
	FTransform GetInitialTransform() const;

	/** Get the max velocity */
	float GetMaximumAcceleration() const
	{
		return (MovementCommand.State == ENovaMovementState::Docking || MovementCommand.State == ENovaMovementState::Undocking)
		         ? MaxSlowLinearAcceleration
		         : LinearAcceleration;
	}

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Distance under which we consider stopped
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float LinearDeadDistance;

	// Maximum moving velocity in m/s
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaxLinearVelocity;

	// Maximum moving acceleration in m/s-2 while docking
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaxSlowLinearAcceleration;

	// Maximum delta-V in m/s for which thrusters shall be enabled
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaxDeltaVForThrusters;

	// Distance under which we consider stopped
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float AngularDeadDistance;

	// Maximum turn rate in °/s (pitch & roll)
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaxAngularVelocity;

	// How much to underestimate stopping distances
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float AngularOvershootRatio;

	// Dot product value over which we consider vectors to be collinear
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float AngularColinearityThreshold;

	// Base restitution coefficient of hits
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float RestitutionCoefficient;

	// Collision shake
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	TSubclassOf<class UCameraShakeBase> HitShake;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// High-level state
	UPROPERTY(Replicated)
	FNovaMovementCommand MovementCommand;
	FSimpleDelegate      CompletionCallback;

	// Authoritative attitude input, produced by the server in real-time
	UPROPERTY(Replicated)
	FNovaAttitudeCommand AttitudeCommand;

	// Dock parameters
	UPROPERTY(ReplicatedUsing = OnDockStateReplicated)
	FNovaMovementDockState DockState;

	// Trajectory movement data
	FVector PreviousOrbitalLocation;
	FVector CurrentOrbitalLocation;

	// Acceleration data
	float LinearAcceleration;
	float AngularAcceleration;

	// Movement state
	FVector CurrentLinearVelocity;
	FVector CurrentAngularVelocity;

	// Measured data
	bool    LinearAttitudeIdle;
	bool    AngularAttitudeIdle;
	float   LinearAttitudeDistance;
	float   AngularAttitudeDistance;
	FVector PreviousVelocity;
	FVector PreviousAngularVelocity;
	FVector MeasuredAcceleration;
	FVector MeasuredAngularAcceleration;

	/*----------------------------------------------------
	    Getters
	----------------------------------------------------*/

public:
	/** Get the current state of the movement system */
	UFUNCTION(BlueprintCallable)
	ENovaMovementState GetState() const
	{
		return MovementCommand.State;
	}

	const FVector GetCurrentLinearVelocity() const
	{
		return CurrentLinearVelocity;
	}

	const FVector GetCurrentAngularVelocity() const
	{
		return CurrentAngularVelocity;
	}

	const FVector GetThrusterAcceleration() const;

	const FVector GetThrusterAngularAcceleration() const
	{
		return MeasuredAngularAcceleration;
	}

	ENetRole GetLocalRole() const
	{
		return GetOwner()->GetLocalRole();
	}

	ENetRole GetRemoteRole() const
	{
		return GetOwner()->GetRemoteRole();
	}
};
