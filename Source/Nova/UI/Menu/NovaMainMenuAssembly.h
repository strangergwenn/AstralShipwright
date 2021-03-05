// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaModalListView.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"

#include "Online.h"

class SNovaMainMenuAssembly : public SNovaTabPanel
{
	typedef SNovaModalListView<const class UNovaCompartmentDescription*> SNovaCompartmentList;
	typedef SNovaModalListView<const class UNovaModuleDescription*>      SNovaModuleList;
	typedef SNovaModalListView<const class UNovaEquipmentDescription*>   SNovaEquipmentList;
	typedef SNovaModalListView<ENovaAssemblyDisplayFilter>               SNovaDisplayFilterList;
	typedef SNovaModalListView<ENovaHullType>                            SNovaHullTypeList;

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

	virtual void Show() override;

	virtual void AbilityPrimary() override;

	virtual void AbilitySecondary() override;

	virtual void ZoomIn() override;

	virtual void ZoomOut() override;

	virtual bool Cancel() override;

	virtual void OnClicked(const FVector2D& Position) override;

	virtual void OnDoubleClicked(const FVector2D& Position) override;

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;

	virtual TSharedPtr<SNovaButton> GetDefaultFocusButton() const override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Get the spacecraft pawn used to display the assembly */
	class ANovaSpacecraftPawn* GetSpacecraftPawn() const;

	/** Get the index of the next compartment to build */
	int32 GetNewBuildIndex(bool Forward) const;

	/** Select a compartment index */
	void SetSelectedCompartment(int32 Index);

	/** Set whether the compartment sub-menu is active */
	void SetCompartmentPanelVisible(bool Active);

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	// Helpers
	TSharedRef<SWidget> GenerateAssetItem(const UNovaAssetDescription* Asset) const;
	FText               GetAssetName(const class UNovaAssetDescription* Asset) const;

	// Panels
	FLinearColor GetMainColor() const;
	FLinearColor GetCompartmentColor() const;

	// Compartment template list
	TSharedRef<SWidget> GenerateCompartmentItem(const class UNovaCompartmentDescription* Description) const;
	FText               GenerateCompartmentTooltip(const class UNovaCompartmentDescription* Description) const;

	// Compartment module list
	bool                IsModuleListEnabled(int32 ModuleIndex) const;
	TSharedRef<SWidget> GenerateModuleItem(const class UNovaModuleDescription* Module) const;
	FText               GetModuleName(const class UNovaModuleDescription* Module) const;
	FText               GenerateModuleTooltip(const class UNovaModuleDescription* Module) const;

	// Compartment equipment list
	bool                IsEquipmentListEnabled(int32 EquipmentIndex) const;
	TSharedRef<SWidget> GenerateEquipmentItem(const class UNovaEquipmentDescription* Equipment) const;
	FText               GetEquipmentName(const class UNovaEquipmentDescription* Equipment) const;
	FText               GenerateEquipmentTooltip(const class UNovaEquipmentDescription* Equipment) const;

	// Compartment hull types list
	TSharedRef<SWidget> GenerateHullTypeItem(ENovaHullType Type) const;
	FText               GetHullTypeName(ENovaHullType Type) const;
	FText               GenerateHullTypeTooltip(ENovaHullType Type) const;

	// General callbacks
	const FSlateBrush* GetCompartmentIcon(int32 Index) const;
	bool               IsSelectCompartmentEnabled(int32 Index) const;
	bool               IsAddCompartmentEnabled(bool Forward) const;
	bool               IsEditCompartmentEnabled() const;
	bool               IsToggleHighlightEnabled() const;

	// Key bindings
	FKey GetPreviousCompartmentKey() const;
	FKey GetNextCompartmentKey() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	// Compartment edition
	void OnEditCompartment();
	void OnRemoveCompartment();
	void OnRemoveCompartmentConfirmed();

	// Compartment selection
	void OnSelectedCompartmentChanged(const class UNovaCompartmentDescription* Compartment, int32 Index, bool Forward);
	void OnSelectedFilterChanged(float Value);
	void OnCompartmentSelected(int32 Index);

	// Modules & equipments
	void OnSelectedModuleChanged(const class UNovaModuleDescription* Module, int32 Index, int32 SlotIndex);
	void OnSelectedEquipmentChanged(const class UNovaEquipmentDescription* Equipment, int32 Index, int32 SlotIndex);
	void OnSelectedHullTypeChanged(ENovaHullType Type, int32 Index);

	// Save the spacecraft
	void OnSaveSpacecraft();

	// Exit compartment details
	void OnBackToAssembly();

	// Display options
	void OnToggleHighlight();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Menu manager
	TWeakObjectPtr<UNovaMenuManager> MenuManager;

	// Widgets
	TSharedPtr<SHorizontalBox>        CompartmentBox;
	TSharedPtr<SVerticalBox>          ModuleBox;
	TSharedPtr<SVerticalBox>          EquipmentBox;
	TSharedPtr<class SNovaButton>     HighlightButton;
	TSharedPtr<class SNovaButton>     EditCompartmentButton;
	TSharedPtr<class SNovaModalPanel> ModalPanel;
	TSharedPtr<SVerticalBox>          MenuBox;

	// Panel fading system
	float FadeDuration;
	float CurrentFadeTime;
	bool  IsCompartmentPanelVisible;

	// Assembly data
	int32 SelectedCompartmentIndex;
	int32 EditedCompartmentIndex;

	// Compartment list
	TSharedPtr<SNovaCompartmentList> CompartmentListView;

	// Compartment hull  list
	TArray<ENovaHullType>         HullTypeList;
	TSharedPtr<SNovaHullTypeList> HullTypeListView;

	// Compartment module lists
	TArray<const class UNovaModuleDescription*> ModuleLists[ENovaConstants::MaxModuleCount];
	TSharedPtr<SNovaModuleList>                 ModuleListViews[ENovaConstants::MaxModuleCount];

	// Compartment equipment lists
	TArray<const class UNovaEquipmentDescription*> EquipmentLists[ENovaConstants::MaxEquipmentCount];
	TSharedPtr<SNovaEquipmentList>                 EquipmentListViews[ENovaConstants::MaxEquipmentCount];
};
