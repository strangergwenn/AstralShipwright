// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/MovementComponent.h"
#include "NovaSpacecraftMovementComponent.generated.h"


/** Movement state */
UENUM(BlueprintType)
enum class ENovaMovementState : uint8
{
	Docked,
	Undocking,
	Docking,
	Idle
};

/** High level movement command sent by a player */
USTRUCT(Atomic)
struct FNovaMovementCommand
{
	GENERATED_BODY()

	FNovaMovementCommand()
		: State(ENovaMovementState::Idle)
		, Target(nullptr)
		, Dirty(false)
	{}

	FNovaMovementCommand(ENovaMovementState S, const class AActor* A = nullptr)
		: State(S)
		, Target(A)
		, Dirty(true)
	{}

	UPROPERTY()
	ENovaMovementState State;

	UPROPERTY()
	const class AActor* Target;

	UPROPERTY()
	bool Dirty;
};

/** Replicated attitude command */
USTRUCT()
struct FNovaAttitudeCommand
{
	GENERATED_BODY()

	FNovaAttitudeCommand()
		: Location(FVector::ZeroVector)
		, Velocity(FVector::ZeroVector)
		, Direction(FVector::ZeroVector)
		, Roll(0)
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

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Get the current state of the movement system */
	UFUNCTION(BlueprintCallable)
	ENovaMovementState GetState() const
	{
		return MovementCommand.State;
	}

	/** Dock at a particular location */
	void Dock(const class AActor* Target, FSimpleDelegate Callback = FSimpleDelegate());

	/** Undock from the current dock */
	void Undock(FSimpleDelegate Callback = FSimpleDelegate());


	/*----------------------------------------------------
		High level movement
	----------------------------------------------------*/

protected:

	/** Run the high level state machine */
	void ProcessState();


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
	FNovaMovementCommand                          MovementCommand;
	FSimpleDelegate                               IdleCallback;

	// Authoritative attitude input, produced by the server in real-time
	UPROPERTY(Replicated)
	FNovaAttitudeCommand                          DesiredAttitude;

	// Movement state
	FVector                                       CurrentLinearVelocity;
	FVector                                       CurrentAngularVelocity;

	// Measured data
	bool                                          LinearAttitudeIdle;
	bool                                          AngularAttitudeIdle;
	float                                         LinearAttitudeDistance;
	float                                         AngularAttitudeDistance;
	FVector                                       PreviousVelocity;
	FVector                                       PreviousAngularVelocity;
	FVector                                       MeasuredAcceleration;
	FVector                                       MeasuredAngularAcceleration;


	/*----------------------------------------------------
		Getters
	----------------------------------------------------*/

public:

	inline bool IsMainDriveRunning() const
	{
		return AngularAttitudeDistance < VectoringAngle
			&& MovementCommand.State != ENovaMovementState::Docking
			&& MovementCommand.State != ENovaMovementState::Undocking;
	}

	inline const FVector GetLocation(const FVector& Offset) const
	{
		return UpdatedComponent->GetComponentLocation() + Offset * 100;
	}

	inline const FVector GetCurrentVelocity() const
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
