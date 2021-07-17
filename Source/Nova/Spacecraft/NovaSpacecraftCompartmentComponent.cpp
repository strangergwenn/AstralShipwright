// Nova project - Gwennaël Arbona

#include "NovaSpacecraftCompartmentComponent.h"
#include "NovaSpacecraftPawn.h"

#include "Nova/Actor/NovaMeshInterface.h"
#include "Nova/Actor/NovaStaticMeshComponent.h"
#include "Nova/Actor/NovaSkeletalMeshComponent.h"
#include "Nova/Actor/NovaDecalComponent.h"

#include "Nova/UI/NovaUITypes.h"
#include "Nova/Nova.h"

#include "Components/PrimitiveComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#define LOCTEXT_NAMESPACE "UNovaSpacecraftCompartmentComponent"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftCompartmentComponent::UNovaSpacecraftCompartmentComponent()
	: Super()
	, Description(nullptr)
	, RequestedLocation(FVector::ZeroVector)
	, LastLocation(FVector::ZeroVector)
	, LocationInitialized(false)
	, ImmediateMode(false)
	, CurrentAnimationTime(0)
{
	// Settings
	PrimaryComponentTick.bCanEverTick = true;
	AnimationDuration                 = 0.5f;
}

/*----------------------------------------------------
    Compartment API
----------------------------------------------------*/

void UNovaSpacecraftCompartmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Animate between locations
	if (!IsAtRequestedLocation())
	{
		CurrentAnimationTime += DeltaTime;
		float   AnimationAlpha = FMath::Clamp(CurrentAnimationTime / AnimationDuration, 0.0f, 1.0f);
		FVector Location       = FMath::InterpEaseInOut(LastLocation, RequestedLocation, AnimationAlpha, ENovaUIConstants::EaseStandard);
		SetRelativeLocation(Location);
	}
}

void UNovaSpacecraftCompartmentComponent::SetRequestedLocation(const FVector& Location)
{
	RequestedLocation = Location;

	if (LocationInitialized && !ImmediateMode)
	{
		LastLocation         = GetRelativeLocation();
		CurrentAnimationTime = 0;
	}
	else
	{
		SetRelativeLocation(RequestedLocation);
		LocationInitialized = true;
	}
}

FVector UNovaSpacecraftCompartmentComponent::GetCompartmentLength(const struct FNovaCompartment& Assembly) const
{
	if (Assembly.Description)
	{
		return GetElementLength(Assembly.Description->MainStructure);
	}
	else
	{
		return FVector::ZeroVector;
	}
}

/*----------------------------------------------------
    Processing methods
----------------------------------------------------*/

void UNovaSpacecraftCompartmentComponent::ProcessCompartment(const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback)
{
	auto ProcessElement = [&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset,
							  FNovaAdditionalComponent AdditionalComponent = FNovaAdditionalComponent())
	{
		Callback.Execute(Element, Asset, AdditionalComponent);
	};

	// Process the structural elements
	const UNovaCompartmentDescription* CompartmentDescription = Compartment.Description;
	ProcessElement(MainStructure, CompartmentDescription ? CompartmentDescription->MainStructure : nullptr);
	ProcessElement(OuterStructure,
		CompartmentDescription ? (Compartment.NeedsOuterSkirt ? CompartmentDescription->OuterStructure : nullptr) : nullptr);
	ProcessElement(MainPiping, CompartmentDescription ? CompartmentDescription->GetMainPiping(Compartment.NeedsMainPiping) : nullptr);
	ProcessElement(MainWiring, CompartmentDescription ? CompartmentDescription->GetMainWiring(Compartment.NeedsMainWiring) : nullptr);
	ProcessElement(MainHull, CompartmentDescription ? CompartmentDescription->GetMainHull(Compartment.HullType) : nullptr);
	ProcessElement(OuterHull, CompartmentDescription
								  ? (Compartment.NeedsOuterSkirt ? CompartmentDescription->GetOuterHull(Compartment.HullType) : nullptr)
								  : nullptr);

	// Process modules
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		ProcessModule(Modules[ModuleIndex], Compartment.Modules[ModuleIndex], Compartment, Callback);
	}

	// Process equipments
	for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
	{
		ProcessEquipment(Equipments[EquipmentIndex], Compartment.Equipments[EquipmentIndex], Compartment, Callback);
	}
}

void UNovaSpacecraftCompartmentComponent::ProcessModule(FNovaModuleAssembly& Assembly, const FNovaCompartmentModule& Module,
	const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback)
{
	auto ProcessElement = [&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset,
							  FNovaAdditionalComponent AdditionalComponent = FNovaAdditionalComponent())
	{
		Callback.Execute(Element, Asset, AdditionalComponent);
	};

	const UNovaCompartmentDescription* CompartmentDescription = Compartment.Description;
	const UNovaModuleDescription*      ModuleDescription      = Module.Description;

	// Process the module elements
	ProcessElement(Assembly.Segment, ModuleDescription ? ModuleDescription->Segment : nullptr);
	ProcessElement(
		Assembly.ForwardBulkhead, ModuleDescription ? ModuleDescription->GetBulkhead(Module.ForwardBulkheadType, true) : nullptr);
	ProcessElement(Assembly.AftBulkhead, ModuleDescription ? ModuleDescription->GetBulkhead(Module.AftBulkheadType, false) : nullptr);
	ProcessElement(
		Assembly.ConnectionPiping, CompartmentDescription ? CompartmentDescription->GetSkirtPiping(Module.SkirtPipingType) : nullptr);
	ProcessElement(
		Assembly.ConnectionWiring, CompartmentDescription ? CompartmentDescription->GetConnectionWiring(Module.NeedsWiring) : nullptr);
}

void UNovaSpacecraftCompartmentComponent::ProcessEquipment(FNovaEquipmentAssembly& Assembly,
	const UNovaEquipmentDescription* EquipmentDescription, const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback)
{
	auto ProcessElement = [&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset,
							  FNovaAdditionalComponent AdditionalComponent = FNovaAdditionalComponent())
	{
		Callback.Execute(Element, Asset, AdditionalComponent);
	};

	// Process the equipment elements
	ProcessElement(Assembly.Equipment, EquipmentDescription ? EquipmentDescription->GetMesh() : nullptr,
		EquipmentDescription ? EquipmentDescription->AdditionalComponent : FNovaAdditionalComponent());
}

/*----------------------------------------------------
    Construction methods
----------------------------------------------------*/

void UNovaSpacecraftCompartmentComponent::BuildCompartment(const struct FNovaCompartment& Compartment, int32 Index)
{
	// Build all elements first for the most general and basic setup
	ProcessCompartment(
		Compartment, FNovaAssemblyCallback::CreateLambda(
						 [&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
						 {
							 BuildElement(Element, Asset, AdditionalComponent);
						 }));

	// Build structure & hull
	FVector StructureOffset = -0.5f * GetElementLength(MainStructure);
	SetElementOffset(MainStructure, StructureOffset);
	SetElementOffset(OuterStructure, StructureOffset);
	SetElementOffset(MainPiping, StructureOffset);
	SetElementOffset(MainWiring, StructureOffset);
	SetElementOffset(MainHull, StructureOffset);
	SetElementOffset(OuterHull, StructureOffset);

	// Setup material
	RequestParameter(MainHull, "AlternateHull", Compartment.HullType == ENovaHullType::MetalFabric);
	RequestParameter(OuterHull, "AlternateHull", Compartment.HullType == ENovaHullType::MetalFabric);

	// Build modules
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		BuildModule(Modules[ModuleIndex], Compartment.Description->GetModuleSlot(ModuleIndex), Compartment);
	}

	// Build equipments
	for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
	{
		BuildEquipment(Equipments[EquipmentIndex], Compartment.Equipments[EquipmentIndex],
			Compartment.Description->GetEquipmentSlot(EquipmentIndex), Compartment);
	}
}

void UNovaSpacecraftCompartmentComponent::UpdateCustomization()
{
	UpdateCustomization(MainStructure);
	UpdateCustomization(OuterStructure);
	UpdateCustomization(MainPiping);
	UpdateCustomization(MainWiring);
	UpdateCustomization(MainHull);
	UpdateCustomization(OuterHull);
}

void UNovaSpacecraftCompartmentComponent::BuildModule(
	FNovaModuleAssembly& Assembly, const FNovaModuleSlot& Slot, const FNovaCompartment& Compartment)
{
	NCHECK(MainStructure.Mesh != nullptr);

	// Get offsets
	FTransform BaseTransform   = MainStructure.Mesh->GetRelativeSocketTransform(Slot.SocketName);
	FVector    StructureOffset = -0.5f * GetElementLength(MainStructure);
	FVector    BulkheadOffset  = BaseTransform.GetRotation().RotateVector(0.5f * GetElementLength(Assembly.Segment));
	FRotator   Rotation        = BaseTransform.GetRotation().Rotator();

	// Offset the module elements
	SetElementOffset(Assembly.Segment, BaseTransform.GetLocation() + StructureOffset, Rotation);
	SetElementOffset(Assembly.ForwardBulkhead, BaseTransform.GetLocation() + StructureOffset + BulkheadOffset, Rotation);
	SetElementOffset(Assembly.AftBulkhead, BaseTransform.GetLocation() + StructureOffset - BulkheadOffset, Rotation);
	SetElementOffset(Assembly.ConnectionPiping, BaseTransform.GetLocation() + StructureOffset, Rotation);
	SetElementOffset(Assembly.ConnectionWiring, BaseTransform.GetLocation() + StructureOffset, Rotation);
}

void UNovaSpacecraftCompartmentComponent::BuildEquipment(FNovaEquipmentAssembly& Assembly,
	const UNovaEquipmentDescription* EquipmentDescription, const FNovaEquipmentSlot& Slot, const FNovaCompartment& Compartment)
{
	if (EquipmentDescription)
	{
		NCHECK(Slot.SupportedTypes.Num() == 0 || Slot.SupportedTypes.Contains(EquipmentDescription->EquipmentType));
	}
	NCHECK(MainStructure.Mesh != nullptr);

	// Get offsets
	FTransform BaseTransform   = MainStructure.Mesh->GetRelativeSocketTransform(Slot.SocketName);
	FVector    StructureOffset = -0.5f * GetElementLength(MainStructure);

	// Offset the equipment and set the animation if any
	SetElementOffset(Assembly.Equipment, BaseTransform.GetLocation() + StructureOffset, BaseTransform.GetRotation().Rotator());
	SetElementAnimation(Assembly.Equipment, EquipmentDescription ? EquipmentDescription->SkeletalAnimation : nullptr);
}

void UNovaSpacecraftCompartmentComponent::BuildElement(
	FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
{
	if (Element.Mesh)
	{
		NCHECK(Cast<UPrimitiveComponent>(Element.Mesh));
	}
	UPrimitiveComponent* PrimitiveMesh = Cast<UPrimitiveComponent>(Element.Mesh);

	// Determine the target component class
	TSubclassOf<UPrimitiveComponent> ComponentClass = nullptr;
	if (Asset.IsValid())
	{
		if (Asset->IsA(USkeletalMesh::StaticClass()))
		{
			ComponentClass = UNovaSkeletalMeshComponent::StaticClass();
		}
		else if (Asset->IsA(UStaticMesh::StaticClass()))
		{
			ComponentClass = UNovaStaticMeshComponent::StaticClass();
		}
		else if (Asset->IsA(UMaterialInterface::StaticClass()))
		{
			ComponentClass = UNovaDecalComponent::StaticClass();
		}
	}

	// Detect whether we need to construct or re-construct the additional component
	INovaAdditionalComponentInterface* AdditionalComponentInterface      = nullptr;
	bool                               NeedConstructingAdditionalElement = AdditionalComponent.ComponentClass.Get() != nullptr;
	if (PrimitiveMesh)
	{
		TArray<USceneComponent*> ChildComponents;
		PrimitiveMesh->GetChildrenComponents(false, ChildComponents);
		for (USceneComponent* ChildComponent : ChildComponents)
		{
			if (ChildComponent->Implements<UNovaAdditionalComponentInterface>())
			{
				AdditionalComponentInterface = Cast<INovaAdditionalComponentInterface>(ChildComponent);
				if (ChildComponent->GetClass() == AdditionalComponent.ComponentClass.Get())
				{
					NeedConstructingAdditionalElement = false;
				}
				else
				{
					TArray<USceneComponent*> AdditionalChildComponents;
					ChildComponent->GetChildrenComponents(false, AdditionalChildComponents);
					for (USceneComponent* AdditionalChildComponent : AdditionalChildComponents)
					{
						AdditionalChildComponent->DestroyComponent();
					}
					ChildComponent->DestroyComponent();
					AdditionalComponentInterface = nullptr;
				}

				break;
			}
		}
	}

	// Detect whether we need to construct or re-construct the mesh
	bool NeedConstructing = Element.Mesh == nullptr;
	if (PrimitiveMesh && PrimitiveMesh->GetClass() != ComponentClass)
	{
		PrimitiveMesh->DestroyComponent();
		NeedConstructing = true;
	}

	// Build the component now that the cleanup is done, if the component class is valid
	if (ComponentClass.Get())
	{
		// Create the element mesh
		if (NeedConstructing)
		{
			Element.Mesh = Cast<INovaMeshInterface>(NewObject<UPrimitiveComponent>(GetOwner(), ComponentClass));
			NCHECK(Element.Mesh);

			if (AdditionalComponent.ComponentClass.Get())
			{
				NeedConstructingAdditionalElement = true;
			}
		}

		// Set the resource
		Element.Asset = Asset.ToSoftObjectPath();
		if (Asset.IsValid())
		{
			NCHECK(Element.Mesh);
			if (Asset->IsA(USkeletalMesh::StaticClass()))
			{
				Cast<UNovaSkeletalMeshComponent>(Element.Mesh)->SetSkeletalMesh(Cast<USkeletalMesh>(Asset.Get()));
			}
			else if (Asset->IsA(UStaticMesh::StaticClass()))
			{
				Cast<UNovaStaticMeshComponent>(Element.Mesh)->SetStaticMesh(Cast<UStaticMesh>(Asset.Get()));
			}
			else if (Asset->IsA(UMaterialInterface::StaticClass()))
			{
				Cast<UNovaDecalComponent>(Element.Mesh)->SetMaterial(0, Cast<UMaterialInterface>(Asset.Get()));
			}
		}

		// Setup the mesh
		if (NeedConstructing)
		{
			UPrimitiveComponent* MeshComponent = Cast<UPrimitiveComponent>(Element.Mesh);
			MeshComponent->AttachToComponent(this, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			MeshComponent->SetCollisionProfileName("Pawn");
			MeshComponent->SetRenderCustomDepth(true);
			MeshComponent->bCastShadowAsTwoSided = true;
			MeshComponent->RegisterComponent();
		}

		// Setup the additional component
		if (NeedConstructingAdditionalElement)
		{
			USceneComponent* AdditionalMeshComponent = NewObject<USceneComponent>(GetOwner(), AdditionalComponent.ComponentClass);
			NCHECK(IsValid(AdditionalMeshComponent));
			AdditionalComponentInterface = Cast<INovaAdditionalComponentInterface>(AdditionalMeshComponent);
			NCHECK(AdditionalComponentInterface);
		}
		if (AdditionalComponentInterface)
		{
			AdditionalComponentInterface->SetAdditionalAsset(AdditionalComponent.AdditionalAsset);
		}
		if (NeedConstructingAdditionalElement)
		{
			UPrimitiveComponent* MeshComponent = Cast<UPrimitiveComponent>(Element.Mesh);
			NCHECK(MeshComponent);
			USceneComponent* AdditionalMeshComponent = Cast<USceneComponent>(AdditionalComponentInterface);
			AdditionalMeshComponent->AttachToComponent(
				MeshComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), AdditionalComponent.SocketName);
			AdditionalMeshComponent->RegisterComponent();
		}
	}

	UpdateCustomization(Element);
}

void UNovaSpacecraftCompartmentComponent::UpdateCustomization(FNovaAssemblyElement& Element)
{
	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(GetOwner());
	NCHECK(SpacecraftPawn);
	const FNovaSpacecraftCustomization& Customization = SpacecraftPawn->GetCustomization();

	RequestParameter(Element, "DirtyIntensity", FMath::Lerp(0.5f, 1.0f, Customization.DirtyIntensity));
	RequestParameter(Element, "StructuralPaint", Customization.StructuralPaint->Unpainted ? 0.0f : 1.0f);
	RequestParameter(Element, "StructuralPaintColor", Customization.StructuralPaint->PaintColor);
	RequestParameter(Element, "WireColor", Customization.WirePaint->PaintColor);
}

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

void UNovaSpacecraftCompartmentComponent::AttachElementToSocket(
	FNovaAssemblyElement& Element, const FNovaAssemblyElement& AttachElement, FName SocketName, const FVector& Offset)
{
	if (Element.Mesh && AttachElement.Mesh)
	{
		UNovaStaticMeshComponent* StaticAttachMesh = Cast<UNovaStaticMeshComponent>(AttachElement.Mesh);
		if (StaticAttachMesh)
		{
			Cast<UPrimitiveComponent>(Element.Mesh)->SetWorldLocation(StaticAttachMesh->GetSocketLocation(SocketName));
			Cast<UPrimitiveComponent>(Element.Mesh)->AddLocalOffset(Offset);
		}
	}
}

void UNovaSpacecraftCompartmentComponent::SetElementAnimation(FNovaAssemblyElement& Element, TSoftObjectPtr<UAnimationAsset> Animation)
{
	if (Element.Mesh)
	{
		UNovaSkeletalMeshComponent* SkeletalComponent = Cast<UNovaSkeletalMeshComponent>(Element.Mesh);

		if (SkeletalComponent && SkeletalComponent->GetAnimInstance() == nullptr)
		{
			SkeletalComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			SkeletalComponent->SetAnimation(Animation.Get());
			SkeletalComponent->Play(false);
		}
	}
}

void UNovaSpacecraftCompartmentComponent::SetElementOffset(FNovaAssemblyElement& Element, const FVector& Offset, const FRotator& Rotation)
{
	if (Element.Mesh)
	{
		Cast<UPrimitiveComponent>(Element.Mesh)->SetRelativeLocation(Offset);
		Cast<UPrimitiveComponent>(Element.Mesh)->SetRelativeRotation(Rotation);
	}
}

void UNovaSpacecraftCompartmentComponent::RequestParameter(FNovaAssemblyElement& Element, FName Name, float Value)
{
	if (Element.Mesh)
	{
		Element.Mesh->RequestParameter(Name, Value, ImmediateMode);
	}
}

void UNovaSpacecraftCompartmentComponent::RequestParameter(FNovaAssemblyElement& Element, FName Name, FLinearColor Value)
{
	if (Element.Mesh)
	{
		Element.Mesh->RequestParameter(Name, Value, ImmediateMode);
	}
}

FVector UNovaSpacecraftCompartmentComponent::GetElementLength(const FNovaAssemblyElement& Element) const
{
	UNovaStaticMeshComponent* StaticMeshComponent = Cast<UNovaStaticMeshComponent>(Element.Mesh);
	if (StaticMeshComponent)
	{
		FVector Min, Max;
		StaticMeshComponent->GetLocalBounds(Min, Max);
		return FVector((Max - Min).X, 0, 0);
	}
	else
	{
		return FVector::ZeroVector;
	}
}

FVector UNovaSpacecraftCompartmentComponent::GetElementLength(TSoftObjectPtr<UObject> Asset) const
{
	NCHECK(Asset.IsValid());

	if (Asset->IsA(UStaticMesh::StaticClass()))
	{
		FBoxSphereBounds MeshBounds = Cast<UStaticMesh>(Asset.Get())->GetBounds();
		FVector          Min        = MeshBounds.Origin - MeshBounds.BoxExtent;
		FVector          Max        = MeshBounds.Origin + MeshBounds.BoxExtent;

		return FVector((Max - Min).X, 0, 0);
	}
	else
	{
		return FVector::ZeroVector;
	}
}

#undef LOCTEXT_NAMESPACE
