// Nova project - Gwennaël Arbona

#include "NovaMenuManager.h"

#include "Nova/Game/NovaWorldSettings.h"
#include "Nova/Player/NovaGameViewportClient.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Menu/NovaMainMenu.h"
#include "Nova/UI/Menu/NovaOverlay.h"
#include "Nova/UI/Widget/NovaButton.h"

#include "Nova/Nova.h"

#include "Framework/Application/SlateApplication.h"
#include "Engine/Console.h"
#include "Engine.h"

// Statics
UNovaMenuManager* UNovaMenuManager::Singleton    = nullptr;
bool              UNovaMenuManager::UsingGamepad = false;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaMenuManager::UNovaMenuManager() : Super(), CurrentMenuState(ENovaFadeState::FadingFromBlack)
{
	// Settings
	FadeDuration        = ENovaUIConstants::FadeDurationLong;
	ColorChangeDuration = ENovaUIConstants::FadeDurationShort;
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

	Singleton    = this;
	GameInstance = NewGameInstance;
	FSlateApplication::Get().SetNavigationConfig(MakeShared<FNovaNavigationConfig>());
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

	// Check for maximized state at boot
	TSharedPtr<SWindow> Window = FSlateApplication::Get().GetTopLevelWindows()[0];
	NCHECK(Window);
	FVector2D ViewportResolution = Window->GetViewportSize();
	FIntPoint DesktopResolution  = GEngine->GetGameUserSettings()->GetDesktopResolution();
	if (!Window->IsWindowMaximized() && ViewportResolution.X == DesktopResolution.X && ViewportResolution.Y != DesktopResolution.Y)
	{
		Window->Maximize();
	}

	// Initialize
	CurrentMenuState    = ENovaFadeState::FadingFromBlack;
	CurrentFadingTime   = FadeDuration;
	LoadingScreenFrozen = false;
	Menu->UpdateKeyBindings();

	// Initialize the desired color
	DesiredInterfaceColor = FLinearColor::White;
	CurrentInterfaceColor.SetPeriod(ColorChangeDuration);
	CurrentInterfaceColor.Set(DesiredInterfaceColor);

	// Open the menu if desired
	RunWaitAction(ENovaLoadingScreen::Black,
		FNovaAsyncAction::CreateLambda(
			[=]()
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
		FNovaAsyncCondition::CreateLambda(
			[=]()
			{
				NLOG("UNovaMenuManager::BeginPlay : done");
				return true;
			}));
}

void UNovaMenuManager::Tick(float DeltaTime)
{
	if (GetPC() && GetPC()->IsReady() && GetPC()->GetGameTimeSinceCreation() > 1.0f)
	{
		switch (CurrentMenuState)
		{
			// Fade to black, call the provided callback, and move on
			case ENovaFadeState::FadingToBlack:
			{
				FNovaAsyncCommand* Command = CommandStack.Peek();
				if (Command)
				{
					FadeDuration = Command->FadeDuration;
				}

				CurrentFadingTime += DeltaTime;
				CurrentFadingTime = FMath::Clamp(CurrentFadingTime, 0.0f, FadeDuration);

				if (CurrentFadingTime >= FadeDuration && CommandStack.Dequeue(CurrentCommand))
				{
					CurrentMenuState = ENovaFadeState::Black;
				}

				break;
			}

			// Process the queue of asynchronous actions + conditions
			case ENovaFadeState::Black:
			{
				// Processing action
				if (CurrentCommand.Action.IsBound())
				{
					CurrentCommand.Action.Execute();
					CurrentCommand.Action.Unbind();
					// NLOG("UNovaMenuManager::Tick : action finished, waiting condition");
				}

				// Waiting condition
				if (CurrentCommand.Condition.IsBound())
				{
					if (CurrentCommand.Condition.Execute())
					{
						CurrentCommand.Condition.Unbind();

						CompleteAsyncAction();

						// NLOG("UNovaMenuManager::Tick : action and condition finished");
					}
				}
				else
				{
					CompleteAsyncAction();

					// NLOG("UNovaMenuManager::Tick : action finished with no condition");
				}

				break;
			}

			// Fading back
			case ENovaFadeState::FadingFromBlack:
			{
				CurrentFadingTime -= DeltaTime;
				CurrentFadingTime = FMath::Clamp(CurrentFadingTime, 0.0f, FadeDuration);

				break;
			}
		}
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
	if (DesiredFocusWidget.IsValid() && CurrentFocusWidget != DesiredFocusWidget && AllowedFocusObjects.Contains(CurrentFocusWidget) &&
		AllowedFocusObjects.Contains(DesiredFocusWidget))
	{
		NLOG("UNovaMenuManager::Tick : moving focus from '%s' to '%s'",
			CurrentFocusWidget.IsValid() ? *CurrentFocusWidget->GetTypeAsString() : TEXT("null"), *DesiredFocusWidget->GetTypeAsString());

		FSlateApplication::Get().SetAllUserFocus(DesiredFocusWidget.ToSharedRef());
	}

	// Update UI color
	CurrentInterfaceColor.Set(DesiredInterfaceColor, DeltaTime);
}

/*----------------------------------------------------
    Menu management
----------------------------------------------------*/

void UNovaMenuManager::RunWaitAction(
	ENovaLoadingScreen LoadingScreen, FNovaAsyncAction Action, FNovaAsyncCondition Condition, bool ShortFade)
{
	NLOG("UNovaMenuManager::RunWaitAction");

	Cast<UNovaGameViewportClient>(GetWorld()->GetGameViewport())->SetLoadingScreen(LoadingScreen);

	CommandStack.Enqueue(FNovaAsyncCommand(Action, Condition, ShortFade));
	CurrentMenuState = ENovaFadeState::FadingToBlack;
}

void UNovaMenuManager::RunAction(ENovaLoadingScreen LoadingScreen, FNovaAsyncAction Action, bool ShortFade)
{
	RunWaitAction(LoadingScreen, Action,
		FNovaAsyncCondition::CreateLambda(
			[=]()
			{
				return false;
			}),
		ShortFade);
}

void UNovaMenuManager::CompleteAsyncAction()
{
	if (!CommandStack.Dequeue(CurrentCommand))
	{
		CurrentMenuState = ENovaFadeState::FadingFromBlack;
	}
}

void UNovaMenuManager::OpenMenu(FNovaAsyncAction Action, FNovaAsyncCondition Condition)
{
	NLOG("UNovaMenuManager::OpenMenu");

	RunWaitAction(ENovaLoadingScreen::Black,
		FNovaAsyncAction::CreateLambda(
			[=]()
			{
				if (Menu.IsValid())
				{
					Action.ExecuteIfBound();
					Menu->Show();
					SetFocusToMenu();
				}
			}),
		Condition);
}

void UNovaMenuManager::CloseMenu(FNovaAsyncAction Action, FNovaAsyncCondition Condition)
{
	NLOG("UNovaMenuManager::CloseMenu");

	RunWaitAction(ENovaLoadingScreen::Black,
		FNovaAsyncAction::CreateLambda(
			[=]()
			{
				if (Menu.IsValid())
				{
					Action.ExecuteIfBound();
					Menu->Hide();
					SetFocusToGame();
				}
			}),
		Condition);
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
		return FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadingTime / FadeDuration, ENovaUIConstants::EaseStrong);
	}
}

void UNovaMenuManager::ShowTooltip(SWidget* TargetWidget, const FText& Content)
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

FLinearColor UNovaMenuManager::GetInterfaceColor() const
{
	return CurrentInterfaceColor.Get();
}

void UNovaMenuManager::SetInterfaceColor(const FLinearColor& Color)
{
	DesiredInterfaceColor = Color;
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
	// NLOG("UNovaMenuManager::SetFocusToMenu");

	if (!IsUsingGamepad())
	{
		GetPC()->SetInputMode(FInputModeUIOnly());
	}

	DesiredFocusWidget = Menu;
}

void UNovaMenuManager::SetFocusToOverlay()
{
	// NLOG("UNovaMenuManager::SetFocusToOverlay");

	if (!IsUsingGamepad())
	{
		GetPC()->SetInputMode(FInputModeUIOnly());
	}

	DesiredFocusWidget = Overlay;
}

void UNovaMenuManager::SetFocusToGame()
{
	// NLOG("UNovaMenuManager::SetFocusToGame");

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

void UNovaMenuManager::MaximizeOrRestore()
{
	TSharedPtr<SWindow> Window = FSlateApplication::Get().GetTopLevelWindows()[0];
	NCHECK(Window);

	if (!Window->IsWindowMaximized())
	{
		Window->Maximize();
	}
	else
	{
		Window->Restore();
	}
}
