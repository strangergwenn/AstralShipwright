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

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void SetToDefaults() override;

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

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
