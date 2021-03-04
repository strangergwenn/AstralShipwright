// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/MovementComponent.h"
#include "Nova/Game/NovaGameTypes.h"
#include "NovaSpacecraftMovementComponent.generated.h"

/** Movement state */
UENUM()
enum class ENovaMovementState : uint8
{
	Idle,
	Docked,
	Undocking,
	Docking,
	LeavingArea,
	Stopping,
};

/** Type of intro animation to play in a level */
UENUM()
enum class ENovaLevelIntroType : uint8
{
	Idle,
	Docked,
	Braking
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

	FNovaAttitudeCommand()
		: Location(FVector::ZeroVector), Velocity(FVector::ZeroVector), Direction(FVector::ZeroVector), Roll(0), MainDriveEnabled(false)
	{}

	UPROPERTY()
	FVector_NetQuantize10 Location;

	UPROPERTY()
	FVector_NetQuantize10 Velocity;

	UPROPERTY()
	FVector_NetQuantize10 Direction;

	UPROPERTY()
	float Roll;

	UPROPERTY()
	bool MainDriveEnabled;
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
	void Initialize(const class AActor* Start, ENovaLevelIntroType IntroType);

	/** Initialize the component with a starting point */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastInitialize(const class AActor* Start, ENovaLevelIntroType IntroType);

	/** Uninitialize the component */
	void Reset();

	/** Dock at a particular location */
	void Dock(FSimpleDelegate Callback = FSimpleDelegate());

	/** Undock from the current dock */
	void Undock(FSimpleDelegate Callback = FSimpleDelegate());

	/** Leave the area */
	void LeaveArea(FSimpleDelegate Callback = FSimpleDelegate());

	/** Stop right there with no particular target */
	void Stop(FSimpleDelegate Callback = FSimpleDelegate());

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

	/** Apply hit effects */
	virtual void OnHit(const FHitResult& Hit, const FVector& HitVelocity);

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Maximum linear acceleration rate in m/s²
	// TODO move to asset
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float LinearAcceleration;

	// Maximum linear acceleration rate with the main drive on, in m/s²
	// TODO move to asset
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float LinearMainAcceleration;

	// Maximum axis turn acceleration rate in °/s²
	// TODO move to asset
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularAcceleration;

	// Max vectoring angle in degrees
	// TODO move to asset
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float VectoringAngle;

	// Distance under which we consider stopped
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float LinearDeadDistance;

	// Maximum moving velocity in m/s
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float MaxLinearVelocity;

	// Distance under which we consider stopped
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularDeadDistance;

	// Maximum turn rate in °/s (pitch & roll)
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float MaxAngularVelocity;

	// Slope multiplier on the angular target
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularControlIntensity;

	// How much to underestimate stopping distances
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularOvershootRatio;

	// Dot product value over which we consider vectors to be collinear
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularColinearityThreshold;

	// Base restitution coefficient of hits
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float RestitutionCoefficient;

	// Collision shake
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
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

	// Dock we were initialized for
	UPROPERTY(Replicated)
	const class AActor* StartActor;

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

	inline bool IsReady() const
	{
		return StartActor != nullptr;
	}

	inline bool IsMainDriveRunning() const
	{
		return AttitudeCommand.MainDriveEnabled && AngularAttitudeDistance < VectoringAngle;
	}

	inline const FVector GetLocation(const FVector& Offset = FVector::ZeroVector) const
	{
		return UpdatedComponent->GetComponentLocation() + Offset * 100;
	}

	inline bool IsMaxVelocity() const
	{
		return CurrentLinearVelocity.Size() >= MaxLinearVelocity;
	}

	inline const FVector GetCurrentLinearVelocity() const
	{
		return CurrentLinearVelocity;
	}

	inline const FVector GetCurrentAngularVelocity() const
	{
		return CurrentAngularVelocity;
	}

	inline const FVector GetThrusterAcceleration() const
	{
		return MeasuredAcceleration;
	}

	inline const FVector GetThrusterAngularAcceleration() const
	{
		return MeasuredAngularAcceleration;
	}

	inline const float GetMainDriveAcceleration() const
	{
		return IsMainDriveRunning() ? CurrentLinearVelocity.Size() : 0;
	}

	inline ENetRole GetLocalRole() const
	{
		return GetOwner()->GetLocalRole();
	}

	inline ENetRole GetRemoteRole() const
	{
		return GetOwner()->GetRemoteRole();
	}
};
