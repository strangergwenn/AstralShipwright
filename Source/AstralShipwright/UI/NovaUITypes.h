// Astral Shipwright - Gwennaël Arbona

#pragma once

/*----------------------------------------------------
    General purpose types
----------------------------------------------------*/

// Callbacks
DECLARE_DELEGATE(FNovaAsyncAction);
DECLARE_DELEGATE_RetVal(bool, FNovaAsyncCondition);

/** UI constants */
namespace ENovaUIConstants {
constexpr float EaseLight           = 1.5f;
constexpr float EaseStandard        = 2.0f;
constexpr float EaseStrong          = 4.0f;
constexpr float FadeDurationMinimal = 0.1f;
constexpr float FadeDurationShort   = 0.25f;
constexpr float FadeDurationLong    = 0.4f;
constexpr float BlurAlphaOffset     = 0.25f;
}    // namespace ENovaUIConstants

/** Notification type */
enum class ENovaNotificationType : uint8
{
	Info,
	Error,
	Save,
	World,
	Time
};

/** Loading screen types */
enum class ENovaLoadingScreen : uint8
{
	None,
	Launch,
	Black
};

/*----------------------------------------------------
    Player input types
----------------------------------------------------*/

/** Player input bindings */
class FNovaPlayerInput
{
public:
	// Game-specific menu actions
	static const FName MenuPrimary;
	static const FName MenuSecondary;
	static const FName MenuAltPrimary;
	static const FName MenuAltSecondary;
	static const FName MenuNextTab;
	static const FName MenuPreviousTab;
	static const FName MenuToggle;

	// Common actions
	static const FName MenuMoveHorizontal;
	static const FName MenuMoveVertical;
	static const FName MenuConfirm;
	static const FName MenuCancel;
	static const FName MenuUp;
	static const FName MenuDown;
	static const FName MenuLeft;
	static const FName MenuRight;
	static const FName MenuNext;
	static const FName MenuPrevious;
	static const FName MenuZoomIn;
	static const FName MenuZoomOut;
	static const FName MenuAnalogHorizontal;
	static const FName MenuAnalogVertical;
};

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

/** Carousel type animation */
template <int Size>
struct FNovaCarouselAnimation
{
	FNovaCarouselAnimation() : AnimationDuration(0.0f)
	{}

	FNovaCarouselAnimation(float Duration, float EaseValue = ENovaUIConstants::EaseLight)
	{
		Initialize(Duration, EaseValue);
	}

	/** Initialize the structure */
	void Initialize(float Duration, float EaseValue = ENovaUIConstants::EaseLight)
	{
		AnimationDuration = Duration;
		AnimationEase     = EaseValue;
		CurrentTotalAlpha = 1.0f;

		FMemory::Memset(AlphaValues, 0, Size);
		FMemory::Memset(InterpolatedAlphaValues, 0, Size);
	}

	/** Feed the currently selected index */
	void Update(int32 SelectedIndex, float DeltaTime)
	{
		NCHECK(AnimationDuration > 0);

		CurrentTotalAlpha = 0.0f;
		for (int32 Index = 0; Index < Size; Index++)
		{
			if (Index == SelectedIndex)
			{
				AlphaValues[Index] += DeltaTime / AnimationDuration;
			}
			else
			{
				AlphaValues[Index] -= DeltaTime / AnimationDuration;
			}
			AlphaValues[Index] = FMath::Clamp(AlphaValues[Index], 0.0f, 1.0f);

			InterpolatedAlphaValues[Index] = FMath::InterpEaseInOut(0.0f, 1.0f, AlphaValues[Index], AnimationEase);
			CurrentTotalAlpha += InterpolatedAlphaValues[Index];
		}
	}

	/** Get the current animation alpha for this index */
	float GetAlpha(int32 Index) const
	{
		return CurrentTotalAlpha > 0.0f ? InterpolatedAlphaValues[Index] / CurrentTotalAlpha : 0.0f;
	}

private:
	// Animation data
	float AnimationDuration;
	float AnimationEase;
	float CurrentTotalAlpha;
	float AlphaValues[Size];
	float InterpolatedAlphaValues[Size];
};