// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"
#include "Nova/Actor/NovaActorTools.h"
#include "Nova/Actor/NovaSkeletalMeshComponent.h"

#include "NovaSpacecraftHatchComponent.generated.h"

/** Main drive component class that attaches to a mesh to add engine effects */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftHatchComponent
	: public UStaticMeshComponent
	, public INovaAdditionalComponentInterface
{
	GENERATED_BODY()

public:
	UNovaSpacecraftHatchComponent();

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

	UPROPERTY()
	class UNovaSkeletalMeshComponent* HatchMesh;

	bool CurrentDockingState;
};
