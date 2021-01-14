// Nova project - Gwennaël Arbona

#include "NovaStyleSet.h"
#include "Nova/Nova.h"

#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"


#define GAIA_STYLE_INSTANCE_NAME "NovaStyle"
#define GAIA_STYLE_PATH          "/Game/UI"


/*----------------------------------------------------
	Singleton
----------------------------------------------------*/

TSharedPtr<FSlateStyleSet> FNovaStyleSet::Instance = NULL;

void FNovaStyleSet::Initialize()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(GAIA_STYLE_INSTANCE_NAME);
	
	if (!Instance.IsValid())
	{
		Instance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*Instance);
	}
}

void FNovaStyleSet::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*Instance);

	NCHECK(Instance.IsUnique());
	Instance.Reset();
}

TPair<FText, const FSlateBrush*> FNovaStyleSet::GetKeyDisplay(const FKey& Key)
{
	if (Key.IsValid())
	{
		FString DisplayBrushName;
		FText DisplayName;

		// Special keyboard keys
		if (Key == EKeys::LeftShift)
		{
			DisplayName = FText::FromString("SFT");
		}
		else if (Key == EKeys::RightShift)
		{
			DisplayName = FText::FromString("RSFT");
		}
		else if (Key == EKeys::LeftControl)
		{
			DisplayName = FText::FromString("CTL");
		}
		else if (Key == EKeys::RightControl)
		{
			DisplayName = FText::FromString("RCTL");
		}
		else if (Key == EKeys::LeftCommand)
		{
			DisplayName = FText::FromString("CMD");
		}
		else if (Key == EKeys::RightCommand)
		{
			DisplayName = FText::FromString("RCMD");
		}
		else if (Key == EKeys::LeftAlt)
		{
			DisplayName = FText::FromString("ALT");
		}
		else if (Key == EKeys::RightAlt)
		{
			DisplayName = FText::FromString("RALT");
		}
		else if (Key == EKeys::Backslash)
		{
			DisplayName = FText::FromString("RET");
		}
		else if (Key == EKeys::NumLock)
		{
			DisplayName = FText::FromString("NUM");
		}
		else if (Key == EKeys::ScrollLock)
		{
			DisplayName = FText::FromString("SCR");
		}
		else if (Key == EKeys::PageDown)
		{
			DisplayName = FText::FromString("PDN");
		}
		else if (Key == EKeys::PageUp)
		{
			DisplayName = FText::FromString("PUP");
		}
		else if (Key == EKeys::Down)
		{
			DisplayBrushName = "Down";
		}
		else if (Key == EKeys::Left)
		{
			DisplayBrushName = "Left";
		}
		else if (Key == EKeys::Right)
		{
			DisplayBrushName = "Right";
		}
		else if (Key == EKeys::Up)
		{
			DisplayBrushName = "Up";
		}

		// Mouse keys
		else if (Key == EKeys::LeftMouseButton)
		{
			DisplayName = FText::FromString("LM");
		}
		else if (Key == EKeys::RightMouseButton)
		{
			DisplayName = FText::FromString("RM");
		}
		else if (Key == EKeys::MiddleMouseButton)
		{
			DisplayName = FText::FromString("MM");
		}
		else if (Key == EKeys::MouseScrollDown)
		{
			DisplayBrushName = "MouseDown";
		}
		else if (Key == EKeys::MouseScrollUp)
		{
			DisplayBrushName = "MouseUp";
		}
		else if (Key == EKeys::ThumbMouseButton)
		{
			DisplayName = FText::FromString("M4");
		}
		else if (Key == EKeys::ThumbMouseButton2)
		{
			DisplayName = FText::FromString("M5");
		}

		// Gamepad
		else if (Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			DisplayBrushName = "A";
		}
		else if (Key == EKeys::Gamepad_FaceButton_Left)
		{
			DisplayBrushName = "X";
		}
		else if (Key == EKeys::Gamepad_FaceButton_Right)
		{
			DisplayBrushName = "B";
		}
		else if (Key == EKeys::Gamepad_FaceButton_Top)
		{
			DisplayBrushName = "Y";
		}
		else if (Key == EKeys::Gamepad_DPad_Down)
		{
			DisplayBrushName = "Down";
		}
		else if (Key == EKeys::Gamepad_DPad_Left)
		{
			DisplayBrushName = "Left";
		}
		else if (Key == EKeys::Gamepad_DPad_Right)
		{
			DisplayBrushName = "Right";
		}
		else if (Key == EKeys::Gamepad_DPad_Up)
		{
			DisplayBrushName = "Up";
		}
		else if (Key == EKeys::Gamepad_LeftShoulder)
		{
			DisplayName = FText::FromString("LB");
		}
		else if (Key == EKeys::Gamepad_RightShoulder)
		{
			DisplayName = FText::FromString("RB");
		}
		else if (Key == EKeys::Gamepad_Special_Right)
		{
			DisplayBrushName = "Menu";
		}
		else if (Key == EKeys::Gamepad_Special_Left)
		{
			DisplayBrushName = "View";
		}

		// Defaults : default brush if no brush set, uppercase name if no text set
		if (DisplayBrushName.Len() == 0)
		{
			DisplayBrushName = "None";
			if (DisplayName.ToString().Len() == 0)
			{
				DisplayName = FText::FromString(Key.GetDisplayName(false).ToString().ToUpper());
			}
		}

		// Return details
		const FSlateBrush* DisplayBrush = FNovaStyleSet::GetBrush(*(FString(TEXT("Key/SB_")) + DisplayBrushName));
		NCHECK(DisplayBrush);
		return TPair<FText, const FSlateBrush*>(DisplayName, DisplayBrush);
	}

	return TPair<FText, const FSlateBrush*>(FText(), nullptr);
}

TPair<TSharedPtr<FSlateBrush>, UMaterialInstanceDynamic*> FNovaStyleSet::GetDynamicBrush(const FString& BrushName)
{
	TSharedPtr<FSlateBrush> ResultBrush = MakeShareable(new FSlateBrush());
	const FSlateBrush* BaseBrush = FNovaStyleSet::GetBrush(BrushName);

	// Create material
	UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(BaseBrush->GetResourceObject());
	NCHECK(BaseMaterial);
	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
	NCHECK(DynamicMaterial);

	// Update icon brush
	FVector2D ImageSize = BaseBrush->ImageSize;
	ResultBrush->SetResourceObject(DynamicMaterial);
	ResultBrush->ImageSize = ImageSize;

	return TPair<TSharedPtr<FSlateBrush>, UMaterialInstanceDynamic*>(ResultBrush, DynamicMaterial);
}


/*----------------------------------------------------
	Internal
----------------------------------------------------*/

TSharedRef< FSlateStyleSet > FNovaStyleSet::Create()
{
	TSharedRef<FSlateStyleSet> StyleRef = FSlateGameResources::New(FName(GAIA_STYLE_INSTANCE_NAME), GAIA_STYLE_PATH, GAIA_STYLE_PATH);
	FSlateStyleSet& Style = StyleRef.Get();

	Style.Set("Nova.Button", FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
		.SetDisabled(FSlateNoResource())
		.SetNormalPadding(0)
		.SetPressedPadding(0)
	);
	
	return StyleRef;
}
