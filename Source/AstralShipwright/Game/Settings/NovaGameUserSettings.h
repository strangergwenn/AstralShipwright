// Astral Shipwright - Gwennaël Arbona

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

	/** Check if Nanite is supported */
	bool IsNaniteSupported() const;

	/** Check if Lumen is supported */
	bool IsLumenSupported() const;

	/** Check if Lumen hardware ray-tracing is supported */
	bool IsLumenHWRTSupported() const;

	/** Check if virtual shadowmaps are supported */
	bool IsVirtualShadowSupported() const;

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

	/** Global sound volume */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	int32 MasterVolume;

	/** Music volume */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	int32 MusicVolume;

	/** Music volume */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	int32 EffectsVolume;

	/** Vertical FOV in degrees */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	float FOV;

	/** Enable TSR */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableTSR;

	/** Enable DLSS */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableDLSS;

	/** Enable Nanite */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableNanite;

	/** Enable Lumen */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableLumen;

	/** Enable Lumen support for hardware ray-tracing */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableLumenHWRT;

	/** Enable virtual shadow maps */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableVirtualShadows;

	/** Enable convolution bloom */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableCinematicBloom;

	/** Screen percentage */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	float ScreenPercentage;
};
