// Nova project - Gwennaël Arbona

#include "NovaScalingRule.h"

float UNovaScalingRule::GetDPIScaleBasedOnSize(FIntPoint Size) const
{
	float NominalAspectRatio = (16.0f / 9.0f);

	// Define the axis-independent rule : scaled under 720p, constant under 1080p, scaled above 1440p
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
		return DimensionalRule(Size.Y, 720, 1080, 1440);
	}
	else
	{
		return DimensionalRule(Size.X, 1280, 1280, 2560);
	}
}
