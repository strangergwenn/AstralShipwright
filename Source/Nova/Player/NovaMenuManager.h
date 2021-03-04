// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUITypes.h"

#include "GameFramework/Actor.h"
#include "CoreMinimal.h"
#include "SlateBasics.h"
#include "NovaMenuManager.generated.h"

DECLARE_DELEGATE(FNovaAsyncAction);
DECLARE_DELEGATE_RetVal(bool, FNovaAsyncCondition);

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

/** Menu manager class */
UCLASS(ClassGroup = (Nova))
class UNovaMenuManager
	: public UObject
	, public TSharedFromThis<UNovaMenuManager>
{
	GENERATED_BODY()

public:
	UNovaMenuManager();

	/*----------------------------------------------------
	    Player interface
	----------------------------------------------------*/

	/** Initialize the menu */
	void Initialize(class UNovaGameInstance* NewGameInstance);

	/** Start playing on a new level */
	void BeginPlay(class ANovaPlayerController* PC);

	/** Tick the menu */
	void Tick(float DeltaSeconds);

	/** Get the game instance */
	class UNovaGameInstance* GetGameInstance() const
	{
		return GameInstance;
	}

	void FreezeLoadingScreen()
	{
		LoadingScreenFrozen = true;
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
	void ShowTooltip(SWidget* TargetWidget, FText Content);

	/** Stop displaying the tooltip */
	void HideTooltip(SWidget* TargetWidget);

	/*----------------------------------------------------
	    Menu tools
	----------------------------------------------------*/

	/** Get the menu manager instance */
	static UNovaMenuManager* Get();

	/** Get the local player controller owning the menus */
	class ANovaPlayerController* GetPC() const;

	/** Get the main menu */
	TSharedPtr<class SNovaMenu> GetMenu();

	/** Get the game overlay */
	TSharedPtr<class SNovaOverlay> GetOverlay() const;

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
	    Properties
	----------------------------------------------------*/

public:
	// Time it takes to fade in or out
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float FadeDuration;

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Singleton pointer
	static UNovaMenuManager* Singleton;

	// Game instance pointer
	UPROPERTY()
	class UNovaGameInstance* GameInstance;

	// Menu pointers
	TSharedPtr<class SNovaMainMenu> Menu;
	TSharedPtr<class SNovaOverlay>  Overlay;
	TSharedPtr<class SWidget>       DesiredFocusWidget;

	// Data
	bool                      LoadingScreenFrozen;
	static bool               UsingGamepad;
	FNovaAsyncCommand         CurrentCommand;
	TQueue<FNovaAsyncCommand> CommandStack;
	ENovaFadeState            CurrentMenuState;
	float                     CurrentFadingTime;

	// Dynamic material pool
	UPROPERTY()
	TArray<class UMaterialInterface*> OwnedMaterials;
};
