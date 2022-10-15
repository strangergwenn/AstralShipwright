// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"

#include "Components/SceneComponent.h"

#include "NovaSpacecraftMiningRigComponent.generated.h"

/** Main drive component class that attaches to a mesh to add engine effects */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftMiningRigComponent
	: public USceneComponent
	, public INovaAdditionalComponentInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftMiningRigComponent();

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

	// Drilling effect template
	UPROPERTY()
	class UNiagaraSystem* DrillingEffect;

	// Drilling effect component
	UPROPERTY()
	class UNiagaraComponent* DrillingEffectComponent;
};
