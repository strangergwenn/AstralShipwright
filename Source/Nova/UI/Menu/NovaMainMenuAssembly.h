// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaModalListView.h"
#include "Nova/Game/NovaGameTypes.h"

#include "Online.h"

enum class ENovaAssemblyDisplayFilter : uint8;
enum class ENovaHullType : uint8;

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

	virtual void Hide() override;

	virtual void Next() override;

	virtual void Previous() override;

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

	/** Select a module or equipment */
	void SetSelectedModuleOrEquipment(int32 Index);

	/** Set whether the compartment sub-menu is active */
	void SetCompartmentPanelVisible(bool Active);

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
	EVisibility         GetModuleListVisibility() const;
	TSharedRef<SWidget> GenerateModuleItem(const class UNovaModuleDescription* Module) const;
	FText               GetModuleListTitle(const class UNovaModuleDescription* Module) const;
	FText               GenerateModuleTooltip(const class UNovaModuleDescription* Module) const;

	// Compartment equipment list
	EVisibility         GetEquipmentListVisibility() const;
	TSharedRef<SWidget> GenerateEquipmentItem(const class UNovaEquipmentDescription* Equipment) const;
	FText               GetEquipmentListTitle(const class UNovaEquipmentDescription* Equipment) const;
	FText               GenerateEquipmentTooltip(const class UNovaEquipmentDescription* Equipment) const;

	// Compartment hull types list
	TSharedRef<SWidget> GenerateHullTypeItem(ENovaHullType Type) const;
	FText               GetHullTypeListTitle(ENovaHullType Type) const;
	FText               GetHullTypeName(ENovaHullType Type) const;
	FText               GenerateHullTypeTooltip(ENovaHullType Type) const;

	// Helpers
	TSharedRef<SWidget> GenerateAssetItem(const class UNovaAssetDescription* Asset) const;
	FText               GetAssetName(const class UNovaAssetDescription* Asset) const;

	// Panels
	FLinearColor GetMainColor() const;
	FLinearColor GetCompartmentColor() const;

	// Assembly callbacks
	FText GetSelectedFilterText() const;
	bool  IsSelectCompartmentEnabled(int32 Index) const;
	bool  IsAddCompartmentEnabled(bool Forward) const;
	bool  IsEditCompartmentEnabled() const;

	// Compartment callbacks
	bool IsBackToAssemblyEnabled() const;
	bool IsModuleEnabled(int32 ModuleIndex) const;
	bool IsEquipmentEnabled(int32 EquipmentIndex) const;

	// Key bindings
	FKey GetPreviousItemKey() const;
	FKey GetNextItemKey() const;

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
	void OnCompartmentSelected(int32 Index);

	// Display filters
	void OnEnterPhotoMode(FName ActionName);
	void OnSelectedFilterChanged(float Value);

	// Modules & equipments
	void OnSelectedModuleChanged(const class UNovaModuleDescription* Module, int32 Index);
	void OnSelectedEquipmentChanged(const class UNovaEquipmentDescription* Equipment, int32 Index);
	void OnSelectedHullTypeChanged(ENovaHullType Type, int32 Index);

	// Save the spacecraft
	void OnSaveSpacecraft();

	// Exit compartment details
	void OnBackToAssembly();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Menu manager
	TWeakObjectPtr<UNovaMenuManager> MenuManager;

	// Widgets
	TSharedPtr<SHorizontalBox>        CompartmentBox;
	TSharedPtr<SHorizontalBox>        ModuleBox;
	TSharedPtr<SHorizontalBox>        EquipmentBox;
	TSharedPtr<class SNovaButton>     SaveCompartmentButton;
	TSharedPtr<class SNovaModalPanel> ModalPanel;
	TSharedPtr<SVerticalBox>          MenuBox;

	// Panel fading system
	float FadeDuration;
	float CurrentFadeTime;
	bool  IsCompartmentPanelVisible;

	// Assembly data
	int32 SelectedCompartmentIndex;
	int32 EditedCompartmentIndex;
	int32 SelectedModuleOrEquipmentIndex;

	// Compartment list
	TArray<ENovaHullType>         HullTypeList;
	TSharedPtr<SNovaHullTypeList> HullTypeListView;

	// Compartment module list
	TArray<const class UNovaModuleDescription*> ModuleList;
	TSharedPtr<SNovaModuleList>                 ModuleListView;

	// Compartment equipment list
	TArray<const class UNovaEquipmentDescription*> EquipmentList;
	TSharedPtr<SNovaEquipmentList>                 EquipmentListView;
};
