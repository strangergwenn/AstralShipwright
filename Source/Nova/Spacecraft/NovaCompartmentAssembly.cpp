// Nova project - Gwennaël Arbona

#include "NovaCompartmentAssembly.h"

#include "Nova/Actor/NovaMeshInterface.h"
#include "Nova/Actor/NovaStaticMeshComponent.h"
#include "Nova/Actor/NovaSkeletalMeshComponent.h"
#include "Nova/Actor/NovaDecalComponent.h"

#include "Nova/UI/NovaUITypes.h"
#include "Nova/Nova.h"

#include "Components/PrimitiveComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"


#define LOCTEXT_NAMESPACE "FNovaCompartment"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UNovaCompartmentAssembly::UNovaCompartmentAssembly()
	: Description(nullptr)
	, RequestedLocation(FVector::ZeroVector)
	, LastLocation(FVector::ZeroVector)
	, LocationInitialized(false)
	, ImmediateMode(false)
	, CurrentAnimationTime(0)
{
	// Get empty mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> InvisibleMeshRef(TEXT("/Game/Master/Meshes/SM_Invisible"));
	EmptyMesh = InvisibleMeshRef.Object;

	// Settings
	PrimaryComponentTick.bCanEverTick = true;
	AnimationDuration = 0.5f;
}


/*----------------------------------------------------
	Compartment API
----------------------------------------------------*/

void UNovaCompartmentAssembly::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Animate between locations
	if (!IsAtRequestedLocation())
	{
		CurrentAnimationTime += DeltaTime;
		float AnimationAlpha = FMath::Clamp(CurrentAnimationTime / AnimationDuration, 0.0f, 1.0f);
		FVector Location = FMath::InterpEaseInOut(LastLocation, RequestedLocation, AnimationAlpha, ENovaUIConstants::EaseStandard);
		SetRelativeLocation(Location);
	}
}

void UNovaCompartmentAssembly::SetRequestedLocation(const FVector& Location)
{
	RequestedLocation = Location;

	if (LocationInitialized && !ImmediateMode)
	{
		LastLocation = GetRelativeLocation();
		CurrentAnimationTime = 0;
	}
	else
	{
		SetRelativeLocation(RequestedLocation);
		LocationInitialized = true;
	}
}

FVector UNovaCompartmentAssembly::GetCompartmentLength(const struct FNovaCompartment& Assembly) const
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

void UNovaCompartmentAssembly::ProcessCompartment(const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback)
{
	auto ProcessElement = [=](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset)
	{
		Callback.Execute(Element, Asset);
	};

	// Process the structural elements
	const UNovaCompartmentDescription* CompartmentDescription = Compartment.Description;
	ProcessElement(MainStructure, CompartmentDescription ? CompartmentDescription->MainStructure : nullptr);
	ProcessElement(OuterStructure, CompartmentDescription ? (Compartment.NeedsOuterSkirt ? CompartmentDescription->OuterStructure : EmptyMesh) : nullptr);
	ProcessElement(MainPiping, CompartmentDescription ? CompartmentDescription->GetMainPiping(Compartment.NeedsMainPiping) : nullptr);
	ProcessElement(MainWiring, CompartmentDescription ? CompartmentDescription->GetMainWiring(Compartment.NeedsMainWiring) : nullptr);
	ProcessElement(MainHull, CompartmentDescription ? CompartmentDescription->GetMainHull(Compartment.HullType) : nullptr);
	ProcessElement(OuterHull, CompartmentDescription ? (Compartment.NeedsOuterSkirt ? CompartmentDescription->GetOuterHull(Compartment.HullType) : EmptyMesh) : nullptr);

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

void UNovaCompartmentAssembly::ProcessModule(FNovaModuleAssembly& Assembly, const FNovaCompartmentModule& Module, const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback)
{
	auto ProcessElement = [=](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset)
	{
		Callback.Execute(Element, Asset);
	};

	const UNovaCompartmentDescription* CompartmentDescription = Compartment.Description;
	const UNovaModuleDescription* ModuleDescription = Module.Description;

	// Process the module elements
	ProcessElement(Assembly.Segment, ModuleDescription ? ModuleDescription->Segment : EmptyMesh);
	ProcessElement(Assembly.ForwardBulkhead, ModuleDescription ? ModuleDescription->GetBulkhead(Module.ForwardBulkheadType, true) : EmptyMesh);
	ProcessElement(Assembly.AftBulkhead, ModuleDescription ? ModuleDescription->GetBulkhead(Module.AftBulkheadType, false) : EmptyMesh);
	ProcessElement(Assembly.ConnectionPiping, CompartmentDescription ? CompartmentDescription->GetSkirtPiping(Module.SkirtPipingType) : nullptr);
	ProcessElement(Assembly.ConnectionWiring, CompartmentDescription ? CompartmentDescription->GetConnectionWiring(Module.NeedsWiring) : nullptr);
}

void UNovaCompartmentAssembly::ProcessEquipment(FNovaEquipmentAssembly& Assembly, const UNovaEquipmentDescription* EquipmentDescription, const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback)
{
	auto ProcessElement = [=](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset)
	{
		Callback.Execute(Element, Asset);
	};

	// Process the equipment elements
	ProcessElement(Assembly.Equipment, EquipmentDescription ? EquipmentDescription->GetMesh() : EmptyMesh);
}


/*----------------------------------------------------
	Construction methods
----------------------------------------------------*/

void UNovaCompartmentAssembly::BuildCompartment(const struct FNovaCompartment& Compartment, int32 Index)
{
	// Build all elements first for the most general and basic setup
	ProcessCompartment(Compartment,
		FNovaAssemblyCallback::CreateLambda([=](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset)
			{
				BuildElement(Element, Asset);
			})
	);

	// Build structure & hull
	FVector StructureOffset = -0.5f * GetElementLength(MainStructure);
	SetElementOffset(MainStructure, StructureOffset);
	SetElementOffset(OuterStructure, StructureOffset);
	SetElementOffset(MainPiping, StructureOffset);
	SetElementOffset(MainWiring, StructureOffset);
	SetElementOffset(MainHull, StructureOffset);
	SetElementOffset(OuterHull, StructureOffset);

	// Setup material
	RequestParameter(MainHull, "AlternateHull", Compartment.HullType == ENovaAssemblyHullType::MetalFabric);
	RequestParameter(OuterHull, "AlternateHull", Compartment.HullType == ENovaAssemblyHullType::MetalFabric);

	// Build modules
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		BuildModule(Modules[ModuleIndex], Compartment.Description->GetModuleSlot(ModuleIndex), Compartment);
	}

	// Build equipments
	for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
	{
		BuildEquipment(Equipments[EquipmentIndex], Compartment.Equipments[EquipmentIndex], Compartment.Description->GetEquipmentSlot(EquipmentIndex), Compartment);
	}
}

void UNovaCompartmentAssembly::BuildModule(FNovaModuleAssembly& Assembly, const FNovaModuleSlot& Slot, const FNovaCompartment& Compartment)
{
	NCHECK(MainStructure.Mesh != nullptr);

	// Get offsets
	FTransform BaseTransform = MainStructure.Mesh->GetRelativeSocketTransform(Slot.SocketName);
	FVector StructureOffset = -0.5f * GetElementLength(MainStructure);
	FVector BulkheadOffset = BaseTransform.GetRotation().RotateVector(0.5f * GetElementLength(Assembly.Segment));
	FRotator Rotation = BaseTransform.GetRotation().Rotator();

	// Offset the module elements
	SetElementOffset(Assembly.Segment, BaseTransform.GetLocation() + StructureOffset, Rotation);
	SetElementOffset(Assembly.ForwardBulkhead, BaseTransform.GetLocation() + StructureOffset + BulkheadOffset, Rotation);
	SetElementOffset(Assembly.AftBulkhead, BaseTransform.GetLocation() + StructureOffset - BulkheadOffset, Rotation);
	SetElementOffset(Assembly.ConnectionPiping, BaseTransform.GetLocation() + StructureOffset, Rotation);
	SetElementOffset(Assembly.ConnectionWiring, BaseTransform.GetLocation() + StructureOffset, Rotation);
}

void UNovaCompartmentAssembly::BuildEquipment(FNovaEquipmentAssembly& Assembly, const UNovaEquipmentDescription* EquipmentDescription, const FNovaEquipmentSlot& Slot, const FNovaCompartment& Compartment)
{
	if (EquipmentDescription)
	{
		NCHECK(Slot.SupportedTypes.Num() == 0 || Slot.SupportedTypes.Contains(EquipmentDescription->EquipmentType));
	}
	NCHECK(MainStructure.Mesh != nullptr);

	// Get offsets
	FTransform BaseTransform = MainStructure.Mesh->GetRelativeSocketTransform(Slot.SocketName);
	FVector StructureOffset = -0.5f * GetElementLength(MainStructure);

	// Offset the equipment and set the animation if any
	SetElementOffset(Assembly.Equipment, BaseTransform.GetLocation() + StructureOffset, BaseTransform.GetRotation().Rotator());
	SetElementAnimation(Assembly.Equipment, EquipmentDescription ? EquipmentDescription->SkeletalAnimation : nullptr);
}

void UNovaCompartmentAssembly::BuildElement(FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset)
{
	NCHECK(Asset.IsValid());

	// Detect whether we need to construct or re-construct the mesh
	bool NeedConstructing = Element.Mesh == nullptr;
	if ((Asset->IsA(USkeletalMesh::StaticClass()) && !Cast<UNovaSkeletalMeshComponent>(Element.Mesh))
		|| (Asset->IsA(UStaticMesh::StaticClass()) && !Cast<UNovaStaticMeshComponent>(Element.Mesh))
		|| (Asset->IsA(UMaterialInterface::StaticClass()) && !Cast<UNovaDecalComponent>(Element.Mesh)))
	{
		if (Element.Mesh)
		{
			Cast<USceneComponent>(Element.Mesh)->DestroyComponent();
		}
		NeedConstructing = true;
	}

	// Create the element mesh
	if (NeedConstructing)
	{
		if (Asset->IsA(USkeletalMesh::StaticClass()))
		{
			Element.Mesh = NewObject<UNovaSkeletalMeshComponent>(GetOwner());
		}
		else if (Asset->IsA(UStaticMesh::StaticClass()))
		{
			Element.Mesh = NewObject<UNovaStaticMeshComponent>(GetOwner());
		}
		else
		{
			Element.Mesh = NewObject<UNovaDecalComponent>(GetOwner());
		}
	}

	// Set the resource
	NCHECK(Element.Mesh);
	Element.Asset = Asset.ToSoftObjectPath();
	if (Asset->IsA(USkeletalMesh::StaticClass()))
	{
		Cast<UNovaSkeletalMeshComponent>(Element.Mesh)->SetSkeletalMesh(Cast<USkeletalMesh>(Asset.Get()));
	}
	else if (Asset->IsA(UStaticMesh::StaticClass()))
	{
		Cast<UNovaStaticMeshComponent>(Element.Mesh)->SetStaticMesh(Cast<UStaticMesh>(Asset.Get()));
	}
	else
	{
		Cast<UNovaDecalComponent>(Element.Mesh)->SetMaterial(0, Cast<UMaterialInterface>(Asset.Get()));
	}

	// Setup the mesh
	if (NeedConstructing)
	{
		Cast<USceneComponent>(Element.Mesh)->AttachToComponent(this,
			FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));

		UPrimitiveComponent* MeshComponent = Cast<UPrimitiveComponent>(Element.Mesh);
		if (MeshComponent)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			MeshComponent->SetCollisionProfileName("Pawn");
			MeshComponent->SetRenderCustomDepth(true);
			MeshComponent->bCastShadowAsTwoSided = true;
		}

		Cast<USceneComponent>(Element.Mesh)->RegisterComponent();
	}
}


/*----------------------------------------------------
	Helpers
----------------------------------------------------*/

void UNovaCompartmentAssembly::AttachElementToSocket(FNovaAssemblyElement& Element, const FNovaAssemblyElement& AttachElement,
	FName SocketName, const FVector& Offset)
{
	NCHECK(Element.Mesh);
	NCHECK(AttachElement.Mesh);

	UNovaStaticMeshComponent* StaticAttachMesh = Cast<UNovaStaticMeshComponent>(AttachElement.Mesh);
	if (StaticAttachMesh)
	{
		Cast<USceneComponent>(Element.Mesh)->SetWorldLocation(StaticAttachMesh->GetSocketLocation(SocketName));
		Cast<USceneComponent>(Element.Mesh)->AddLocalOffset(Offset);
	}
}

void UNovaCompartmentAssembly::SetElementAnimation(FNovaAssemblyElement& Element, TSoftObjectPtr<UAnimationAsset> Animation)
{
	NCHECK(Element.Mesh);
	UNovaSkeletalMeshComponent* SkeletalComponent = Cast<UNovaSkeletalMeshComponent>(Element.Mesh);

	if (SkeletalComponent && SkeletalComponent->GetAnimInstance() == nullptr)
	{
		SkeletalComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		SkeletalComponent->SetAnimation(Animation.Get());
		SkeletalComponent->Play(false);
	}
}

void UNovaCompartmentAssembly::SetElementOffset(FNovaAssemblyElement& Element, const FVector& Offset, const FRotator& Rotation)
{
	NCHECK(Element.Mesh);

	Cast<USceneComponent>(Element.Mesh)->SetRelativeLocation(Offset);
	Cast<USceneComponent>(Element.Mesh)->SetRelativeRotation(Rotation);
}

void UNovaCompartmentAssembly::RequestParameter(FNovaAssemblyElement& Element, FName Name, float Value)
{
	Element.Mesh->RequestParameter(Name, Value, ImmediateMode);
}

FVector UNovaCompartmentAssembly::GetElementLength(const FNovaAssemblyElement& Element) const
{
	NCHECK(Element.Mesh);

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

FVector UNovaCompartmentAssembly::GetElementLength(TSoftObjectPtr<UObject> Asset) const
{
	NCHECK(Asset.IsValid());

	if (Asset->IsA(UStaticMesh::StaticClass()))
	{
		FBoxSphereBounds MeshBounds = Cast<UStaticMesh>(Asset.Get())->GetBounds();
		FVector Min = MeshBounds.Origin - MeshBounds.BoxExtent;
		FVector Max = MeshBounds.Origin + MeshBounds.BoxExtent;

		return FVector((Max - Min).X, 0, 0);
	}
	else
	{
		return FVector::ZeroVector;
	}
}


#undef LOCTEXT_NAMESPACE
