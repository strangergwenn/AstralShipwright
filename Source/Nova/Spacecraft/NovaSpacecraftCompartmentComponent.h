// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraft.h"
#include "NovaSpacecraftCompartmentComponent.generated.h"


/** Module construction element */
struct FNovaModuleAssembly
{
	FNovaAssemblyElement Segment{ ENovaAssemblyElementType::Module };
	FNovaAssemblyElement ForwardBulkhead{ ENovaAssemblyElementType::Module };
	FNovaAssemblyElement AftBulkhead{ ENovaAssemblyElementType::Module };
	FNovaAssemblyElement ConnectionPiping{ ENovaAssemblyElementType::Wiring };
	FNovaAssemblyElement ConnectionWiring{ ENovaAssemblyElementType::Wiring };
};

/** Equipment construction element */
struct FNovaEquipmentAssembly
{
	FNovaAssemblyElement Equipment{ ENovaAssemblyElementType::Equipment };
};


/** Spacecraft compartment component */
UCLASS(ClassGroup = (Nova))
class UNovaSpacecraftCompartmentComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UNovaSpacecraftCompartmentComponent();

	/*----------------------------------------------------
		Compartment API
	----------------------------------------------------*/

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Set a location to go to smoothly */
	void SetRequestedLocation(const FVector& Location);

	/** Check the animation state */
	bool IsAtRequestedLocation() const
	{
		return CurrentAnimationTime >= AnimationDuration;
	}

	/** Set this compartment to immediate mode (no transitions on shader parameters) */
	void SetImmediateMode(bool Value)
	{
		ImmediateMode = Value;
	}

	/** Get the length of this compartment for positioning purposes (basically how much to increment X after it) */
	virtual FVector GetCompartmentLength(const struct FNovaCompartment& Assembly) const;

	/** Get the relative X position at which number decals should start */
	virtual FVector GetCompartmentDecalOffset(const struct FNovaCompartment& Assembly) const
	{
		return -GetCompartmentLength(Assembly) / 2 - FVector(0, 0, GetComponentLocation().Z);
	}


	/*----------------------------------------------------
		Processing methods
	----------------------------------------------------*/

public:

	/** Run a process on all compartment assembly elements */
	virtual void ProcessCompartment(const struct FNovaCompartment& Assembly, FNovaAssemblyCallback Callback);

protected:

	/** Run the process system on a single compartment module */
	void ProcessModule(FNovaModuleAssembly& Assembly, const struct FNovaCompartmentModule& Module, const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback);

	/** Run the process system on a single compartment equipment */
	void ProcessEquipment(FNovaEquipmentAssembly& Assembly, const class UNovaEquipmentDescription* Description, const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback);


	/*----------------------------------------------------
		Construction methods
	----------------------------------------------------*/

public:

	/** Create assembly elements and set the relevant locations */
	virtual void BuildCompartment(const struct FNovaCompartment& Compartment, int32 Index);

protected:

	/** Build a single module of a compartment assembly */
	void BuildModule(FNovaModuleAssembly& Assembly, const FNovaModuleSlot& Slot, const FNovaCompartment& Compartment);

	/** Build a single module of a compartment assembly */
	void BuildEquipment(FNovaEquipmentAssembly& Assembly, const UNovaEquipmentDescription* Description, const FNovaEquipmentSlot& Slot, const FNovaCompartment& Compartment);

	/** Build a single element of a compartment assembly */
	void BuildElement(FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent);


	/*----------------------------------------------------
		Helpers
	----------------------------------------------------*/

protected:

	/** Attach an element to a socket on another element */
	void AttachElementToSocket(FNovaAssemblyElement& Element, const FNovaAssemblyElement& AttachElement,
		FName SocketName, const FVector& Offset = FVector::ZeroVector);

	/** Add an animation to an element */
	void SetElementAnimation(FNovaAssemblyElement& Element, TSoftObjectPtr<UAnimationAsset> Animation);

	/** Apply an offset to an element */
	void SetElementOffset(FNovaAssemblyElement& Element, const FVector& Offset, const FRotator& Rotation = FRotator::ZeroRotator);

	/** Request a parameter value change */
	void RequestParameter(FNovaAssemblyElement& Element, FName Name, float Value);

	/** Get the length along X of a given element */
	FVector GetElementLength(const FNovaAssemblyElement& Element) const;

	/** Get the length along X of a given mesh asset */
	FVector GetElementLength(TSoftObjectPtr<UObject> Asset) const;


	/*----------------------------------------------------
		Properties
	----------------------------------------------------*/

public:

	/** Menu mode */
	UPROPERTY(EditDefaultsOnly, NoClear, BlueprintReadOnly, Category = Nova)
	float AnimationDuration;


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

public:

	// Shared data
	UPROPERTY()
	const UNovaCompartmentDescription*           Description;

protected:

	// Internal data
	FVector                                       RequestedLocation;
	FVector                                       LastLocation;
	bool                                          LocationInitialized;
	bool                                          ImmediateMode;
	float                                         CurrentAnimationTime;

	// Main elements
	FNovaAssemblyElement MainStructure{ ENovaAssemblyElementType::Structure };
	FNovaAssemblyElement OuterStructure{ ENovaAssemblyElementType::Structure };
	FNovaAssemblyElement MainPiping{ ENovaAssemblyElementType::Wiring };
	FNovaAssemblyElement MainWiring{ ENovaAssemblyElementType::Wiring };
	FNovaAssemblyElement MainHull{ ENovaAssemblyElementType::Hull };
	FNovaAssemblyElement OuterHull{ ENovaAssemblyElementType::Hull };

	// Modules & equipments
	FNovaModuleAssembly Modules[ENovaConstants::MaxModuleCount];
	FNovaEquipmentAssembly Equipments[ENovaConstants::MaxEquipmentCount];

};

