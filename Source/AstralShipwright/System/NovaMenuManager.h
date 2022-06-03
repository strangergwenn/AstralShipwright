// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"

#include "GameFramework/Actor.h"
#include "Actor/NovaActorTools.h"
#include "CoreMinimal.h"
#include "SlateBasics.h"
#include "Tickable.h"
#include "NovaMenuManager.generated.h"

/*----------------------------------------------------
    Supporting types
----------------------------------------------------*/

/** Menu states */
enum class ENovaFadeState : uint8
{
	FadingFromBlack,
	Black,
	FadingToBlack
};

/** Async command data */
struct FNovaAsyncCommand
{
	FNovaAsyncCommand() : Action(), Condition(), FadeDuration(0)
	{}

	FNovaAsyncCommand(FNovaAsyncAction A, FNovaAsyncCondition C, bool ShortFade)
		: Action(A), Condition(C), FadeDuration(ShortFade ? ENovaUIConstants::FadeDurationShort : ENovaUIConstants::FadeDurationLong)
	{}

	FNovaAsyncAction    Action;
	FNovaAsyncCondition Condition;
	float               FadeDuration;
};

/*----------------------------------------------------
    Menu manager class
----------------------------------------------------*/

/** Menu manager class */
UCLASS(ClassGroup = (Nova))
class UNovaMenuManager
	: public UObject
	, public TSharedFromThis<UNovaMenuManager>
	, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UNovaMenuManager();

	/*----------------------------------------------------
	    Player interface
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNovaMenuManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNovaGameInstance* NewGameInstance);

	/** Start playing on a new level */
	template <typename MenuClass, typename OverlayClass>
	void BeginPlay(class ANovaPlayerController* PC)
	{
		NLOG("UNovaMenuManager::BeginPlay");

		bool AddMenusToScreen = false;
		if (!Menu.IsValid() || !Overlay.IsValid())
		{
			SAssignNew(Menu, MenuClass).MenuManager(this);
			SAssignNew(Overlay, OverlayClass).MenuManager(this);
			AddMenusToScreen = true;
		}

		BeginPlayInternal(PC, AddMenusToScreen);
	}

	/*----------------------------------------------------
	    Tick
	----------------------------------------------------*/

	virtual void              Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UNovaMenuManager, STATGROUP_Tickables);
	}
	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}

	/*----------------------------------------------------
	    Menu management
	----------------------------------------------------*/

	/** Fade to black with a loading screen, call the action, wait for the condition to return true, then fade back */
	void RunWaitAction(ENovaLoadingScreen LoadingScreen, FNovaAsyncAction Action, FNovaAsyncCondition Condition = FNovaAsyncCondition(),
		bool ShortFade = false);

	/** Fade to black with a loading screen, call the action, stay black */
	void RunAction(ENovaLoadingScreen LoadingScreen, FNovaAsyncAction Action, bool ShortFade = false);

	/** Manually reveal the game image once the action is finished */
	void CompleteAsyncAction();

	/** Check if the menu system is idle */
	bool IsIdle() const
	{
		return CurrentMenuState == ENovaFadeState::FadingFromBlack;
	}

	/** Request opening of the main menu */
	void OpenMenu(FNovaAsyncAction Action = FNovaAsyncAction(), FNovaAsyncCondition Condition = FNovaAsyncCondition());

	/** Request closing of the main menu */
	void CloseMenu(FNovaAsyncAction Action = FNovaAsyncAction(), FNovaAsyncCondition Condition = FNovaAsyncCondition());

	/** Check if the main menu is open */
	bool IsMenuOpen() const;

	/** Check if the main menu is open or currently opening */
	bool IsMenuOpening() const;

	/** Get the current loading screen alpha */
	float GetLoadingScreenAlpha() const;

	/** Start displaying the tooltip */
	void ShowTooltip(SWidget* TargetWidget, const FText& Content);

	/** Stop displaying the tooltip */
	void HideTooltip(SWidget* TargetWidget);

	/** Change the current UI colors */
	void SetInterfaceColor(const FLinearColor& Color, const FLinearColor& HighlightColor);

	/** Return the current UI color */
	FLinearColor GetInterfaceColor() const;

	/** Return the current highlight UI color */
	FLinearColor GetHighlightColor() const;

	/*----------------------------------------------------
	    Menu tools
	----------------------------------------------------*/

	/** Register a menu **/
	void RegisterGameMenu(TSharedPtr<class INovaGameMenu> GameMenu)
	{
		GameMenus.Add(GameMenu.Get());
	}

	/** Get the local player controller owning the menus */
	template <typename T = ANovaPlayerController>
	T* GetPC() const
	{
		return Cast<T>(PlayerController);
	}

	/** Get the main menu */
	template <typename T = SNovaMenu>
	TSharedPtr<T> GetMenu()
	{
		return StaticCastSharedPtr<T>(Menu);
	}

	/** Get the game overlay */
	template <typename T = SWidget>
	TSharedPtr<T> GetOverlay() const
	{
		return StaticCastSharedPtr<T>(Overlay);
	}

	/** Set the focus to the main menu */
	void SetFocusToMenu();

	/** Set the focus to the game overlay */
	void SetFocusToOverlay();

	/** Set the focus to the game */
	void SetFocusToGame();

	/** Set if we are using a gamepad */
	void SetUsingGamepad(bool State);

	/** Check if we are using a gamepad */
	UFUNCTION(Category = Nova, BlueprintCallable)
	bool IsUsingGamepad() const;

	/** Are we running on console ? */
	UFUNCTION(Category = Nova, BlueprintCallable)
	bool IsOnConsole() const;

	/** Get the first key mapped to a specific action */
	FKey GetFirstActionKey(FName ActionName) const;

	/** Maximize the current window or restore it */
	void MaximizeOrRestore();

	/*----------------------------------------------------
	    Internal
	----------------------------------------------------*/

protected:

	/** Signal the player is ready */
	void BeginPlayInternal(class ANovaPlayerController* PC, bool AddMenusToScreen);

	/** World was cleaned up, warn menus */
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:

	// Time it takes to fade in or out in seconds
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float FadeDuration;

	// Time it takes to change the UI color in seconds
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float ColorChangeDuration;

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Singleton pointer
	static UNovaMenuManager* Singleton;

	// Player owner
	UPROPERTY()
	class ANovaPlayerController* PlayerController;

	// Menu pointers
	TSharedPtr<class SNovaMenu> Menu;
	TSharedPtr<class SWidget>   Overlay;
	TSharedPtr<class SWidget>   DesiredFocusWidget;

	// Current menu state
	bool                         UsingGamepad;
	FNovaAsyncCommand            CurrentCommand;
	TQueue<FNovaAsyncCommand>    CommandStack;
	ENovaFadeState               CurrentMenuState;
	TArray<class INovaGameMenu*> GameMenus;

	// Current color state
	float                           CurrentFadingTime;
	FLinearColor                    DesiredInterfaceColor;
	FLinearColor                    DesiredHighlightColor;
	TNovaTimedAverage<FLinearColor> CurrentInterfaceColor;
	TNovaTimedAverage<FLinearColor> CurrentHighlightColor;

	// Dynamic material pool
	UPROPERTY()
	TArray<class UMaterialInterface*> OwnedMaterials;
};
