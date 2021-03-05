// Nova project - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "NovaMainTheme.h"
#include "NovaButtonTheme.h"
#include "NovaSliderTheme.h"

class FNovaStyleSet
{

public:
	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Setup the game resources */
	static void Initialize();

	/** Remove the game resources */
	static void Shutdown();

	/** Get an appropriate icon & overlay text for this key */
	static TPair<FText, const FSlateBrush*> GetKeyDisplay(const FKey& Key);

	/** Get a Slate brush */
	static const FSlateBrush* GetBrush(const FString& Name)
	{
		NCHECK(Name.Len() > 0);
		FString Path = "/" + Name;
		return Instance ? GetStyle().GetBrush(*Path) : nullptr;
	}

	/** Get a new dynamic Slate brush material based on a static one - use FGCObject */
	static TPair<TSharedPtr<struct FSlateBrush>, class UMaterialInstanceDynamic*> GetDynamicBrush(const FString& Name);

	/** Get a color along a gradient using the Plasma palette */
	static FLinearColor GetPlasmaColor(float Alpha);

	/** Get a color along a gradient using the Viridis palette */
	static FLinearColor GetViridisColor(float Alpha);

	/** Get the main theme */
	static const FNovaMainTheme& GetMainTheme(const FName& Name = TEXT("DefaultTheme"))
	{
		NCHECK(Name != NAME_None);
		return GetTheme<FNovaMainTheme>(Name);
	}

	/** Get a button theme */
	static const FNovaButtonTheme& GetButtonTheme(const FName& Name = TEXT("DefaultButton"))
	{
		NCHECK(Name != NAME_None);
		return GetTheme<FNovaButtonTheme>(Name);
	}

	/** Get a button size asset */
	static const FNovaButtonSize& GetButtonSize(const FName& Name = TEXT("DefaultButtonSize"))
	{
		NCHECK(Name != NAME_None);
		return GetTheme<FNovaButtonSize>(Name);
	}

	/** Get a widget theme object */
	template <typename T>
	static const T& GetTheme(const FName& Name = TEXT("Default"))
	{
#if WITH_EDITOR
		if (Instance)
#else
		NCHECK(Instance);
#endif
		{
			return GetStyle().GetWidgetStyle<T>(Name);
		}
#if WITH_EDITOR
		else
		{
			return T::GetDefault();
		}
#endif
	}

	/** Get a reference to the current data */
	static const ISlateStyle& GetStyle()
	{
		return *Instance;
	}

	/*----------------------------------------------------
	    Internal
	----------------------------------------------------*/

protected:
	/** Setup resources (internal) */
	static TSharedRef<FSlateStyleSet> Create();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	/** Resource pointer */
	UPROPERTY()
	static TSharedPtr<FSlateStyleSet> Instance;
};
