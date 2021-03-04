// Nova project - Gwennaël Arbona

#include "NovaGameViewportClient.h"

#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Player/NovaMenuManager.h"

#include "Nova/Tools/NovaCanvas.h"
#include "Nova/UI/Component/NovaLoadingScreen.h"
#include "Nova/Nova.h"

#include "MoviePlayer.h"
#include "Engine/Canvas.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaGameViewportClient::UNovaGameViewportClient() : Super()
{}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNovaGameViewportClient::SetViewport(FViewport* InViewport)
{
	NLOG("UNovaGameViewportClient::SetViewport");

	Super::SetViewport(InViewport);

	Initialize();

	SetLoadingScreen(ENovaLoadingScreen::None);
}

void UNovaGameViewportClient::BeginDestroy()
{
	NLOG("UNovaGameViewportClient::BeginDestroy");

	AnimatedMaterialInstance = nullptr;
	LoadingScreenWidget.Reset();

	Super::BeginDestroy();
}

void UNovaGameViewportClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Get the current loading screen alpha from the menu manager if valid
	float             Alpha       = 1;
	UNovaMenuManager* MenuManager = Cast<UNovaGameInstance>(GetGameInstance())->GetMenuManager();
	if (MenuManager)
	{
		Alpha = MenuManager->GetLoadingScreenAlpha();
	}

	// Apply alpha
	if (LoadingScreenWidget.IsValid())
	{
		LoadingScreenWidget->SetFadeAlpha(Alpha);
	}
}

void UNovaGameViewportClient::SetLoadingScreen(ENovaLoadingScreen LoadingScreen)
{
	Initialize();

	// Disallow changes during animation
	if (CurrentLoadingScreen != ENovaLoadingScreen::None && LoadingScreen != CurrentLoadingScreen && LoadingScreenWidget.IsValid() &&
		LoadingScreenWidget->GetFadeAlpha() != 0)
	{
		NERR("UNovaGameViewportClient::SetLoadingScreen : can't switch loading screen here");
		return;
	}

	// Default to previous screen if none was passed
	if (LoadingScreen == ENovaLoadingScreen::None)
	{
		if (CurrentLoadingScreen == ENovaLoadingScreen::None)
		{
			LoadingScreen = ENovaLoadingScreen::Black;
		}
		else
		{
			LoadingScreen = CurrentLoadingScreen;
		}
	}
	else
	{
		CurrentLoadingScreen = LoadingScreen;
	}

	NLOG("UNovaGameViewportClient::SetLoadingScreen : setting %d", static_cast<int32>(LoadingScreen));

	// Setup the loading screen widget with the desired resource
	UTexture2D* LoadingScreenTexture =
		CurrentLoadingScreen == ENovaLoadingScreen::Launch ? LoadingScreenSetup->LaunchScreen : LoadingScreenSetup->BlackScreen;

	// Set the texture
	NCHECK(AnimatedMaterialInstance);
	NCHECK(LoadingScreenTexture);
	AnimatedMaterialInstance->SetTextureParameterValue("LoadingScreenTexture", LoadingScreenTexture);
}

void UNovaGameViewportClient::ShowLoadingScreen()
{
	NLOG("UNovaGameViewportClient::ShowLoadingScreen");

	// Start the loading screen
	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	LoadingScreen.WidgetLoadingScreen               = LoadingScreenWidget;
	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNovaGameViewportClient::Initialize()
{
	NLOG("UNovaGameViewportClient::Initialize");

	if (LoadingScreenSetup == nullptr || AnimatedMaterialInstance == nullptr || !LoadingScreenWidget.IsValid())
	{
		// Find the loading screen setup
		TArray<const UNovaLoadingScreenSetup*> LoadingScreenSetupList =
			Cast<UNovaGameInstance>(GetGameInstance())->GetCatalog()->GetAssets<UNovaLoadingScreenSetup>();
		if (LoadingScreenSetupList.Num())
		{
			LoadingScreenSetup = LoadingScreenSetupList.Last();
			NCHECK(LoadingScreenSetup);

			// Create the material
			if (AnimatedMaterialInstance == nullptr && LoadingScreenSetup && LoadingScreenSetup->AnimatedMaterial)
			{
				AnimatedMaterialInstance = UMaterialInstanceDynamic::Create(LoadingScreenSetup->AnimatedMaterial, this);
				NCHECK(AnimatedMaterialInstance);
			}

			// Build the loading widget using the material instance
			LoadingScreenWidget = SNew(SNovaLoadingScreen).Settings(LoadingScreenSetup).Material(AnimatedMaterialInstance);
			AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(LoadingScreenWidget), 1000);
		}
	}
}
