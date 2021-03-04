// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "NovaMeshInterface.generated.h"

DECLARE_DELEGATE_RetVal_ThreeParams(bool, FInternalSetWorldLocationAndRotation, const FVector&, const FQuat&, ETeleportType);

/** CMaterial parameter change request */
struct FNovaMaterialParameterRequest
{
	FNovaMaterialParameterRequest(FName N, float V) : Name(N), Time(0), IsColor(false), FloatValue(V), ColorValue()
	{}

	FNovaMaterialParameterRequest(FName N, FLinearColor V) : Name(N), Time(0), IsColor(true), FloatValue(), ColorValue(V)
	{}

	bool operator==(const FNovaMaterialParameterRequest& Other) const
	{
		return Name == Other.Name && IsColor == Other.IsColor &&
			   (IsColor ? ColorValue == Other.ColorValue : FloatValue == Other.FloatValue);
	}

	bool operator!=(const FNovaMaterialParameterRequest& Other) const
	{
		return !operator==(Other);
	}

	FName        Name;
	float        Time;
	bool         IsColor;
	float        FloatValue;
	FLinearColor ColorValue;
};

/** Shared behavior structure for mesh objects */
USTRUCT()
struct FNovaMeshInterfaceBehavior
{
	GENERATED_BODY()

public:
	FNovaMeshInterfaceBehavior();

	/*----------------------------------------------------
	    Mesh interface
	----------------------------------------------------*/

public:
	/** Setup this behavior */
	void SetupMaterial(class UPrimitiveComponent* Mesh, class UMaterialInterface* Material);

	/** Setup this behavior */
	void SetupMaterial(class UDecalComponent* Decal, class UMaterialInterface* Material);

	/** Get the dynamic material */
	class UMaterialInstanceDynamic* GetComponentMaterial() const
	{
		return ComponentMaterial;
	}

	/** Update this behavior */
	void TickMaterial(float DeltaTime);

	/** Start materializing the mesh, or force it visible entirely */
	void Materialize(bool Force);

	/** Start de-materializing the mesh, or force it invisible entirely */
	void Dematerialize(bool Force);

	/** Get the materialization alpha */
	float GetMaterializationAlpha() const;

	/** Asynchronously set a float parameter */
	void RequestParameter(FName Name, float Value, bool Immediate = false);

	/** Asynchronously set a color parameter */
	void RequestParameter(FName Name, FLinearColor Value, bool Immediate = false);

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Time in seconds for the materialization effect
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaterializationDuration;

	// Time in seconds for a parameter change request to be processed
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float ParameterFadeDuration;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Dynamic material
	UPROPERTY()
	class UMaterialInstanceDynamic* ComponentMaterial;

	// Materialization state
	bool                                  CurrentMaterializationState;
	float                                 CurrentMaterializationTime;
	TArray<FNovaMaterialParameterRequest> CurrentRequests;
	TMap<FName, float>                    PreviousParameterFloatValues;
	TMap<FName, FLinearColor>             PreviousParameterColorValues;
};

/** Interface wrapper */
UINTERFACE(MinimalAPI, Blueprintable)
class UNovaMeshInterface : public UInterface
{
	GENERATED_BODY()
};

/** Base mesh interface for static and skeletal components */
class INovaMeshInterface
{
	GENERATED_BODY()

public:
	/*----------------------------------------------------
	    Mesh interface
	----------------------------------------------------*/

	/** Start materializing the mesh, or force it visible entirely */
	virtual void Materialize(bool Force = false) = 0;

	/** Start de-materializing the mesh, or force it invisible entirely */
	virtual void Dematerialize(bool Force = false) = 0;

	/** Check if the mesh is somewhat invisible */
	virtual bool IsDematerializing() const = 0;

	/** Check if the mesh is entirely invisible */
	virtual bool IsDematerialized() const = 0;

	/** Asynchronously set a float parameter */
	virtual void RequestParameter(FName Name, float Value, bool Immediate = false) = 0;

	/** Asynchronously set a color parameter */
	virtual void RequestParameter(FName Name, FLinearColor Value, bool Immediate = false) = 0;

	/** Check the existence of a socket */
	virtual bool HasSocket(FName SocketName) const
	{
		return false;
	}

	/** Get the relative transform for a socket */
	virtual FTransform GetRelativeSocketTransform(FName SocketName) const
	{
		return FTransform::Identity;
	}

	/** Get the extent of this object */
	virtual FVector GetExtent() const;

	/** Move a component and test for collision on the entire actor
	    Taken from UPrimitiveComponent - check for updates when updating UE ! */
	static bool MoveComponentHierarchy(UPrimitiveComponent* RootComponent, const FVector& OriginalLocation, const FVector& Delta,
		const FQuat& NewRotationQuat, bool bSweep, FHitResult* OutHit, ETeleportType Teleport,
		FInternalSetWorldLocationAndRotation MovementCallback);
};
