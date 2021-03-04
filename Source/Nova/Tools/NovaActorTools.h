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
	static float GetPlayerLatency(class APlayerController* PC);

	/** Get half of the ping time in seconds for this player */
	static float GetPlayerLatency(class APlayerState* PC);

	/** Apply collision effects on velocity */
	static FVector GetVelocityCollisionResponse(
		const FVector& Velocity, const FHitResult& Hit, float BaseRestitution = 1.0f, const FVector& WorldUp = FVector(0, 0, 1));

	/** Play a camera shake with linear attenuation */
	static void PlayCameraShake(TSubclassOf<class UCameraShakeBase> Shake, class AActor* Owner, float Scale = 1.0f,
		float AttenuationStartDistance = 300, float AttenuationDistance = 5000);
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
