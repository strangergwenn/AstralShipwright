// Nova project - Gwennaël Arbona

#include "NovaCaptureActor.h"
#include "NovaSpacecraftAssembly.h"
#include "NovaCompartmentAssembly.h"

#include "Nova/Actor/NovaStaticMeshComponent.h"
#include "Nova/Actor/NovaSkeletalMeshComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Game/NovaGameTypes.h"
#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/UI/NovaUI.h"
#include "Nova/Nova.h"

#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneCaptureComponent2D.h"

#if WITH_EDITOR

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "AssetRegistryModule.h"
#include "ObjectTools.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#endif


#define LOCTEXT_NAMESPACE "ANovaCaptureActor"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

ANovaCaptureActor::ANovaCaptureActor()
	: Super()
	, Assembly(nullptr)
	, Catalog(nullptr)
{
	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->bIsEditorOnly = true;

	// Create camera arm
	CameraArmComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CameraArm"));
	CameraArmComponent->SetupAttachment(RootComponent);
	CameraArmComponent->bIsEditorOnly = true;

	// Create camera component
	CameraCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CameraCapture"));
	CameraCapture->SetupAttachment(CameraArmComponent, USpringArmComponent::SocketName);
	CameraCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	CameraCapture->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;
	CameraCapture->bCaptureEveryFrame = false;
	CameraCapture->bCaptureOnMovement = false;
	CameraCapture->ShowFlags.EnableAdvancedFeatures();
	CameraCapture->ShowFlags.SetFog(false);
	CameraCapture->FOVAngle = 70;
	CameraCapture->bIsEditorOnly = true;
	CameraCapture->bAlwaysPersistRenderingState = true;
	CameraCapture->bUseRayTracingIfEnabled = true;

	// Defaults
	bIsEditorOnlyActor = true;
	RenderUpscaleFactor = 4;
	ResultUpscaleFactor = 2;
}

/*----------------------------------------------------
	Asset screenshot system
----------------------------------------------------*/

void ANovaCaptureActor::RenderAsset(UNovaAssetDescription* Asset, FSlateBrush& AssetRender)
{
#if WITH_EDITOR

	NCHECK(Asset);
	NLOG("ANovaCaptureActor::CaptureScreenshot for '%s'", *GetName());

	// Get required objects
	CreateAssembly();
	CreateCatalog();
	CreateRenderTarget();

	// Delete previous content
	if (AssetRender.GetResourceObject())
	{
		TArray<UObject*> AssetsToDelete;
		AssetsToDelete.Add(AssetRender.GetResourceObject());
		ObjectTools::ForceDeleteObjects(AssetsToDelete, false);
	}

	// Build path
	int32 SeparatorIndex;
	FString ScreenshotPath = Asset->GetOuter()->GetFName().ToString();
	if (ScreenshotPath.FindLastChar('/', SeparatorIndex))
	{
		ScreenshotPath.InsertAt(SeparatorIndex + 1, TEXT("T_"));
	}

	// Define the content to load
	CurrentAssembly = MakeShareable(new FNovaSpacecraft);
	if (Asset->IsA(UNovaCompartmentDescription::StaticClass()))
	{
		CurrentAssembly->Compartments.Add(FNovaCompartment(Cast<UNovaCompartmentDescription>(Asset)));
	}
	else if (Asset->IsA(UNovaModuleDescription::StaticClass()))
	{
		FNovaCompartment CompartmentAssembly(EmptyCompartmentDescription);
		CompartmentAssembly.Modules[0].Description = Cast<UNovaModuleDescription>(Asset);
		CurrentAssembly->Compartments.Add(CompartmentAssembly);
	}
	else if (Asset->IsA(UNovaEquipmentDescription::StaticClass()))
	{
		FNovaCompartment CompartmentAssembly(EmptyCompartmentDescription);
		CompartmentAssembly.Equipments[0] = Cast<UNovaEquipmentDescription>(Asset);
		CurrentAssembly->Compartments.Add(CompartmentAssembly);
	}

	// Set up the scene
	Assembly->SetSpacecraft(CurrentAssembly);
	Assembly->UpdateAssembly();
	PlaceCamera();

	// Proceed with the screenshot
	CameraCapture->CaptureScene();
	UTexture2D* AssetRenderTexture = SaveTexture(ScreenshotPath);
	AssetRender.SetResourceObject(AssetRenderTexture);
	AssetRender.SetImageSize(GetDesiredSize());

	// Clean up
	Asset->MarkPackageDirty();
	Assembly->Destroy();

#endif
}

#if WITH_EDITOR

void ANovaCaptureActor::CreateAssembly()
{
	Assembly = Cast<ANovaSpacecraftAssembly>(GetWorld()->SpawnActor(ANovaSpacecraftAssembly::StaticClass()));
	NCHECK(Assembly);
	Assembly->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));
	Assembly->SetImmediateMode(true);
}

void ANovaCaptureActor::CreateCatalog()
{
	if (Catalog == nullptr)
	{
		Catalog = NewObject<UNovaAssetCatalog>(this, UNovaAssetCatalog::StaticClass(), TEXT("AssetCatalog"));
		NCHECK(Catalog);
	}

	Catalog->Initialize();
}

void ANovaCaptureActor::CreateRenderTarget()
{
	RenderTarget = NewObject<UTextureRenderTarget2D>();
	NCHECK(RenderTarget);

	FVector2D DesiredSize = GetDesiredSize();
	RenderTarget->InitAutoFormat(
		FGenericPlatformMath::RoundUpToPowerOfTwo(RenderUpscaleFactor * DesiredSize.X),
		FGenericPlatformMath::RoundUpToPowerOfTwo(RenderUpscaleFactor * DesiredSize.Y));
	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;

	CameraCapture->TextureTarget = RenderTarget;
}

void ANovaCaptureActor::PlaceCamera()
{
	FVector CurrentOrigin = FVector::ZeroVector;
	FVector CurrentExtent = FVector::ZeroVector;

	// Compute bounds
	FBox Bounds(ForceInit);
	Assembly->ForEachComponent<UPrimitiveComponent>(false, [&](const UPrimitiveComponent* Prim)
		{
			if (Prim->IsRegistered())
			{
				const UNovaStaticMeshComponent* StaticPrim = Cast<UNovaStaticMeshComponent>(Prim);
				const UNovaSkeletalMeshComponent* SkeletalPrim = Cast<UNovaSkeletalMeshComponent>(Prim);

				if (StaticPrim || SkeletalPrim)
				{
					Bounds += Prim->Bounds.GetBox();
				}
			}
		});
	Bounds.GetCenterAndExtents(CurrentOrigin, CurrentExtent);

	// Compute camera offset
	const float HalfFOVRadians = FMath::DegreesToRadians(CameraCapture->FOVAngle / 2.0f);
	const float DistanceFromSphere = FMath::Max(CurrentExtent.Size(), 2.0f * CurrentExtent.Z) / FMath::Tan(HalfFOVRadians);
	FVector ProjectedOffset = FVector(-3.0f * DistanceFromSphere, 0, 0);

	// Apply offset
	CameraArmComponent->SetRelativeLocation(FVector(CurrentOrigin.X, 0, 0));
	CameraCapture->SetRelativeLocation(ProjectedOffset);
}

UTexture2D* ANovaCaptureActor::SaveTexture(FString TextureName)
{
	FString NewTextureName = TextureName;
	UTexture2D* Texture = UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(RenderTarget, NewTextureName, TC_EditorIcon, TMGS_Sharpen5);
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->bPreserveBorder = true;
	Texture->NeverStream = true;
	Texture->SRGB = true;
	Texture->MaxTextureSize = (ResultUpscaleFactor * RenderTarget->SizeX) / RenderUpscaleFactor;
	Texture->PostEditChange();

	return Texture;
}

FVector2D ANovaCaptureActor::GetDesiredSize() const
{
	const FNovaButtonSize& ListButtonSize = FNovaStyleSet::GetButtonSize("LargeListButtonSize");

	return FVector2D(ListButtonSize.Width, ListButtonSize.Height);
}

#endif

#undef LOCTEXT_NAMESPACE
