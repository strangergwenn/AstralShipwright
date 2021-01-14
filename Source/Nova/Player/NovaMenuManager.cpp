// Nova project - Gwennaël Arbona

#include "NovaMenuManager.h"
#include "NovaPlayerController.h"
#include "NovaGameViewportClient.h"

#include "Nova/Game/NovaWorldSettings.h"

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Menu/NovaMainMenu.h"
#include "Nova/UI/Menu/NovaOverlay.h"
#include "Nova/UI/Widget/NovaButton.h"

#include "Nova/Nova.h"

#include "Framework/Application/SlateApplication.h"
#include "Engine/Console.h"
#include "Engine.h"


// Statics
UNovaMenuManager* UNovaMenuManager::Singleton = nullptr;
bool UNovaMenuManager::UsingGamepad = false;


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UNovaMenuManager::UNovaMenuManager()
	: Super()
	, IsPlayerInitialized(false)
	, CurrentMenuState(ENovaFadeState::FadingFromBlack)
{
	// Settings
	FadingTime = ENovaUIConstants::FadeDurationLong;

	// Initialize 
	CurrentFadingTime = FadingTime;
}


/*----------------------------------------------------
	Inherited
----------------------------------------------------*/

class FNovaNavigationConfig : public FNavigationConfig
{
	EUINavigation GetNavigationDirectionFromKey(const FKeyEvent& InKeyEvent) const override
	{
		return EUINavigation::Invalid;
	}

	EUINavigation GetNavigationDirectionFromAnalog(const FAnalogInputEvent& InAnalogEvent) override
	{
		return EUINavigation::Invalid;
	}

	EUINavigationAction GetNavigationActionFromKey(const FKeyEvent& InKeyEvent) const override
	{
		return EUINavigationAction::Invalid;
	}
};

void UNovaMenuManager::Initialize(UNovaGameInstance* NewGameInstance)
{
	NLOG("UNovaMenuManager::Initialize");

	Singleton = this;
	GameInstance = NewGameInstance;
	FSlateApplication::Get().SetNavigationConfig(MakeShareable(new FNovaNavigationConfig()));
}

void UNovaMenuManager::BeginPlay(ANovaPlayerController* PC)
{
	NLOG("UNovaMenuManager::BeginPlay");

	// Create menus
	if (!Menu.IsValid() || !Overlay.IsValid())
	{
		SAssignNew(Menu, SNovaMainMenu).MenuManager(this);
		SAssignNew(Overlay, SNovaOverlay).MenuManager(this);

		UNovaGameViewportClient* GameViewportClient = Cast<UNovaGameViewportClient>(GetWorld()->GetGameViewport());
		GameViewportClient->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(Menu.ToSharedRef()), 50);
		GameViewportClient->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(Overlay.ToSharedRef()), 100);
	}

	// Initialize
	CurrentMenuState = ENovaFadeState::FadingFromBlack;
	CurrentFadingTime = FadingTime;
	LoadingScreenFrozen = false;
	Menu->UpdateKeyBindings();

	// Open the menu if desired
	RunWaitAction(ENovaLoadingScreen::Black,
		FNovaFadeAction::CreateLambda([=]()
		{
			NLOG("UNovaMenuManager::BeginPlay : setting up menu");

			if (Cast<ANovaWorldSettings>(GetWorld()->GetWorldSettings())->IsMenuMap())
			{
				Menu->Show();
				SetFocusToMenu();
			}
			else
			{
				Menu->Hide();
				SetFocusToGame();
			}

			NLOG("UNovaMenuManager::BeginPlay : waiting for a bit");
		}),
		FNovaFadeCondition::CreateLambda([=]()
		{
			// Wait a bit to ensure assets have some time to load
			if (GetPC() && GetPC()->GetGameTimeSinceCreation() > 1.0f)
			{
				NLOG("UNovaMenuManager::BeginPlay : done");
				return true;
			}
			else
			{
				return false;
			}
		})
	);
}

void UNovaMenuManager::Tick(float DeltaTime)
{
	if (IsPlayerInitialized)
	{
		switch (CurrentMenuState)
		{
			// Fade to black, call the provided callback, and move on
			case ENovaFadeState::FadingToBlack:
			{
				CurrentFadingTime += DeltaTime;
				CurrentFadingTime = FMath::Clamp(CurrentFadingTime, 0.0f, FadingTime);

				if (CurrentFadingTime >= FadingTime && ActionStack.Dequeue(CurrentAction))
				{
					CurrentMenuState = ENovaFadeState::Black;
				}

				break;
			}

			// Process the queue of asynchronous actions + conditions
			case ENovaFadeState::Black:
			{
				// Processing action
				if (CurrentAction.Key.IsBound())
				{
					CurrentAction.Key.Execute();
					CurrentAction.Key.Unbind();
					NLOG("UNovaMenuManager::Tick : action finished, waiting condition");
				}

				// Waiting condition
				if (CurrentAction.Value.IsBound())
				{
					if (CurrentAction.Value.Execute())
					{
						CurrentAction.Value.Unbind();

						CompleteAsyncAction();

						NLOG("UNovaMenuManager::Tick : action and condition finished");
					}
				}
				else
				{
					CompleteAsyncAction();

					NLOG("UNovaMenuManager::Tick : action finished with no condition");
				}

				break;
			}

			// Fading back
			case ENovaFadeState::FadingFromBlack:
			{
				CurrentFadingTime -= DeltaTime;
				CurrentFadingTime = FMath::Clamp(CurrentFadingTime, 0.0f, FadingTime);

				break;
			}
		}
	}

	// Waiting initial player player
	else if (GetPC() && GetPC()->IsReady())
	{
		NLOG("UNovaMenuManager::Tick : player initialized");
		IsPlayerInitialized = true;
	}

	// Build focus filter
	TArray<TSharedPtr<SWidget>> AllowedFocusObjects;
	AllowedFocusObjects.Add(Menu);
	AllowedFocusObjects.Add(Overlay);

	// Conditionally allow the game viewport, when not in console
	UNovaGameViewportClient* GameViewportClient = Cast<UNovaGameViewportClient>(GetWorld()->GetGameViewport());
	if (GameViewportClient && (GameViewportClient->ViewportConsole == nullptr || !GameViewportClient->ViewportConsole->ConsoleActive()))
	{
		AllowedFocusObjects.Add(FSlateApplication::Get().GetGameViewport());
	}

	// Update focus when it's one of our widgets to another of our widgets
	TSharedPtr<class SWidget> CurrentFocusWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
	if (DesiredFocusWidget.IsValid() && CurrentFocusWidget != DesiredFocusWidget
		&& AllowedFocusObjects.Contains(CurrentFocusWidget)
		&& AllowedFocusObjects.Contains(DesiredFocusWidget))
	{
		NLOG("UNovaMenuManager::Tick : moving focus from '%s' to '%s'",
			CurrentFocusWidget.IsValid() ? *CurrentFocusWidget->GetTypeAsString() : TEXT("null"),
			*DesiredFocusWidget->GetTypeAsString());

		FSlateApplication::Get().SetAllUserFocus(DesiredFocusWidget.ToSharedRef());
	}
}


/*----------------------------------------------------
	Menu management
----------------------------------------------------*/

void UNovaMenuManager::RunWaitAction(ENovaLoadingScreen LoadingScreen, FNovaFadeAction Action, FNovaFadeCondition Condition)
{
	NLOG("UNovaMenuManager::RunWaitAction");

	Cast<UNovaGameViewportClient>(GetWorld()->GetGameViewport())->SetLoadingScreen(LoadingScreen);

	ActionStack.Enqueue(TPair<FNovaFadeAction, FNovaFadeCondition>(Action, Condition));
	CurrentMenuState = ENovaFadeState::FadingToBlack;
}

void UNovaMenuManager::RunAction(ENovaLoadingScreen LoadingScreen, FNovaFadeAction Action)
{
	RunWaitAction(LoadingScreen, Action, FNovaFadeCondition::CreateLambda([=]()
		{
			return false;
		})
	);
}

void UNovaMenuManager::CompleteAsyncAction()
{
	if (!ActionStack.Dequeue(CurrentAction))
	{
		CurrentMenuState = ENovaFadeState::FadingFromBlack;
	}
}

void UNovaMenuManager::OpenMenu()
{
	NLOG("UNovaMenuManager::OpenMenu");

	RunWaitAction(ENovaLoadingScreen::Black,
		FNovaFadeAction::CreateLambda([=]()
		{
			if (Menu.IsValid())
			{
				GetPC()->OnOpenMenu();
				Menu->Show();
				SetFocusToMenu();
			}
		})
	);
}

void UNovaMenuManager::CloseMenu()
{
	NLOG("UNovaMenuManager::CloseMenu");
	
	RunWaitAction(ENovaLoadingScreen::Black,
		FNovaFadeAction::CreateLambda([=]()
		{
			if (Menu.IsValid())
			{
				GetPC()->OnCloseMenu();
				Menu->Hide();
				SetFocusToGame();
			}
		})
	);
}

bool UNovaMenuManager::IsMenuOpen() const
{
	return Menu && Menu->GetVisibility() == EVisibility::Visible;
}

bool UNovaMenuManager::IsMenuOpening() const
{
	return IsMenuOpen() || CurrentMenuState == ENovaFadeState::FadingToBlack;
}

float UNovaMenuManager::GetLoadingScreenAlpha() const
{
	if (LoadingScreenFrozen)
	{
		return 1.0f;
	}
	else
	{
		return FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadingTime / FadingTime, ENovaUIConstants::EaseStrong);
	}
}

void UNovaMenuManager::ShowTooltip(SWidget* TargetWidget, FText Content)
{
	if (Menu.IsValid())
	{
		Menu->ShowTooltip(TargetWidget, Content);
	}
}

void UNovaMenuManager::HideTooltip(SWidget* TargetWidget)
{
	if (Menu.IsValid())
	{
		Menu->HideTooltip(TargetWidget);
	}
}


/*----------------------------------------------------
	Menu tools
----------------------------------------------------*/

UNovaMenuManager* UNovaMenuManager::Get()
{
	return Singleton;
}

ANovaPlayerController* UNovaMenuManager::GetPC() const
{
	return Cast<ANovaPlayerController>(GameInstance->GetFirstLocalPlayerController());
}

TSharedPtr<SNovaMenu> UNovaMenuManager::GetMenu()
{
	return Menu;
}

TSharedPtr<SNovaOverlay> UNovaMenuManager::GetOverlay() const
{
	return Overlay;
}

void UNovaMenuManager::SetFocusToMenu()
{
	NLOG("UNovaMenuManager::SetFocusToMenu");

	if (!IsUsingGamepad())
	{
		GetPC()->SetInputMode(FInputModeUIOnly());
	}

	DesiredFocusWidget = Menu;
}

void UNovaMenuManager::SetFocusToOverlay()
{
	NLOG("UNovaMenuManager::SetFocusToOverlay");
	
	if (!IsUsingGamepad())
	{
		GetPC()->SetInputMode(FInputModeUIOnly());
	}

	DesiredFocusWidget = Overlay;
}

void UNovaMenuManager::SetFocusToGame()
{
	NLOG("UNovaMenuManager::SetFocusToGame");

	GetPC()->SetInputMode(FInputModeGameOnly());

	DesiredFocusWidget = FSlateApplication::Get().GetGameViewport();
}

void UNovaMenuManager::SetUsingGamepad(bool State)
{
	if (State != UsingGamepad)
	{
		if (State)
		{
			NLOG("UNovaMenuManager::SetUsingGamepad : using gamepad");

			FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);
		}
		else
		{
			NLOG("UNovaMenuManager::SetUsingGamepad : using mouse + keyboard");

			FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);
			FSlateApplication::Get().ReleaseAllPointerCapture();
			FSlateApplication::Get().GetPlatformApplication()->SetCapture(nullptr);
		}
	}

	UsingGamepad = State;
}

bool UNovaMenuManager::IsUsingGamepad() const
{
	return UsingGamepad;
}

bool UNovaMenuManager::IsOnConsole() const
{
	return false;
}

FKey UNovaMenuManager::GetFirstActionKey(FName ActionName) const
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	NCHECK(InputSettings);

	for (int32 i = 0; i < InputSettings->GetActionMappings().Num(); i++)
	{
		FInputActionKeyMapping Action = InputSettings->GetActionMappings()[i];
		if (Action.ActionName == ActionName && Action.Key.IsGamepadKey() == IsUsingGamepad())
		{
			return Action.Key;
		}
	}

	return FKey(NAME_None);
}
