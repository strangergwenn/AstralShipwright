// Nova project - Gwennaël Arbona

#include "NovaSpacecraftTypes.h"

/*----------------------------------------------------
    Module data asset
----------------------------------------------------*/

TSoftObjectPtr<class UStaticMesh> UNovaModuleDescription::GetBulkhead(ENovaBulkheadType Style, bool Forward) const
{
	switch (Style)
	{
		case ENovaBulkheadType::None:
			return nullptr;
		case ENovaBulkheadType::Standard:
			return Forward ? ForwardBulkhead : AftBulkhead;
		case ENovaBulkheadType::Skirt:
			return Forward ? nullptr : SkirtBulkhead;
		case ENovaBulkheadType::Outer:
			return Forward ? OuterForwardBulkhead : OuterAftBulkhead;
		default:
			return nullptr;
	}
}

/*----------------------------------------------------
    Equipment data asset
----------------------------------------------------*/

TSoftObjectPtr<UObject> UNovaEquipmentDescription::GetMesh() const
{
	if (!SkeletalEquipment.IsNull())
	{
		return SkeletalEquipment;
	}
	else if (!StaticEquipment.IsNull())
	{
		return StaticEquipment;
	}
	else
	{
		return nullptr;
	}
}
