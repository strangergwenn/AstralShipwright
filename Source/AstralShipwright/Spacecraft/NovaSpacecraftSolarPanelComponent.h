#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"
#include "Components/SceneComponent.h"

#include "Neutron/Actor/NeutronActorTools.h"

#include "NovaSpacecraftSolarPanelComponent.generated.h"

/** Solar panel component */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftSolarPanelComponent
	: public USceneComponent
	, public INovaAdditionalComponentInterface
{

public:

	GENERATED_BODY()

	UNovaSpacecraftSolarPanelComponent();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	virtual void SetAdditionalAsset(TSoftObjectPtr<class UObject> AdditionalAsset) override
	{}

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:

	FNovaTime LastUpdateGameTime;
};
