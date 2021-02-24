// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaMeshInterface.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "NovaDecalComponent.generated.h"

/** Modified decal component that supports dynamic materials */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaDecalComponent
	: public UDecalComponent
	, public INovaMeshInterface
	, public FNovaMeshInterfaceBehavior
{
	GENERATED_BODY()

public:
	UNovaDecalComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
	}

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		FNovaMeshInterfaceBehavior::SetupMaterial(this, GetMaterial(0));
	}

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

		FNovaMeshInterfaceBehavior::TickMaterial(DeltaTime);

		SetVisibility(CurrentMaterializationTime > 0);
	}

	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) override
	{
		FNovaMeshInterfaceBehavior::SetupMaterial(this, InMaterial);
		Super::SetMaterial(0, ComponentMaterial);
	}

	virtual void Materialize(bool Force = false) override
	{
		FNovaMeshInterfaceBehavior::Materialize(Force);
	}

	virtual void Dematerialize(bool Force = false) override
	{
		FNovaMeshInterfaceBehavior::Dematerialize(Force);
	}

	virtual bool IsDematerializing() const override
	{
		return FNovaMeshInterfaceBehavior::GetMaterializationAlpha() < 1;
	}

	virtual bool IsDematerialized() const override
	{
		return FNovaMeshInterfaceBehavior::GetMaterializationAlpha() == 0;
	}

	virtual void RequestParameter(FName Name, float Value, bool Immediate = false) override
	{
		FNovaMeshInterfaceBehavior::RequestParameter(Name, Value, Immediate);
	}

	virtual void RequestParameter(FName Name, FLinearColor Value, bool Immediate = false) override
	{
		FNovaMeshInterfaceBehavior::RequestParameter(Name, Value, Immediate);
	}
};
