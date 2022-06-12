// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Neutron/Settings/NeutronGameUserSettings.h"
#include "NovaGameUserSettings.generated.h"

/** Default game mode class */
UCLASS(ClassGroup = (Nova), BlueprintType)
class UNovaGameUserSettings : public UNeutronGameUserSettings
{
	GENERATED_BODY()

public:

	UNovaGameUserSettings();

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void SetToDefaults() override;

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

	/** Enable post-processing */
	UPROPERTY(Config, BlueprintReadOnly, VisibleAnywhere)
	bool EnableCameraDegradation;
};
