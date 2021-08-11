// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/System/NovaMenuManager.h"
#include "Nova/UI/NovaUI.h"
#include "Nova/Nova.h"

#include "Widgets/Images/SImage.h"
#include "GameFramework/GameUserSettings.h"

/*----------------------------------------------------
    Window manipulator
----------------------------------------------------*/

class SNovaMenuManipulator : public SImage
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMenuManipulator) : _ColorAndOpacity(FLinearColor::White)
	{}

	SLATE_ARGUMENT(const FSlateBrush*, Image)
	SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)

	SLATE_END_ARGS()

public:
	SNovaMenuManipulator() : Moving(false)
	{
		SetCanTick(true);
	}

	void Construct(const FArguments& InArgs)
	{
		SImage::Construct(SImage::FArguments().Image(InArgs._Image).ColorAndOpacity(InArgs._ColorAndOpacity));
	}

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		SImage::OnMouseButtonDown(MyGeometry, MouseEvent);

		TSharedPtr<SWindow> Window = FSlateApplication::Get().GetTopLevelWindows()[0];
		NCHECK(Window);

		// Start moving when dragged
		if (!Window->IsWindowMaximized() && GEngine->GetGameUserSettings()->GetFullscreenMode() == EWindowMode::Windowed)
		{
			Moving = true;
			Origin = FSlateApplication::Get().GetCursorPos() - Window->GetPositionInScreen();
		}

		return FReply::Handled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		FReply Result = SImage::OnMouseButtonUp(MyGeometry, MouseEvent);

		TSharedPtr<SWindow> Window = FSlateApplication::Get().GetTopLevelWindows()[0];
		NCHECK(Window);

		// Auto-maximize the window when dragged to the top border of the screen
		if (!Window->IsWindowMaximized() && GEngine->GetGameUserSettings()->GetFullscreenMode() == EWindowMode::Windowed && Moving &&
			FSlateApplication::Get().GetCursorPos().Y <= 0)
		{
			UNovaMenuManager::Get()->MaximizeOrRestore();
			Result = FReply::Handled();
		}

		Moving = false;

		return Result;
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		SImage::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);

		UNovaMenuManager::Get()->MaximizeOrRestore();

		return FReply::Handled();
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override
	{
		SImage::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		TSharedPtr<SWindow> Window = FSlateApplication::Get().GetTopLevelWindows()[0];
		NCHECK(Window);

		if (Moving && !Window->IsWindowMaximized())
		{
			if (!Window->GetNativeWindow()->IsForegroundWindow())
			{
				Moving = false;
			}

			Window->MoveWindowTo(FSlateApplication::Get().GetCursorPos() - Origin);
		}
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	bool      Moving;
	FVector2D Origin;
};
