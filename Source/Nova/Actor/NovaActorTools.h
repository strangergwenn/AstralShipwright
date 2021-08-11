// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaActorTools.generated.h"

/*----------------------------------------------------
    Helper class
----------------------------------------------------*/

UCLASS(ClassGroup = (Nova))
class UNovaActorTools : public UObject
{
	GENERATED_BODY()

public:
	/** Sort actors from closest to farthest from location */
	static void SortActorsByClosestDistance(TArray<AActor*>& Actors, const FVector& BaseLocation);

	/** Sort components from closest to farthest from location */
	static void SortComponentsByClosestDistance(TArray<USceneComponent*>& Components, const FVector& BaseLocation);

	/** Get half of the ping time in seconds for this player */
	static float GetPlayerLatency(const class APlayerController* PC);

	/** Get half of the ping time in seconds for this player */
	static float GetPlayerLatency(const class APlayerState* PC);

	/** Apply collision effects on velocity */
	static FVector GetVelocityCollisionResponse(
		const FVector& Velocity, const FHitResult& Hit, float BaseRestitution = 1.0f, const FVector& WorldUp = FVector(0, 0, 1));

	/** Play a camera shake with linear attenuation */
	static void PlayCameraShake(TSubclassOf<class UCameraShakeBase> Shake, class AActor* Owner, float Scale = 1.0f,
		float AttenuationStartDistance = 300, float AttenuationDistance = 5000);
};

/*----------------------------------------------------
    Camera filter with resistance and inertia
----------------------------------------------------*/

USTRUCT()
struct FNovaCameraInputFilter
{
	GENERATED_BODY()

public:
	FNovaCameraInputFilter()
		: Velocity(200.0f)
		, GamepadMultiplier(0.5f)
		, Acceleration(300.0f)
		, Resistance(1 / 360.0f)
		, InputPower(3.0f)
		, Brake(2.f)
		, Brake2(04.f)
	{}

public:
	/** Apply the filter to a current position, speed, to match a target */
	template <bool UnwindPosition = false>
	void ApplyFilter(
		float& CurrentPosition, float& CurrentVelocity, float& TargetPosition, const float DeltaTime, const bool IsGamepad) const
	{
		float MaximumVelocity = Velocity;
		if (IsGamepad)
		{
			MaximumVelocity *= GamepadMultiplier;
		}

		// Compute acceleration and resistance
		float Acc = FMath::Pow(TargetPosition, InputPower) * Acceleration;
		float Res = FMath::Sign(CurrentVelocity) *
					(Resistance * FMath::Square(CurrentVelocity) + (Acc == 0 ? Brake2 + Brake * FMath::Abs(CurrentVelocity) : 0));
		float MaxResDeltaSpeed = CurrentVelocity;
		float AccDeltaSpeed    = Acc * DeltaTime;
		float ResDeltaSpeed    = -(FMath::Abs(Res * DeltaTime) > FMath::Abs(MaxResDeltaSpeed) ? MaxResDeltaSpeed : Res * DeltaTime);

		// Update velocity, integrate the current angle and consume the input
		CurrentVelocity += AccDeltaSpeed + ResDeltaSpeed;
		CurrentVelocity = FMath::Clamp(CurrentVelocity, -MaximumVelocity, MaximumVelocity);
		CurrentPosition += CurrentVelocity * DeltaTime;
		TargetPosition = 0;

		// Optionally unwind output
		if (UnwindPosition)
		{
			CurrentPosition = FMath::UnwindDegrees(CurrentPosition);
		}
	}

	void ApplyFilter(FVector2D& CurrentPosition, FVector2D& CurrentVelocity, FVector2D& TargetPosition, const float DeltaTime,
		const bool IsGamepad) const
	{
		ApplyFilter(CurrentPosition.X, CurrentVelocity.X, TargetPosition.X, DeltaTime, IsGamepad);
		ApplyFilter(CurrentPosition.Y, CurrentVelocity.Y, TargetPosition.Y, DeltaTime, IsGamepad);
	}

	void ApplyFilter(
		FVector& CurrentPosition, FVector& CurrentVelocity, FVector& TargetPosition, const float DeltaTime, const bool IsGamepad) const
	{
		ApplyFilter(CurrentPosition.X, CurrentVelocity.X, TargetPosition.X, DeltaTime, IsGamepad);
		ApplyFilter(CurrentPosition.Y, CurrentVelocity.Y, TargetPosition.Y, DeltaTime, IsGamepad);
		ApplyFilter(CurrentPosition.Z, CurrentVelocity.Z, TargetPosition.Z, DeltaTime, IsGamepad);
	}

public:
	// Angular velocity of camera in °/s
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float Velocity;

	// Velocity multiplier when using a gamepad
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float GamepadMultiplier;

	// Camera acceleration force in °/s²
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float Acceleration;

	// Camera acceleration force as a multiplier of velocity
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float Resistance;

	// Power function applied to inputs
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float InputPower;

	// Brake component 1
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float Brake;

	// Brake component 2
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float Brake2;
};

/*----------------------------------------------------
    Time-based moving average structure
----------------------------------------------------*/

template <class T>
struct TNovaTimedAverage
{
public:
	TNovaTimedAverage()
	{
		SetPeriod(1.0f);
	}

	void SetPeriod(float New)
	{
		Period = New;

		Update();
	}

	void Set(T New, float DeltaTime = -1)
	{
		if (DeltaTime > Period || DeltaTime < 0)
		{
			DeltaTime = Period;
		}

		TPair<float, T> Value(DeltaTime, New);
		Values.Insert(Value, 0);

		Update();
	}

	T Get() const
	{
		return Average;
	}

	int32 Num() const
	{
		return Values.Num();
	}

	void Clear()
	{
		Values.Empty();
		Update();
	}

	void Update()
	{
		// Trim values that exceed the time
		float TotalTime     = 0;
		int32 ValidElements = 0;
		for (TPair<float, T> Entry : Values)
		{
			TotalTime += Entry.Key;
			if (TotalTime > Period)
			{
				break;
			}

			ValidElements++;
		}
		Values.RemoveAt(ValidElements, Values.Num() - ValidElements);

		// Sum values
		T Total = 0 * T();
		for (TPair<float, T> Entry : Values)
		{
			Total += Entry.Value;
		}

		// Get result
		if (Values.Num() > 0)
		{
			float SizeDivider = 1.0f / Values.Num();
			Average           = Total * SizeDivider;
		}
		else
		{
			Average = 0 * T();
		}
	}

private:
	T                       Average;
	TArray<TPair<float, T>> Values;
	float                   Period;
};

/*----------------------------------------------------
    Location interpolator
----------------------------------------------------*/

struct FNovaMovementInterpolator
{
	/** Build the interpolator */
	FNovaMovementInterpolator(const FTransform& StartTransform, const FVector& StartVelocity, const FVector& StartAngularVelocity,
		const FTransform& EndTransform, const FVector& EndVelocity, const FVector& EndAngularVelocity, float Duration);

	/** Get the interpolation result - full variant */
	void Get(FVector& OutLocation, FQuat& OutRotation, FVector& OutVelocity, FVector& OutAngularVelocity, float Time) const;

	/** Get the interpolation result - simple variant */
	void Get(FVector& OutLocation, FVector& OutVelocity, float Time) const;

protected:
	FVector StartLocation;
	FQuat   StartRotation;
	FVector StartDerivative;
	FVector StartAngularDerivative;

	FVector TargetLocation;
	FQuat   TargetRotation;
	FVector TargetDerivative;
	FVector TargetAngularDerivative;

	float InterpolationDuration;
};
