// Nova project - Gwennaël Arbona

#pragma once

#include "NovaButton.h"
#include "NovaListView.h"
#include "NovaMenu.h"
#include "NovaModalPanel.h"
#include "Nova/UI/NovaUI.h"
#include "Nova/Nova.h"

/** Templatized, modal, list class to display elements from a TArray */
template <typename ItemType>
class SNovaModalListView : public SNovaButton
{
public:
	typedef TPair<int32, TArray<ItemType>> SelfRefreshType;

	DECLARE_DELEGATE_RetVal(SelfRefreshType, FNovaOnSelfRefresh);
	DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FNovaOnGenerateItem, ItemType);
	DECLARE_DELEGATE_RetVal_OneParam(FText, FNovaOnGenerateName, ItemType);
	DECLARE_DELEGATE_RetVal_OneParam(FText, FNovaOnGenerateTooltip, ItemType);
	DECLARE_DELEGATE_TwoParams(FNovaListSelectionChanged, ItemType, int32);

private:
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaModalListView<ItemType>)
		: _Panel(nullptr)
		, _ItemsSource(nullptr)
		, _Enabled(true)
		, _ButtonTheme("DefaultButton")
		, _ButtonSize("DefaultButtonSize")
		, _ListButtonTheme("DefaultButton")
		, _ListButtonSize("DoubleButtonSize")
	{}

	SLATE_ARGUMENT(SNovaNavigationPanel*, Panel)
	SLATE_ARGUMENT(const TArray<ItemType>*, ItemsSource)
	SLATE_ATTRIBUTE(FText, TitleText)
	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ATTRIBUTE(FName, Action)
	SLATE_ATTRIBUTE(bool, Enabled)
	SLATE_ATTRIBUTE(bool, Focusable)

	SLATE_ARGUMENT(FSimpleDelegate, OnDoubleClicked)
	SLATE_ARGUMENT(FNovaOnSelfRefresh, OnSelfRefresh)
	SLATE_EVENT(FNovaOnGenerateItem, OnGenerateItem)
	SLATE_EVENT(FNovaOnGenerateName, OnGenerateName)
	SLATE_EVENT(FNovaOnGenerateTooltip, OnGenerateTooltip)
	SLATE_EVENT(FNovaListSelectionChanged, OnSelectionChanged)

	SLATE_ARGUMENT(FName, ButtonTheme)
	SLATE_ARGUMENT(FName, ButtonSize)
	SLATE_ARGUMENT(FName, ListButtonTheme)
	SLATE_ARGUMENT(FName, ListButtonSize)
	SLATE_ARGUMENT(FNovaButtonUserSizeCondition, UserSizeCallback)
	SLATE_NAMED_SLOT(FArguments, ButtonContent)

	SLATE_END_ARGS()

	/*----------------------------------------------------
	    Constructor
	----------------------------------------------------*/

public:
	void Construct(const FArguments& InArgs)
	{
		TitleText          = InArgs._TitleText;
		HelpText           = InArgs._HelpText;
		OnSelfRefresh      = InArgs._OnSelfRefresh;
		OnGenerateName     = InArgs._OnGenerateName;
		OnSelectionChanged = InArgs._OnSelectionChanged;

		ListPanel = InArgs._Panel->GetMenu()->CreateModalPanel();

		SNovaButton::Construct(
			SNovaButton::FArguments()
				.Text((OnGenerateName.IsBound() || TitleText.IsSet() || TitleText.IsBound())
						  ? TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SNovaModalListView::GetButtonText))
						  : TAttribute<FText>())
				.HelpText(HelpText)
				.Action(InArgs._Action)
				.Theme(InArgs._ButtonTheme)
				.UserSizeCallback(InArgs._UserSizeCallback)
				.Size(InArgs._ButtonSize)
				.Enabled(InArgs._Enabled)
				.Focusable(InArgs._Focusable)
				.OnClicked(this, &SNovaModalListView::OnOpenList)
				.OnDoubleClicked(InArgs._OnDoubleClicked)
				.Content()[InArgs._ButtonContent.Widget]);

		SAssignNew(ListView, SNovaListView<ItemType>)
			.Panel(ListPanel.Get())
			.ItemsSource(InArgs._ItemsSource ? InArgs._ItemsSource : &InternalItemsSource)
			.OnGenerateItem(InArgs._OnGenerateItem)
			.OnGenerateTooltip(InArgs._OnGenerateTooltip)
			.OnSelectionChanged(this, &SNovaModalListView::OnListSelectionChanged)
			.OnSelectionDoubleClicked(this, &SNovaModalListView::OnListConfirmed)
			.ButtonTheme(InArgs._ListButtonTheme)
			.ButtonSize(InArgs._ListButtonSize);
	}

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

public:
	/** Refresh the list based on the items source */
	void Refresh(int32 SelectedIndex = INDEX_NONE)
	{
		ListView->Refresh(SelectedIndex);
		CurrentConfirmed = ListView->GetSelectedItem();
	}

	/** Show the list */
	void Show()
	{
		OnOpenList();
	}

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	FText GetButtonText() const
	{
		if (OnGenerateName.IsBound())
		{
			return OnGenerateName.Execute(CurrentConfirmed);
		}
		else
		{
			return TitleText.Get();
		}
	}

	void OnOpenList()
	{
		CurrentUserSelectedIndex = INDEX_NONE;
		int32 CurrentListIndex   = ListView->GetSelectedIndex();

		if (OnSelfRefresh.IsBound())
		{
			auto RefreshResult = OnSelfRefresh.Execute();

			if (RefreshResult.Key != INDEX_NONE)
			{
				CurrentListIndex = RefreshResult.Key;
			}
			InternalItemsSource = RefreshResult.Value;
		}

		ListPanel->Show(TitleText.Get(), HelpText.Get(), FSimpleDelegate::CreateSP(this, &SNovaModalListView::OnListConfirmed),
			FSimpleDelegate(), FSimpleDelegate(), ListView);

		ListView->Refresh(CurrentListIndex);
	}

	void OnListConfirmed()
	{
		if (CurrentUserSelectedIndex != INDEX_NONE)
		{
			CurrentConfirmed = CurrentSelected;
			OnSelectionChanged.ExecuteIfBound(CurrentSelected, CurrentUserSelectedIndex);
		}

		ListPanel->Hide();
	}

	void OnListSelectionChanged(ItemType Selected, int32 Index)
	{
		CurrentSelected          = Selected;
		CurrentUserSelectedIndex = Index;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// State
	TAttribute<FText>         TitleText;
	TAttribute<FText>         HelpText;
	ItemType                  CurrentSelected;
	ItemType                  CurrentConfirmed;
	int32                     CurrentUserSelectedIndex;
	FSimpleDelegate           OnOpen;
	FNovaOnSelfRefresh        OnSelfRefresh;
	FNovaOnGenerateName       OnGenerateName;
	FNovaListSelectionChanged OnSelectionChanged;

	// Internal list
	TArray<ItemType> InternalItemsSource;

	// Widgets
	TSharedPtr<SNovaModalPanel>         ListPanel;
	TSharedPtr<SNovaListView<ItemType>> ListView;
};
