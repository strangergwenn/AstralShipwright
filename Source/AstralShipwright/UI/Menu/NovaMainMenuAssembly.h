// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"
#include "UI/Widget/NovaTabView.h"
#include "UI/Widget/NovaModalListView.h"
#include "Game/NovaGameTypes.h"

#include "Online.h"

enum class ENovaAssemblyDisplayFilter : uint8;
enum class ENovaHullType : uint8;

enum class ENovaMainMenuAssemblyState
{
	Assembly,
	Compartment,
	Customization
};

class SNovaMainMenuAssembly
	: public SNovaTabPanel
	, public INovaGameMenu
{
	typedef SNovaModalListView<const class UNovaCompartmentDescription*> SNovaCompartmentList;
	typedef SNovaModalListView<const class UNovaModuleDescription*>      SNovaModuleList;
	typedef SNovaModalListView<const class UNovaEquipmentDescription*>   SNovaEquipmentList;
	typedef SNovaModalListView<ENovaAssemblyDisplayFilter>               SNovaDisplayFilterList;
	typedef SNovaModalListView<const class UNovaHullDescription*>        SNovaHullTypeList;

	typedef SNovaListView<const class UNovaPaintDescription*>           SNovaPaintList;
	typedef SNovaListView<const class UNovaStructuralPaintDescription*> SNovaStructuralPaintList;

	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuAssembly)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	SNovaMainMenuAssembly();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void UpdateGameObjects() override;

	virtual void Next() override;

	virtual void Previous() override;

	virtual void OnClicked(const FVector2D& Position) override;

	virtual void OnDoubleClicked(const FVector2D& Position) override;

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;

	virtual TSharedPtr<SNovaButton> GetDefaultFocusButton() const override;

	virtual bool IsButtonActionAllowed(TSharedPtr<SNovaButton> Button) const override;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Get the index of the next compartment to build */
	int32 GetNewBuildIndex(bool Forward = true) const;

	/** Select a compartment index */
	void SetSelectedCompartment(int32 Index);

	/** Select a module or equipment */
	void SetSelectedModuleOrEquipment(int32 Index);

	/** Set which sub-menu is active */
	void SetPanelState(ENovaMainMenuAssemblyState State);

	bool IsCompartmentPanelVisible() const
	{
		return CurrentPanelState == ENovaMainMenuAssemblyState::Compartment;
	}

	/*----------------------------------------------------
	    Module / equipment index helpers
	----------------------------------------------------*/

	int32 GetMaxCommonIndex() const
	{
		return ENovaConstants::MaxModuleCount + ENovaConstants::MaxEquipmentCount - 1;
	}

	bool IsModuleIndex(int32 Index) const
	{
		return Index < ENovaConstants::MaxModuleCount;
	}

	int32 GetModuleIndex(int32 Index) const
	{
		return Index;
	}

	int32 GetEquipmentIndex(int32 Index) const
	{
		return Index - ENovaConstants::MaxModuleCount;
	}

	int32 GetCommonIndexFromModule(int32 Index) const
	{
		return Index;
	}

	int32 GetCommonIndexFromEquipment(int32 Index) const
	{
		return Index + ENovaConstants::MaxModuleCount;
	}

	bool IsModuleSelected() const
	{
		return IsModuleIndex(SelectedModuleOrEquipmentIndex);
	}

	int32 GetSelectedModuleIndex() const
	{
		return GetModuleIndex(SelectedModuleOrEquipmentIndex);
	}

	int32 GetSelectedEquipmentIndex() const
	{
		return GetEquipmentIndex(SelectedModuleOrEquipmentIndex);
	}

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	// Compartment template list
	TSharedRef<SWidget> GenerateCompartmentItem(const class UNovaCompartmentDescription* Description) const;
	FText               GenerateCompartmentTooltip(const class UNovaCompartmentDescription* Description) const;

	// Compartment module list
	bool                IsModuleListEnabled() const;
	EVisibility         GetModuleListVisibility() const;
	FText               GetModuleListHelpText() const;
	TSharedRef<SWidget> GenerateModuleItem(const class UNovaModuleDescription* Module) const;
	FText               GetModuleListTitle(const class UNovaModuleDescription* Module) const;
	FText               GenerateModuleTooltip(const class UNovaModuleDescription* Module) const;

	// Compartment equipment list
	bool                IsEquipmentListEnabled() const;
	EVisibility         GetEquipmentListVisibility() const;
	FText               GetEquipmentListHelpText() const;
	TSharedRef<SWidget> GenerateEquipmentItem(const class UNovaEquipmentDescription* Equipment) const;
	FText               GetEquipmentListTitle(const class UNovaEquipmentDescription* Equipment) const;
	FText               GenerateEquipmentTooltip(const class UNovaEquipmentDescription* Equipment) const;

	// Compartment hull types list
	TSharedRef<SWidget> GenerateHullTypeItem(const class UNovaHullDescription* Hull) const;
	FText               GetHullTypeListTitle(const class UNovaHullDescription* Hull) const;
	FText               GenerateHullTypeTooltip(const class UNovaHullDescription* Hull) const;

	// Helpers
	FText GetAssetName(const class UNovaTradableAssetDescription* Asset) const;

	// Panels
	FLinearColor GetMainColor() const;
	FLinearColor GetCompartmentColor() const;
	FLinearColor GetCustomizationColor() const;

	// Assembly callbacks
	FText GetSelectedFilterText() const;
	bool  IsSelectCompartmentEnabled(int32 Index) const;
	bool  IsAddCompartmentEnabled() const;
	bool  IsEditCompartmentEnabled() const;
	FText GetCompartmentText();
	bool  IsBackToAssemblyEnabled() const;

	// Module callbacks
	FText GetModuleHelpText(int32 ModuleIndex) const;
	bool  IsModuleEnabled(int32 ModuleIndex) const
	{
		return IsModuleEnabled(ModuleIndex, nullptr);
	}
	bool IsModuleEnabled(int32 ModuleIndex, FText* Help) const;

	// Equipment callbacks
	FText GetEquipmentHelpText(int32 EquipmentIndex) const;
	bool  IsEquipmentEnabled(int32 EquipmentIndex) const
	{
		return IsEquipmentEnabled(EquipmentIndex, nullptr);
	}
	bool IsEquipmentEnabled(int32 EquipmentIndex, FText* Help) const;

	// Module + equipment
	FText GetModuleOrEquipmentText();

	// Key bindings
	FKey GetPreviousItemKey() const;
	FKey GetNextItemKey() const;

	// Paint lists
	TSharedRef<SWidget> GenerateStructuralPaintItem(const class UNovaStructuralPaintDescription* Paint) const;
	FText               GenerateStructuralPaintTooltip(const class UNovaStructuralPaintDescription* Paint) const;
	TSharedRef<SWidget> GenerateHullPaintItem(const class UNovaStructuralPaintDescription* Paint) const;
	TSharedRef<SWidget> GenerateDetailPaintItem(const class UNovaPaintDescription* Paint) const;
	FText               GenerateDetailPaintTooltip(const class UNovaPaintDescription* Paint) const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	// Compartment edition
	void OnEditCompartment();
	void OnRemoveCompartment();
	void OnRemoveCompartmentConfirmed();

	// Compartment selection
	void OnAddCompartment(const class UNovaCompartmentDescription* Compartment, int32 Index, bool Forward = true);
	void OnCompartmentSelected(int32 Index);

	// Display filters
	void OnEnterPhotoMode(FName ActionName);
	void OnSelectedFilterChanged(float Value);

	// Modules & equipment
	void OnSelectedModuleChanged(const class UNovaModuleDescription* Module, int32 Index);
	void OnSelectedEquipmentChanged(const class UNovaEquipmentDescription* Equipment, int32 Index);
	void OnSelectedHullTypeChanged(const class UNovaHullDescription* Hull, int32 Index);

	// Customization
	void OnOpenCustomization();
	void OnStructuralPaintSelected(const class UNovaStructuralPaintDescription* Paint, int32 Index);
	void OnHullPaintSelected(const class UNovaStructuralPaintDescription* Paint, int32 Index);
	void OnDetailPaintSelected(const class UNovaPaintDescription* Paint, int32 Index);
	void OnDirtyIntensityChanged(float Value);
	void OnSpacecraftNameChanged(const FText& InText);
	void OnConfirmCustomization();

	// Save & exit
	void OnReviewSpacecraft();
	void OnBackToAssembly();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Game objects
	TWeakObjectPtr<UNovaMenuManager> MenuManager;
	class ANovaPlayerController*     PC;
	class ANovaSpacecraftPawn*       SpacecraftPawn;
	class ANovaGameState*            GameState;

	// Widgets
	TSharedPtr<SHorizontalBox>                CompartmentBox;
	TSharedPtr<SHorizontalBox>                ModuleBox;
	TSharedPtr<SHorizontalBox>                EquipmentBox;
	TSharedPtr<SEditableText>                 SpacecraftNameText;
	TSharedPtr<class SNovaSlider>             DisplayFilter;
	TSharedPtr<class SNovaButton>             SaveButton;
	TSharedPtr<class SNovaButton>             BackButton;
	TSharedPtr<class SNovaButton>             EditButton;
	TSharedPtr<class SNovaButton>             PhotoModeButton;
	TSharedPtr<class SNovaModalPanel>         GenericModalPanel;
	TSharedPtr<class SNovaAssemblyModalPanel> AssemblyModalPanel;
	TSharedPtr<SVerticalBox>                  MenuBox;
	TSharedPtr<class SNovaSlider>             DirtyIntensity;

	// Panel fading system
	ENovaMainMenuAssemblyState DesiredPanelState;
	ENovaMainMenuAssemblyState CurrentPanelState;
	float                      FadeDuration;
	float                      CurrentFadeTime;

	// Assembly data
	int32 SelectedCompartmentIndex;
	int32 EditedCompartmentIndex;
	int32 SelectedModuleOrEquipmentIndex;
	float TimeSinceLeftIndexChange;

	// Carousel animations
	FNovaCarouselAnimation<ENovaConstants::MaxCompartmentCount>                                CompartmentAnimation;
	FNovaCarouselAnimation<ENovaConstants::MaxModuleCount + ENovaConstants::MaxEquipmentCount> ModuleEquipmentAnimation;

	// Compartment list
	TSharedPtr<SNovaCompartmentList> CompartmentList;

	// Hull type list
	TArray<const class UNovaHullDescription*> HullTypeList;
	TSharedPtr<SNovaHullTypeList>             HullTypeListView;

	// Compartment module list
	TArray<const class UNovaModuleDescription*> ModuleList;
	TSharedPtr<SNovaModuleList>                 ModuleListView;

	// Compartment equipment list
	TArray<const class UNovaEquipmentDescription*> EquipmentList;
	TSharedPtr<SNovaEquipmentList>                 EquipmentListView;

	// Paint lists
	TArray<const class UNovaPaintDescription*>           GenericPaintList;
	TArray<const class UNovaStructuralPaintDescription*> StructuralPaintList;
	TSharedPtr<SNovaStructuralPaintList>                 StructuralPaintListView;
	TSharedPtr<SNovaStructuralPaintList>                 HullPaintListView;
	TSharedPtr<SNovaPaintList>                           DetailPaintListView;
};
