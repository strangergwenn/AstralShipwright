// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/MovementComponent.h"
#include "NovaSpacecraftMovementComponent.generated.h"


DECLARE_DELEGATE(FNovaMovementCallback);


/** Movement state */
UENUM(BlueprintType)
enum class ENovaMovementState : uint8
{
	Docked,
	Undocking,
	Docking,
	Idle
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
		, Time(0)
	{}

	bool operator==(const FNovaAttitudeCommand& Other) const
	{
		return Location == Other.Location
			&& Velocity == Other.Velocity
			&& Direction == Other.Direction
			&& Roll == Other.Roll;
	}

	bool operator!=(const FNovaAttitudeCommand& Other) const
	{
		return !operator==(Other);
	}

	UPROPERTY()
	FVector_NetQuantize10 Location;

	UPROPERTY()
	FVector_NetQuantize10 Velocity;

	UPROPERTY()
	FVector_NetQuantize10 Direction;

	UPROPERTY()
	float Roll;

	UPROPERTY()
	float Time;
};

/** Data structure for the current state after a command has been sent to the server */
struct FNovaUnacknowledgedAttitudeCommand
{
	FNovaUnacknowledgedAttitudeCommand()
		: Transform(FTransform::Identity)
		, LinearVelocity(FVector::ZeroVector)
		, AngularVelocity(FVector::ZeroVector)
		, Time(0)
	{}

	FNovaUnacknowledgedAttitudeCommand(const FNovaAttitudeCommand& Cm, const FTransform& Tr, const FVector& Lv, const FVector& Av, float Time)
		: Command(Cm)
		, Transform(Tr)
		, LinearVelocity(Lv)
		, AngularVelocity(Av)
		, Time(Time)
	{}

	FNovaAttitudeCommand Command;
	FTransform Transform;
	FVector LinearVelocity;
	FVector AngularVelocity;
	float Time;
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

	/** Get the current state of the movement system */
	UFUNCTION(BlueprintCallable)
	ENovaMovementState GetState() const
	{
		return MovementState;
	}

	/** Dock at a particular location */
	void Dock(FNovaMovementCallback Callback, const FVector& Location);

	/** Undock from the current dock */
	void Undock(FNovaMovementCallback Callback);


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

	/** Update the attitude control on server */
	void UpdateServerAttitude();

	/** Store the latest command and transform if a new local command was issued */
	void UpdateUnacknowledgedCommands();

	UFUNCTION(Server, Unreliable)
	void ServerSetDesiredAttitude(const FNovaAttitudeCommand& Attitude);

	UFUNCTION()
	void OnServerDesiredAttitudeReplicated();


	/*----------------------------------------------------
		Internal movement implementation
	----------------------------------------------------*/

protected:

	/** Measure velocities and accelerations */
	void ProcessMeasurements(float DeltaTime);

	/** Run attitude control on linear velocity */
	void ProcessLinearAttitude(float DeltaTime);

	/** Run attitude control on angular velocity */
	void ProcessAngularAttitude(float DeltaTime);

	/** Process overall movement */
	void ProcessMovement(float DeltaTime);

	/** Replicate the desired attitude */
	void ProcessNetworkCorrection(float DeltaTime);

	/** Apply hit effects */
	virtual void OnHit(const FHitResult& Hit, const FVector& HitVelocity);


	/*----------------------------------------------------
		Properties
	----------------------------------------------------*/
	
public:

	// Maximum linear acceleration rate in m/s²
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float LinearAcceleration;

	// Distance under which we consider stopped
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float LinearDeadDistance;


	// Maximum axis turn acceleration rate in °/s²
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularAcceleration;

	// Slope multiplier on the angular target
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float AngularControlIntensity;

	// Maximum moving velocity in m/s
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float MaxLinearVelocity;

	// Maximum turn rate in °/s (pitch & roll)
	UPROPERTY(Category = Gaia, EditDefaultsOnly)
	float MaxAngularVelocity;

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
	ENovaMovementState                            MovementState;
	FNovaMovementCallback                         StateCallback;
	bool                                          MovementStateDirty;

	// Authoritative attitude input
	UPROPERTY(ReplicatedUsing=OnServerDesiredAttitudeReplicated)
	FNovaAttitudeCommand                          ServerDesiredAttitude;

	// Client-side attitude input
	FNovaAttitudeCommand                          DesiredAttitude;
	FNovaAttitudeCommand                          PreviousDesiredAttitude;
	bool                                          ShouldStoreCommand;
	TArray<FNovaUnacknowledgedAttitudeCommand>    UnacknowledgedAttitudeCommands;

	// Movement state
	FVector                                       CurrentLinearVelocity;
	FVector                                       CurrentAngularVelocity;

	// Measured data
	bool                                          LinearAttitudeIdle;
	bool                                          AngularAttitudeIdle;
	float                                         LinearAttitudeDistance;
	FVector                                       PreviousVelocity;
	FVector                                       PreviousAngularVelocity;
	FVector                                       MeasuredAcceleration;
	FVector                                       MeasuredAngularAcceleration;


	/*----------------------------------------------------
		Getters
	----------------------------------------------------*/

public:

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
		return 0.0f;
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
