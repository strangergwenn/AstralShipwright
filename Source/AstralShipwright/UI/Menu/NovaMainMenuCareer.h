// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronTabView.h"
#include "Neutron/UI/Widgets/NeutronListView.h"

#include "Online.h"

class SNovaMainMenuCareer
	: public SNeutronTabPanel
	, public INeutronGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuCareer)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	SNovaMainMenuCareer();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Show() override;

	virtual void Hide() override;

	virtual void UpdateGameObjects() override;

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

	FText GetCrewChanges() const;
	FText GetCrewDetails() const;
	bool  CanConfirmCrew() const;

	FText GetUnlockInfo() const;

	bool IsComponentUnlockable(const class UNovaTradableAssetDescription* Asset) const;
	bool IsComponentUnlocked(const class UNovaTradableAssetDescription* Asset) const;
	bool IsComponentUnlockAllowed(const class UNovaTradableAssetDescription* Asset) const
	{
		return IsComponentUnlockable(Asset) && !IsComponentUnlocked(Asset);
	}

	FText              GetComponentHelpText(const class UNovaTradableAssetDescription* Asset) const;
	const FSlateBrush* GetComponentIcon(const class UNovaTradableAssetDescription* Asset) const;

protected:

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

	void OnCrewChanged(float Value);

	void OnCrewConfirmed();

	void OnComponentUnlocked(const class UNovaTradableAssetDescription* Asset);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Game objects
	TWeakObjectPtr<class UNeutronMenuManager> MenuManager;
	class ANovaPlayerController*              PC;
	class ANovaGameState*                     GameState;
	class UNovaSpacecraftCrewSystem*          CrewSystem;

	// Widgets
	TArray<TSharedPtr<class SVerticalBox>>   UnlockBoxes;
	TArray<TSharedPtr<class SNeutronButton>> UnlockButtons;
	TSharedPtr<class SNeutronModalPanel>     ModalPanel;
	TSharedPtr<class SNeutronSlider>         CrewSlider;

	// Local data
	int32 CurrentCrewValue;
};
