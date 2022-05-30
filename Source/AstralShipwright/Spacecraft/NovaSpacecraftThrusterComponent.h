// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"
#include "Actor/NovaActorTools.h"
#include "Components/SceneComponent.h"

#include "NovaSpacecraftThrusterComponent.generated.h"

/** Exhaust management object */
USTRUCT()
struct FNovaThrusterExhaust
{
	GENERATED_BODY()

	FNovaThrusterExhaust() : Mesh(nullptr), Material(nullptr)
	{}

	// Exhaust identifier
	FName Name;

	// Exhaust mesh
	UPROPERTY()
	class UStaticMeshComponent* Mesh;

	// Exhaust material
	UPROPERTY()
	class UMaterialInstanceDynamic* Material;

	// Current power
	TNovaTimedAverage<float> Power;
};

/** Thruster component class that attaches to a mesh to add thrusters effects */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftThrusterComponent
	: public USceneComponent
	, public INovaAdditionalComponentInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftThrusterComponent();

	/** Check for thruster activity */
	bool IsThrusterActive() const
	{
		for (const FNovaThrusterExhaust& Exhaust : ThrusterExhausts)
		{
			if (Exhaust.Power.Get() > 0.1f)
			{
				return true;
			}
		}

		return false;
	}

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void SetAdditionalAsset(TSoftObjectPtr<class UObject> AdditionalAsset) override;

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Exhaust template mesh
	UPROPERTY()
	class UStaticMesh* ExhaustMesh;

	// Exhausts
	UPROPERTY()
	TArray<FNovaThrusterExhaust> ThrusterExhausts;

	// Local data
	FCollisionQueryParams ThrusterTraceParams;
};
