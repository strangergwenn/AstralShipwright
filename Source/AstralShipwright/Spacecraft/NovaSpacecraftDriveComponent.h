// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"
#include "Components/StaticMeshComponent.h"

#include "Neutron/Actor/NeutronActorTools.h"

#include "NovaSpacecraftDriveComponent.generated.h"

/** Main drive component class that attaches to a mesh to add engine effects */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftDriveComponent
	: public UStaticMeshComponent
	, public INovaAdditionalComponentInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftDriveComponent();

	/** Check for drive activity */
	bool IsDriveActive() const
	{
		return ExhaustPower.Get() > 0.1f;
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

	// Max engine heat effect temperature in °C
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaxTemperature;

	// Temperature ramp up speed in °C/s
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float TemperatureRampUp;

	// Temperature ramp down speed in °C/s
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float TemperatureRampDown;

	// Exhaust template mesh
	UPROPERTY()
	class UStaticMesh* ExhaustMesh;

	// Exhaust material
	UPROPERTY()
	class UMaterialInstanceDynamic* ExhaustMaterial;

	// Current power
	TNeutronTimedAverage<float> ExhaustPower;

	// Temperature
	float CurrentTemperature;
	float EngineIntensity;
};
