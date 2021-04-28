// Nova project - Gwennaël Arbona

#include "NovaSpacecraftTypes.h"

#define LOCTEXT_NAMESPACE "NovaSpacecraftTypes"

/*----------------------------------------------------
    Compartment data asset
----------------------------------------------------*/

TArray<FName> UNovaCompartmentDescription::GetGroupedEquipmentSlotsNames(int32 Index) const
{
	TArray<FName> GroupedSocketNames;

	if (Index >= 0 && Index < EquipmentSlots.Num())
	{
		FName CurrentSocketName = EquipmentSlots[Index].SocketName;

		for (int32 GroupIndex = 0; GroupIndex < EquipmentSlotsGroups.Num(); GroupIndex++)
		{
			const TArray<FName>& GroupSocketNames = EquipmentSlotsGroups[GroupIndex].SocketNames;

			for (FName SocketName : GroupSocketNames)
			{
				if (SocketName == CurrentSocketName)
				{
					TArray<FName> OtherSocketNames = GroupSocketNames;
					OtherSocketNames.Remove(CurrentSocketName);
					GroupedSocketNames.Append(OtherSocketNames);
					break;
				}
			}
		}
	}

	return GroupedSocketNames;
}

TArray<int32> UNovaCompartmentDescription::GetGroupedEquipmentSlotsIndices(int32 Index) const
{
	TArray<int32> GroupedSlotsIndices;

	for (FName SocketName : GetGroupedEquipmentSlotsNames(Index))
	{
		for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); SlotIndex++)
		{
			const FNovaEquipmentSlot& Slot = EquipmentSlots[SlotIndex];

			if (Slot.SocketName == SocketName)
			{
				GroupedSlotsIndices.Add(SlotIndex);
			}
		}
	}

	return GroupedSlotsIndices;
}

TArray<const FNovaEquipmentSlot*> UNovaCompartmentDescription::GetGroupedEquipmentSlots(int32 Index) const
{
	TArray<const FNovaEquipmentSlot*> GroupedSlots;

	for (FName SocketName : GetGroupedEquipmentSlotsNames(Index))
	{
		for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); SlotIndex++)
		{
			const FNovaEquipmentSlot& Slot = EquipmentSlots[SlotIndex];

			if (Slot.SocketName == SocketName)
			{
				GroupedSlots.Add(&Slot);
			}
		}
	}

	return GroupedSlots;
}

TArray<FText> UNovaCompartmentDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("CompartmentDescriptionMassFormat", "<img src=\"/Text/Mass\"/> {mass} T"), TEXT("mass"),
		FText::AsNumber(FMath::RoundToInt(Mass))));

	Result.Add(FText::FormatNamed(LOCTEXT("CompartmentDescriptionModulesFormat", "<img src=\"/Text/Module\"/> {modules} module slots"),
		TEXT("modules"), FText::AsNumber(ModuleSlots.Num())));

	Result.Add(
		FText::FormatNamed(LOCTEXT("CompartmentDescriptionEquipmentsFormat", "<img src=\"/Text/Equipment\"/> {equipments} equipment slots"),
			TEXT("equipments"), FText::AsNumber(EquipmentSlots.Num())));

	return Result;
}

/*----------------------------------------------------
    Module data assets
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

TArray<FText> UNovaModuleDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(
		FText::FormatNamed(LOCTEXT("ModuleDescriptionFormat", "<img src=\"/Text/Mass\"/> {mass}T"), TEXT("mass"), FText::AsNumber(Mass)));

	return Result;
}

TArray<FText> UNovaPropellantModuleDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(
		LOCTEXT("PropellantModuleDescriptionFormat", "<img src=\"/Text/Propellant\"/> {propellant} T propellant capacity"),
		TEXT("propellant"), FText::AsNumber(PropellantMass)));

	return Result;
}

TArray<FText> UNovaCargoModuleDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("CargoModuleDescriptionFormat", "<img src=\"/Text/Cargo\"/> {cargo} T cargo capacity"),
		TEXT("cargo"), FText::AsNumber(CargoMass)));

	return Result;
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

TArray<FText> UNovaEquipmentDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(
		LOCTEXT("EquipmenteDescriptionFormat", "<img src=\"/Text/Mass\"/> {mass} T"), TEXT("mass"), FText::AsNumber(Mass)));

	return Result;
}

TArray<FText> UNovaEngineDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("EngineDescriptionFormat", "<img src=\"/Text/Thrust\"/> {thrust} kN max thrust"), TEXT("thrust"),
		FText::AsNumber(FMath::RoundToInt(Thrust))));

	return Result;
}

#undef LOCTEXT_NAMESPACE
