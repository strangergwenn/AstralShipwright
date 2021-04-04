// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"
#include "Nova/Actor/NovaActorTools.h"
#include "Components/StaticMeshComponent.h"

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

	// Exhaust material
	UPROPERTY()
	class UMaterialInstanceDynamic* ExhaustMaterial;

	// Current power
	TNovaTimedAverage<float> ExhaustPower;
};
