// Nova project - Gwennaël Arbona

#pragma once

#include "Engine/DPICustomScalingRule.h"
#include "NovaScalingRule.generated.h"


UCLASS()
class UNovaScalingRule : public UDPICustomScalingRule
{
	GENERATED_BODY()

public:

	virtual float GetDPIScaleBasedOnSize(FIntPoint Size) const override;

};
