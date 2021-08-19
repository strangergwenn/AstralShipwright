// Nova project - Gwennaël Arbona

#include "NovaCaptureActor.h"
#include "NovaStaticMeshComponent.h"
#include "NovaSkeletalMeshComponent.h"

#include "Nova/Game/NovaGameTypes.h"
#include "Nova/System/NovaAssetManager.h"
#include "Nova/UI/NovaUI.h"
#include "Nova/Nova.h"

#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/StaticMeshActor.h"

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
    Constructors
----------------------------------------------------*/

FNovaAssetPreviewSettings::FNovaAssetPreviewSettings()
	: Class(AStaticMeshActor::StaticClass())
	, RequireCustomPrimitives(false)
	, UsePowerfulLight(false)
	, Offset(FVector::ZeroVector)
	, Rotation(FRotator::ZeroRotator)
	, Scale(1.0f)
{}

ANovaCaptureActor::ANovaCaptureActor()
	: Super()
#if WITH_EDITORONLY_DATA
	, AssetToShoot(nullptr)
	, AssetManager(nullptr)
	, TargetAssetRender(nullptr)
	, TimeBeforeScreenshot(0)
#endif    // WITH_EDITORONLY_DATA
{
#if WITH_EDITORONLY_DATA

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create camera arm
	CameraArmComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CameraArm"));
	CameraArmComponent->SetupAttachment(RootComponent);

	// Create camera component
	CameraCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CameraCapture"));
	CameraCapture->SetupAttachment(CameraArmComponent, USpringArmComponent::SocketName);
	CameraCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	CameraCapture->CaptureSource       = ESceneCaptureSource::SCS_FinalToneCurveHDR;
	CameraCapture->bCaptureEveryFrame  = false;
	CameraCapture->bCaptureOnMovement  = false;
	CameraCapture->ShowFlags.EnableAdvancedFeatures();
	CameraCapture->ShowFlags.SetFog(false);
	CameraCapture->ShowFlags.SetGame(false);
	CameraCapture->ShowFlags.SetGrid(false);
	CameraCapture->FOVAngle                     = 70;
	CameraCapture->bAlwaysPersistRenderingState = true;
	CameraCapture->bUseRayTracingIfEnabled      = true;

	// Settings
	bIsEditorOnlyActor = true;
	SetActorTickEnabled(true);
	PrimaryActorTick.bCanEverTick          = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Defaults
	RenderUpscaleFactor = 4;
	ResultUpscaleFactor = 2;

#endif    // WITH_EDITORONLY_DATA
}

/*----------------------------------------------------
    Asset screenshot system
----------------------------------------------------*/

void ANovaCaptureActor::RenderAsset(UNovaAssetDescription* Asset, FSlateBrush& AssetRender)
{
#if WITH_EDITOR

	NCHECK(Asset);

	if (AssetToShoot == nullptr)
	{
		NLOG("ANovaCaptureActor::RenderAsset for '%s'", *Asset->GetName());

		// Copy state
		AssetToShoot         = Asset;
		TargetAssetRender    = &AssetRender;
		TimeBeforeScreenshot = 0.1f;

		// Get required objects
		CreateAssetManager();
		CreateRenderTarget();

		// Create the actor and scene
		const FNovaAssetPreviewSettings& Settings = AssetToShoot->GetPreviewSettings();
		CreateActor(Settings.Class);
		AssetToShoot->ConfigurePreviewActor(PreviewActor);
		ConfigureScene(Settings);
		IStreamingManager::Get().GetRenderAssetStreamingManager().BlockTillAllRequestsFinished();

		// Force all actor textures at maximum resolution
		PreviewActor->ForEachComponent<UPrimitiveComponent>(false,
			[=](UPrimitiveComponent* MeshComponent)
			{
				NLOG("ANovaCaptureActor::RenderAsset : actor has component '%s'",
					MeshComponent ? *MeshComponent->GetName() : TEXT("nullptr"));

				if (IsValid(MeshComponent) && IsValid(MeshComponent->GetMaterial(0)))
				{
					UMaterialInterface* Material = MeshComponent->GetMaterial(0);
					NLOG("ANovaCaptureActor::RenderAsset : component has material '%s'", *Material->GetName());

					TArray<UTexture*> Textures;
					Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, false, ERHIFeatureLevel::Num, true);

					for (UTexture* Texture : Textures)
					{
						Texture->SetForceMipLevelsToBeResident(30.0f);
						if (Texture->IsA<UTexture2D>())
						{
							Texture->StreamIn(Cast<UTexture2D>(Texture)->GetNumMips(), true);
						}
						Texture->WaitForStreaming();
					}
				}
			});
		IStreamingManager::Get().GetRenderAssetStreamingManager().BlockTillAllRequestsFinished();
	}

#endif    // WITH_EDITOR
}

#if WITH_EDITOR

void ANovaCaptureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeBeforeScreenshot -= DeltaTime;

	if (AssetToShoot && TimeBeforeScreenshot <= 0.0f)
	{
		NLOG("ANovaCaptureActor::Tick : proceeding with screenshot for '%s'", *GetName());

		// Proceed with the screenshot
		CameraCapture->CaptureScene();

		// Delete previous content
		if (TargetAssetRender->GetResourceObject())
		{
			TArray<UObject*> AssetsToDelete;
			AssetsToDelete.Add(TargetAssetRender->GetResourceObject());
			ObjectTools::ForceDeleteObjects(AssetsToDelete, false);
		}

		// Build path
		int32   SeparatorIndex;
		FString ScreenshotPath = AssetToShoot->GetOuter()->GetFName().ToString();
		if (ScreenshotPath.FindLastChar('/', SeparatorIndex))
		{
			ScreenshotPath.InsertAt(SeparatorIndex + 1, TEXT("T_"));
		}

		// Save the asset
		UTexture2D* AssetRenderTexture = SaveTexture(ScreenshotPath);
		TargetAssetRender->SetResourceObject(AssetRenderTexture);
		TargetAssetRender->SetImageSize(GetDesiredSize());

		// Clean up
		AssetToShoot->MarkPackageDirty();
		PreviewActor->Destroy();
		AssetToShoot = nullptr;
	}
}

void ANovaCaptureActor::CreateActor(TSubclassOf<AActor> ActorClass)
{
	PreviewActor = Cast<AActor>(GetWorld()->SpawnActor(ActorClass));
	NCHECK(PreviewActor);

	PreviewActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	PreviewActor->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));
}

void ANovaCaptureActor::CreateAssetManager()
{
	if (AssetManager == nullptr)
	{
		AssetManager = NewObject<UNovaAssetManager>(this, UNovaAssetManager::StaticClass(), TEXT("AssetManager"));
		NCHECK(AssetManager);
	}

	AssetManager->Initialize();
}

void ANovaCaptureActor::CreateRenderTarget()
{
	RenderTarget = NewObject<UTextureRenderTarget2D>();
	NCHECK(RenderTarget);

	FVector2D DesiredSize = GetDesiredSize();
	RenderTarget->InitAutoFormat(FGenericPlatformMath::RoundUpToPowerOfTwo(RenderUpscaleFactor * DesiredSize.X),
		FGenericPlatformMath::RoundUpToPowerOfTwo(RenderUpscaleFactor * DesiredSize.Y));
	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;

	CameraCapture->TextureTarget = RenderTarget;
}

void ANovaCaptureActor::ConfigureScene(const FNovaAssetPreviewSettings& Settings)
{
	FVector CurrentOrigin = FVector::ZeroVector;
	FVector CurrentExtent = FVector::ZeroVector;

	NCHECK(PreviewActor);

	// Compute bounds
	FBox Bounds(ForceInit);
	PreviewActor->ForEachComponent<UPrimitiveComponent>(false,
		[&](const UPrimitiveComponent* Prim)
		{
			if (Prim->IsRegistered())
			{
				bool IsValidPrimitive = Prim->IsA<UStaticMeshComponent>() || Prim->IsA<USkeletalMeshComponent>();
				if (Settings.RequireCustomPrimitives)
				{
					IsValidPrimitive = Prim->IsA<UNovaStaticMeshComponent>() || Prim->IsA<UNovaSkeletalMeshComponent>();
				}

				if (IsValidPrimitive)
				{
					Bounds += Prim->Bounds.GetBox();
				}
			}
		});
	Bounds.GetCenterAndExtents(CurrentOrigin, CurrentExtent);

	// Compute camera offset
	const float HalfFOVRadians     = FMath::DegreesToRadians(CameraCapture->FOVAngle / 2.0f);
	const float DistanceFromSphere = FMath::Max(CurrentExtent.Size(), 2.0f * CurrentExtent.Z) / FMath::Tan(HalfFOVRadians);
	FVector     ProjectedOffset    = FVector(-3.0f * DistanceFromSphere, 0, 0);

	// Apply offset
	CameraArmComponent->SetRelativeLocation(FVector(CurrentOrigin.X, 0, 0));
	CameraCapture->SetRelativeLocation(ProjectedOffset);
	PreviewActor->SetActorLocation(GetActorLocation() + Settings.Offset);
	PreviewActor->SetActorRelativeRotation(Settings.Rotation);
	PreviewActor->SetActorScale3D(Settings.Scale * FVector(1.0f, 1.0f, 1.0f));

	// Set lights
	PreviewActor->ForEachComponent<USpotLightComponent>(false,
		[&](USpotLightComponent* SpotLight)
		{
			SpotLight->SetLightBrightness(Settings.UsePowerfulLight ? 10000000 : 500000);
		});
}

UTexture2D* ANovaCaptureActor::SaveTexture(FString TextureName)
{
	FString     NewTextureName = TextureName;
	UTexture2D* Texture =
		UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(RenderTarget, NewTextureName, TC_EditorIcon, TMGS_Sharpen5);
	Texture->LODGroup        = TEXTUREGROUP_UI;
	Texture->bPreserveBorder = true;
	Texture->NeverStream     = true;
	Texture->SRGB            = true;
	Texture->MaxTextureSize  = (ResultUpscaleFactor * RenderTarget->SizeX) / RenderUpscaleFactor;
	Texture->PostEditChange();

	return Texture;
}

FVector2D ANovaCaptureActor::GetDesiredSize() const
{
	const FNovaButtonSize& ListButtonSize = FNovaStyleSet::GetButtonSize("LargeListButtonSize");

	return FVector2D(ListButtonSize.Width, ListButtonSize.Height);
}

#endif    // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
