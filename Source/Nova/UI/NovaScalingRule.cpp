// Nova project - Gwennaël Arbona

#include "NovaScalingRule.h"

float UNovaScalingRule::GetDPIScaleBasedOnSize(FIntPoint Size) const
{
	float NominalAspectRatio = (16.0f / 9.0f);

	// Define the axis-independent rule : scaled against ReferenceSize when under SmallSize or above LargeSize
	auto DimensionalRule = [](float CurrentSize, float SmallSize, float ReferenceSize, float LargeSize)
	{
		if (CurrentSize < SmallSize || CurrentSize > LargeSize)
		{
			return CurrentSize / ReferenceSize;
		}
		else
		{
			return 1.0f;
		}
	};

	// Apply rule per axis depending on wide or narrow screen
	if (Size.X == 0 || Size.Y == 0)
	{
		return 1.0f;
	}
	else if (Size.X >= NominalAspectRatio * Size.Y)
	{
		return DimensionalRule(Size.Y, 1080, 1080, 1440);
	}
	else
	{
		return DimensionalRule(Size.X, 1920, 1920, 2560);
	}
}
