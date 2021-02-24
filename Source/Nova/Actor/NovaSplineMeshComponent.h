// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaMeshInterface.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"

#include "NovaSplineMeshComponent.generated.h"

/** Modified spline mesh component that supports dynamic materials */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSplineMeshComponent
	: public USplineMeshComponent
	, public INovaMeshInterface
	, public FNovaMeshInterfaceBehavior
{
	GENERATED_BODY()

public:
	UNovaSplineMeshComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
		SetRenderCustomDepth(true);
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

		bAffectDistanceFieldLighting = CurrentMaterializationTime >= 0.99f;
	}

	virtual bool SetStaticMesh(UStaticMesh* Mesh) override
	{
		bool Changed = USplineMeshComponent::SetStaticMesh(Mesh);
		FNovaMeshInterfaceBehavior::SetupMaterial(this, Mesh->GetMaterial(0));
		return Changed;
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
