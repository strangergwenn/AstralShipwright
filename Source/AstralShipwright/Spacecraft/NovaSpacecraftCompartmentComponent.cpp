// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftCompartmentComponent.h"
#include "NovaSpacecraftPawn.h"

#include "Nova.h"

#include "Neutron/Actor/NeutronMeshInterface.h"
#include "Neutron/Actor/NeutronStaticMeshComponent.h"
#include "Neutron/Actor/NeutronSkeletalMeshComponent.h"
#include "Neutron/Actor/NeutronDecalComponent.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/NeutronUI.h"

#include "Components/PrimitiveComponent.h"
#include "Components/DecalComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
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
		FVector Location       = FMath::InterpEaseInOut(LastLocation, RequestedLocation, AnimationAlpha, ENeutronUIConstants::EaseStandard);
		SetRelativeLocation(Location);
	}

	// Handle rotation
	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(GetOwner());
	NCHECK(SpacecraftPawn);
	if (IsValid(SpacecraftPawn) && Description->RotationSpeed)
	{
		const UNeutronMenuManager* MenuManager = UNeutronMenuManager::Get();
		if (!SpacecraftPawn->IsDocked())
		{
			AddRelativeRotation(FRotator(0, 0, Description->RotationSpeed * DeltaTime));
		}
		else if (MenuManager->IsBlack())
		{
			SetRelativeRotation(FRotator::ZeroRotator);
		}
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
		return FVector(
			FMath::Max(GetElementLength(Assembly.Description->MainStructure).X, GetElementLength(Assembly.Description->FixedStructure).X),
			0, 0);
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

	const UNovaCompartmentDescription* CompDesc = Compartment.Description;

	// Process the structural elements
	ProcessElement(MainStructure, CompDesc ? CompDesc->MainStructure : nullptr);
	ProcessElement(FixedStructure, CompDesc ? CompDesc->FixedStructure : nullptr);
	ProcessElement(MainPiping, CompDesc ? CompDesc->MainPiping : nullptr);
	ProcessElement(MainWiring, CompDesc ? CompDesc->MainWiring : nullptr);
	ProcessElement(MainHull, CompDesc ? CompDesc->GetMainHull(Compartment.HullType) : nullptr);

	// Process modules
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		ProcessModule(Modules[ModuleIndex], Compartment.Modules[ModuleIndex], Compartment, Callback);
	}

	// Process equipment
	for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
	{
		ProcessEquipment(Equipment[EquipmentIndex], Compartment.Equipment[EquipmentIndex], Compartment, Callback);
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

	const UNovaCompartmentDescription* CompDesc   = Compartment.Description;
	const UNovaModuleDescription*      ModuleDesc = Module.Description;

	// Process the module elements
	ProcessElement(Assembly.SkirtStructure, CompDesc ? (Module.NeedsSkirt ? CompDesc->SkirtStructure : nullptr) : nullptr);
	ProcessElement(Assembly.Segment, ModuleDesc ? ModuleDesc->Segment : nullptr);
	ProcessElement(Assembly.ForwardBulkhead, CompDesc ? CompDesc->GetBulkhead(ModuleDesc, Module.ForwardBulkheadType, true) : nullptr);
	ProcessElement(Assembly.AftBulkhead, CompDesc ? CompDesc->GetBulkhead(ModuleDesc, Module.AftBulkheadType, false) : nullptr);
	ProcessElement(Assembly.ConnectionPiping, CompDesc ? CompDesc->GetSkirtPiping(Module.SkirtPipingType) : nullptr);
	ProcessElement(Assembly.CollectorPiping, CompDesc && Module.NeedsCollectorPiping ? CompDesc->CollectorPiping : nullptr);
	ProcessElement(Assembly.ConnectionWiring, CompDesc ? CompDesc->GetConnectionWiring(Module.NeedsConnectionWiring) : nullptr);
	ProcessElement(Assembly.DomeHull, CompDesc ? (Module.NeedsDome ? CompDesc->GetDomeHull(Compartment.HullType) : nullptr) : nullptr);
	ProcessElement(Assembly.SkirtHull, CompDesc ? (Module.NeedsSkirt ? CompDesc->GetSkirtHull(Compartment.HullType) : nullptr) : nullptr);
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
	SetElementOffset(FixedStructure, StructureOffset);
	SetElementOffset(MainPiping, StructureOffset);
	SetElementOffset(MainWiring, StructureOffset);
	SetElementOffset(MainHull, StructureOffset);

	// Build modules
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		BuildModule(Modules[ModuleIndex], Compartment.Description->GetModuleSlot(ModuleIndex), Compartment);
	}

	// Build equipment
	for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
	{
		BuildEquipment(Equipment[EquipmentIndex], Compartment.Equipment[EquipmentIndex],
			Compartment.Description->GetEquipmentSlot(EquipmentIndex), Compartment);
	}

	// Make the fixed structure independent
	if (FixedStructure.Mesh)
	{
		USceneComponent* MainStructuredMesh  = Cast<USceneComponent>(MainStructure.Mesh);
		USceneComponent* FixedStructuredMesh = Cast<USceneComponent>(FixedStructure.Mesh);
		FixedStructuredMesh->AttachToComponent(GetAttachParent(), FAttachmentTransformRules(EAttachmentRule::KeepWorld, true));
		FixedStructuredMesh->SetWorldLocation(MainStructuredMesh->GetComponentLocation());
	}
}

void UNovaSpacecraftCompartmentComponent::UpdateCustomization()
{
	UpdateCustomization(MainStructure);
	UpdateCustomization(FixedStructure);
	UpdateCustomization(MainPiping);
	UpdateCustomization(MainWiring);
	UpdateCustomization(MainHull);

	for (FNovaModuleAssembly& ModuleAssembly : Modules)
	{
		UpdateCustomization(ModuleAssembly.SkirtStructure);
		UpdateCustomization(ModuleAssembly.AftBulkhead);
		UpdateCustomization(ModuleAssembly.ForwardBulkhead);
		UpdateCustomization(ModuleAssembly.Segment);
		UpdateCustomization(ModuleAssembly.ConnectionPiping);
		UpdateCustomization(ModuleAssembly.CollectorPiping);
		UpdateCustomization(ModuleAssembly.ConnectionWiring);
		UpdateCustomization(ModuleAssembly.DomeHull);
		UpdateCustomization(ModuleAssembly.SkirtHull);
	}

	for (FNovaEquipmentAssembly& EquipmentAssembly : Equipment)
	{
		UpdateCustomization(EquipmentAssembly.Equipment);
	}
}

void UNovaSpacecraftCompartmentComponent::BuildModule(
	FNovaModuleAssembly& Assembly, const FNovaModuleSlot& Slot, const FNovaCompartment& Compartment)
{
	NCHECK(MainStructure.Mesh != nullptr);

	if (MainStructure.Mesh)
	{
		// Get offsets
		const FTransform BaseTransform   = MainStructure.Mesh->GetRelativeSocketTransform(Slot.SocketName);
		const FVector    StructureOffset = -0.5f * GetElementLength(MainStructure);
		const FVector    BulkheadOffset  = BaseTransform.GetRotation().RotateVector(0.5f * GetElementLength(Assembly.Segment));
		const FVector    Location        = BaseTransform.GetLocation() + StructureOffset;
		const FRotator   Rotation        = BaseTransform.GetRotation().Rotator();

		// Offset the module elements
		SetElementOffset(Assembly.SkirtStructure, Location, Rotation);
		SetElementOffset(Assembly.Segment, Location, Rotation);
		SetElementOffset(Assembly.ForwardBulkhead, Location + BulkheadOffset, Rotation);
		SetElementOffset(Assembly.AftBulkhead, Location - BulkheadOffset, Rotation);
		SetElementOffset(Assembly.ConnectionPiping, Location, Rotation);
		SetElementOffset(Assembly.CollectorPiping, Location, Rotation);
		SetElementOffset(Assembly.ConnectionWiring, Location, Rotation);
		SetElementOffset(Assembly.DomeHull, Location, Rotation);
		SetElementOffset(Assembly.SkirtHull, Location, Rotation);
	}
}

void UNovaSpacecraftCompartmentComponent::BuildEquipment(FNovaEquipmentAssembly& Assembly,
	const UNovaEquipmentDescription* EquipmentDescription, const FNovaEquipmentSlot& Slot, const FNovaCompartment& Compartment)
{
	if (EquipmentDescription)
	{
		NCHECK(Slot.SupportedTypes.Num() == 0 || Slot.SupportedTypes.Contains(EquipmentDescription->EquipmentType));
	}
	NCHECK(MainStructure.Mesh != nullptr);

	if (MainStructure.Mesh)
	{
		// Get offsets
		bool       IsForwardEquipment = EquipmentDescription && EquipmentDescription->EquipmentType == ENovaEquipmentType::Forward;
		FTransform BaseTransform =
			MainStructure.Mesh->GetRelativeSocketTransform(IsForwardEquipment ? Slot.ForwardSocketName : Slot.SocketName);
		FVector StructureOffset = -0.5f * FVector(FMath::Max(GetElementLength(MainStructure).X, GetElementLength(FixedStructure).X), 0, 0);

		// Offset the equipment and set the animation if any
		SetElementOffset(Assembly.Equipment, BaseTransform.GetLocation() + StructureOffset, BaseTransform.GetRotation().Rotator());
		SetElementAnimation(Assembly.Equipment, EquipmentDescription ? EquipmentDescription->SkeletalAnimation : nullptr);
	}
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
			ComponentClass = UNeutronSkeletalMeshComponent::StaticClass();
		}
		else if (Asset->IsA(UStaticMesh::StaticClass()))
		{
			ComponentClass = UNeutronStaticMeshComponent::StaticClass();
		}
		else if (Asset->IsA(UMaterialInterface::StaticClass()))
		{
			ComponentClass = UNeutronDecalComponent::StaticClass();
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
			Element.Mesh = Cast<INeutronMeshInterface>(NewObject<UPrimitiveComponent>(GetOwner(), ComponentClass));
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
				Cast<UNeutronSkeletalMeshComponent>(Element.Mesh)->SetSkeletalMesh(Cast<USkeletalMesh>(Asset.Get()));
			}
			else if (Asset->IsA(UStaticMesh::StaticClass()))
			{
				Cast<UNeutronStaticMeshComponent>(Element.Mesh)->SetStaticMesh(Cast<UStaticMesh>(Asset.Get()));
			}
			else if (Asset->IsA(UMaterialInterface::StaticClass()))
			{
				Cast<UNeutronDecalComponent>(Element.Mesh)->SetMaterial(0, Cast<UMaterialInterface>(Asset.Get()));
			}
		}

		// Setup the mesh
		if (NeedConstructing)
		{
			UPrimitiveComponent* MeshComponent = Cast<UPrimitiveComponent>(Element.Mesh);
			MeshComponent->AttachToComponent(this, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			MeshComponent->SetCollisionProfileName("Pawn");
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

	RequestParameter(Element, "DirtyIntensity", FMath::Lerp(0.15f, 0.85f, Customization.DirtyIntensity));

	if (Customization.StructuralPaint)
	{
		RequestParameter(Element, "StructuralPaintColor", Customization.StructuralPaint->PaintColor);
	}

	if (Customization.HullPaint)
	{
		RequestParameter(Element, "HullPaint", Customization.EnableHullPaint ? 1.0f : 0.0f);
		RequestParameter(Element, "HullPaintColor", Customization.HullPaint->PaintColor);
	}

	if (Customization.DetailPaint)
	{
		RequestParameter(Element, "PaintColor", Customization.DetailPaint->PaintColor);
	}

	if (Customization.Emblem && Element.Mesh && Element.Type == ENovaAssemblyElementType::Equipment)
	{
		Cast<UMaterialInstanceDynamic>(Cast<UPrimitiveComponent>(Element.Mesh)->GetMaterial(0))
			->SetTextureParameterValue("ColorDecalTexture", Customization.Emblem->Image);
	}
}

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

void UNovaSpacecraftCompartmentComponent::AttachElementToSocket(
	FNovaAssemblyElement& Element, const FNovaAssemblyElement& AttachElement, FName SocketName, const FVector& Offset)
{
	if (Element.Mesh && AttachElement.Mesh)
	{
		UNeutronStaticMeshComponent* StaticAttachMesh = Cast<UNeutronStaticMeshComponent>(AttachElement.Mesh);
		if (StaticAttachMesh)
		{
			Cast<UPrimitiveComponent>(Element.Mesh)->SetWorldLocation(StaticAttachMesh->GetSocketLocation(SocketName));
			Cast<UPrimitiveComponent>(Element.Mesh)->AddLocalOffset(Offset);
		}
	}
}

void UNovaSpacecraftCompartmentComponent::SetElementAnimation(FNovaAssemblyElement& Element, TSoftObjectPtr<UAnimationAsset> Animation)
{
	UNeutronSkeletalMeshComponent* SkeletalComponent = Cast<UNeutronSkeletalMeshComponent>(Element.Mesh);
	if (IsValid(SkeletalComponent))
	{
		UAnimSingleNodeInstance* AnimInstance = SkeletalComponent->GetSingleNodeInstance();
		if (!IsValid(AnimInstance))
		{
			SkeletalComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			AnimInstance = SkeletalComponent->GetSingleNodeInstance();
			NCHECK(AnimInstance);
		}

		AnimInstance->SetAnimationAsset(Animation.Get(), false);
		AnimInstance->SetPlaying(true);
		AnimInstance->SetPosition(ImmediateMode ? AnimInstance->GetLength() : 0.0f);
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
	UNeutronStaticMeshComponent* StaticMeshComponent = Cast<UNeutronStaticMeshComponent>(Element.Mesh);
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
	if (Asset && Asset->IsA(UStaticMesh::StaticClass()))
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
