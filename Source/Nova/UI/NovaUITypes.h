// Nova project - Gwennaël Arbona

#pragma once

/*----------------------------------------------------
    General purpose types
----------------------------------------------------*/

// Callbacks
DECLARE_DELEGATE(FNovaAsyncAction);
DECLARE_DELEGATE_RetVal(bool, FNovaAsyncCondition);

/** UI constants */
namespace ENovaUIConstants
{
constexpr float EaseLight           = 1.5f;
constexpr float EaseStandard        = 2.0f;
constexpr float EaseStrong          = 4.0f;
constexpr float FadeDurationMinimal = 0.1f;
constexpr float FadeDurationShort   = 0.25f;
constexpr float FadeDurationLong    = 0.5f;
}    // namespace ENovaUIConstants

/** Notification type */
enum class ENovaNotificationType : uint8
{
	Info,
	World,
	Saved,
	Error
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
	static const FName MenuZoomIn;
	static const FName MenuZoomOut;
	static const FName MenuAnalogHorizontal;
	static const FName MenuAnalogVertical;
};
