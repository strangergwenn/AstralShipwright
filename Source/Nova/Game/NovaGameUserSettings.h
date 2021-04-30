// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "NovaGameUserSettings.generated.h"

/** Default game mode class */
UCLASS(ClassGroup = (Nova), BlueprintType)
class UNovaGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UNovaGameUserSettings();

	/** Apply custom graphics settings that the engine doesn't know to apply */
	void ApplyCustomGraphicsSettings();

	/** Check if HDR is supported */
	bool IsHDRSupported() const;

	/** Check if DLSS is supported */
	bool IsDLSSSupported() const;

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void SetToDefaults() override;

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

	/** Enable the crash reporter */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableCrashReports;

	/** Mouse sensitivity */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	float MouseSensitivity;

	/** Gamepad sensitivity */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	float GamepadSensitivity;

	/** Vertical FOV in degrees */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	float FOV;

	/** Enable SSGI */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableSSGI;

	/** Enable DLSS */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableDLSS;

	/** Enable DXR reflections */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableRaytracedReflections;

	/** Enable DXR shadows */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableRaytracedShadows;

	/** Enable DXR ambient occlusion */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableRaytracedAO;

	/** Enable convolution bloom */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableCinematicBloom;

	/** Screen percentage */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	float ScreenPercentage;
};
