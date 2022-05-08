// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"
#include "Actor/NovaActorTools.h"
#include "Components/SceneComponent.h"

#include "NovaSpacecraftFloodlightComponent.generated.h"

/** Floodlight component */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftFloodlightComponent
	: public USceneComponent
	, public INovaAdditionalComponentInterface
{
	GENERATED_BODY()

public:
	UNovaSpacecraftFloodlightComponent();

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void SetAdditionalAsset(TSoftObjectPtr<class UObject> AdditionalAsset) override
	{}

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Lights
	UPROPERTY()
	TArray<class USpotLightComponent*> Lights;
};
