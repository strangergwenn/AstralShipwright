// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"
#include "Components/SceneComponent.h"
#include "Nova/Tools/NovaActorTools.h"

#include "NovaSpacecraftThrusterComponent.generated.h"


/** Exhaust management object */
USTRUCT()
struct FNovaThrusterExhaust
{
	GENERATED_BODY()

	// Exhaust identifier
	FName                                         Name;

	// Exhaust mesh
	UPROPERTY()
	class UStaticMeshComponent*                   Mesh;

	// Exhaust material
	UPROPERTY()
	class UMaterialInstanceDynamic*               Material;

	// Current power
	TNovaTimedAverage<float>                      Power;
};


/** Thruster component class that attaches to an actor to add thrusters to the root mesh */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftThrusterComponent : public USceneComponent, public INovaAdditionalComponentInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftThrusterComponent();
	

	/*----------------------------------------------------
		Inherited
	----------------------------------------------------*/

	virtual void SetupComponent(TSoftObjectPtr<class UObject> AdditionalAsset) override;

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
protected:

	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

	// Exhaust template mesh
	UPROPERTY()
	class UStaticMesh*                            ExhaustMesh;

	// Exhausts
	UPROPERTY()
	TArray<FNovaThrusterExhaust>                  ThrusterExhausts;

	
};
