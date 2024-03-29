// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftTypes.h"
#include "NovaSpacecraftPawn.h"
#include "Game/NovaGameMode.h"

#include "Neutron/Actor/NeutronCaptureActor.h"
#include "Neutron/System/NeutronAssetManager.h"

#define LOCTEXT_NAMESPACE "NovaSpacecraftTypes"

/*----------------------------------------------------
    Compartment data asset
----------------------------------------------------*/

TArray<const UNovaHullDescription*> UNovaCompartmentDescription::GetSupportedHulls() const
{
	TArray<const UNovaHullDescription*> Result;

	Result.Add(nullptr);

	Result.Append(UNeutronAssetManager::Get()->GetAssets<UNovaHullDescription>());

	return Result;
}

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

TSoftObjectPtr<UStaticMesh> UNovaCompartmentDescription::GetMainHull(const UNovaHullDescription* Hull) const
{
	if (!IsValid(Hull))
	{
		return nullptr;
	}
	else if (Hull->Type == ENovaHullType::SoftCladding)
	{
		return SoftHull;
	}
	else
	{
		return RigidHull;
	}
}

TSoftObjectPtr<UStaticMesh> UNovaCompartmentDescription::GetDomeHull(const UNovaHullDescription* Hull) const
{
	if (!IsValid(Hull))
	{
		return nullptr;
	}
	else if (Hull->Type == ENovaHullType::SoftCladding)
	{
		return DomeSoftHull;
	}
	else
	{
		return DomeRigidHull;
	}
}

TSoftObjectPtr<UStaticMesh> UNovaCompartmentDescription::GetSkirtHull(const UNovaHullDescription* Hull) const
{
	if (!IsValid(Hull))
	{
		return nullptr;
	}
	else if (Hull->Type == ENovaHullType::SoftCladding)
	{
		return SkirtSoftHull;
	}
	else
	{
		return SkirtRigidHull;
	}
}

TSoftObjectPtr<UStaticMesh> UNovaCompartmentDescription::GetBulkhead(
	const UNovaModuleDescription* ModuleDescription, ENovaBulkheadType Style, bool Forward) const
{
	if (Style == ENovaBulkheadType::Skirt && !Forward)
	{
		if (ModuleDescription && ModuleDescription->IsA<UNovaCargoModuleDescription>())
		{
			return CargoSkirt;
		}
		else
		{
			return TankSkirt;
		}
	}
	else if (ModuleDescription)
	{
		return ModuleDescription->GetBulkhead(Style, Forward);
	}
	else
	{
		return nullptr;
	}
}

FNeutronAssetPreviewSettings UNovaCompartmentDescription::GetPreviewSettings() const
{
	FNeutronAssetPreviewSettings Settings;

	Settings.Class                   = ANovaSpacecraftPawn::StaticClass();
	Settings.RequireCustomPrimitives = true;
	Settings.Rotation                = FRotator(0, 180, 0);
	Settings.RelativeXOffset         = -0.125f;
	Settings.Scale *= 0.75f;

	return Settings;
}

void UNovaCompartmentDescription::ConfigurePreviewActor(AActor* Actor) const
{
	NCHECK(Actor->GetClass() == ANovaSpacecraftPawn::StaticClass());

	TSharedPtr<FNovaSpacecraft> Spacecraft = MakeShared<FNovaSpacecraft>();
	Spacecraft->Compartments.Add(FNovaCompartment(this));

	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(Actor);
	SpacecraftPawn->SetImmediateMode(true);
	SpacecraftPawn->SetSpacecraft(Spacecraft.Get());
}

TArray<FText> UNovaCompartmentDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("CompartmentDescriptionMassFormat", "<img src=\"/Text/Mass\"/> {mass} T"), TEXT("mass"),
		FText::AsNumber(FMath::RoundToInt(Mass))));

	Result.Add(FText::FormatNamed(LOCTEXT("CompartmentDescriptionModulesFormat", "<img src=\"/Text/Module\"/> {modules} module slots"),
		TEXT("modules"), FText::AsNumber(ModuleSlots.Num())));

	Result.Add(
		FText::FormatNamed(LOCTEXT("CompartmentDescriptionEquipmentsFormat", "<img src=\"/Text/Equipment\"/> {equipment} equipment slots"),
			TEXT("equipment"), FText::AsNumber(EquipmentSlots.Num())));

	return Result;
}

/*----------------------------------------------------
    Module data assets
----------------------------------------------------*/

TSoftObjectPtr<class UStaticMesh> UNovaModuleDescription::GetBulkhead(ENovaBulkheadType Style, bool Forward) const
{
	switch (Style)
	{
		default:
		case ENovaBulkheadType::None:
		case ENovaBulkheadType::Skirt:
			return nullptr;
		case ENovaBulkheadType::Standard:
			return Forward ? ForwardBulkhead : AftBulkhead;
		case ENovaBulkheadType::Outer:
			return Forward ? OuterForwardBulkhead : OuterAftBulkhead;
	}
}

FNeutronAssetPreviewSettings UNovaModuleDescription::GetPreviewSettings() const
{
	FNeutronAssetPreviewSettings Settings;

	Settings.Class                   = ANovaSpacecraftPawn::StaticClass();
	Settings.RequireCustomPrimitives = true;

	return Settings;
}

void UNovaModuleDescription::ConfigurePreviewActor(AActor* Actor) const
{
	NCHECK(Actor->GetClass() == ANovaSpacecraftPawn::StaticClass());
	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(Actor);

	TSharedPtr<FNovaSpacecraft> Spacecraft = MakeShared<FNovaSpacecraft>();
	FNovaCompartment            Compartment(UNeutronAssetManager::Get()->GetDefaultAsset<UNovaCompartmentDescription>());
	Compartment.Modules[0].Description = this;
	Spacecraft->Compartments.Add(Compartment);

	SpacecraftPawn->SetImmediateMode(true);
	SpacecraftPawn->SetSpacecraft(Spacecraft.Get());
}

TArray<FText> UNovaModuleDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(
		FText::FormatNamed(LOCTEXT("ModuleDescriptionFormat", "<img src=\"/Text/Mass\"/> {mass} T"), TEXT("mass"), FText::AsNumber(Mass)));

	// This doesn't fit anymore, unfortunately.
	// Result.Add(FText::FormatNamed(INVTEXT("<img src=\"{icon}\"/> {name}"), TEXT("icon"),
	//	FNovaSpacecraft::GetModuleGroupIcon(FNovaSpacecraft::GetModuleType(this)), TEXT("name"),
	//	FNovaSpacecraft::GetModuleGroupDescription(FNovaSpacecraft::GetModuleType(this))));

	if (CrewEffect > 0)
	{
		Result.Add(FText::FormatNamed(
			LOCTEXT("CrewProviderFormat", "<img src=\"/Text/Crew\"/> Provides {crew} crew"), TEXT("crew"), FText::AsNumber(CrewEffect)));
	}
	else if (CrewEffect < 0)
	{
		Result.Add(FText::FormatNamed(LOCTEXT("CrewRequirementFormat", "<img src=\"/Text/Crew\"/> Requires {crew} crew"), TEXT("crew"),
			FText::AsNumber(-CrewEffect)));
	}

	return Result;
}

/*----------------------------------------------------
    Module subclasses
----------------------------------------------------*/

TArray<FText> UNovaPropellantModuleDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("PropellantModuleDescriptionFormat", "<img src=\"/Text/Propellant\"/> {propellant} T propellant"),
		TEXT("propellant"), FText::AsNumber(PropellantMass)));

	return Result;
}

TArray<FText> UNovaCargoModuleDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(
		LOCTEXT("CargoModuleDescriptionFormat", "<img src=\"/Text/Cargo\"/> {cargo} T cargo"), TEXT("cargo"), FText::AsNumber(CargoMass)));

	return Result;
}

TArray<FText> UNovaProcessingModuleDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("ProcessingModuleDescriptionFormat", "<img src=\"/Text/Cargo\"/> Produces {rate} T/s"),
		TEXT("rate"), FText::AsNumber(ProcessingRate)));

	if (Power < 0)
	{
		Result.Add(FText::FormatNamed(
			LOCTEXT("PowerProviderFormat", "<img src=\"/Text/Power\"/> Produces {power} kW"), TEXT("power"), FText::AsNumber(-Power)));
	}
	else if (Power > 0)
	{
		Result.Add(FText::FormatNamed(
			LOCTEXT("PowerRequirementFormat", "<img src=\"/Text/Power\"/> Requires {power} kW"), TEXT("power"), FText::AsNumber(Power)));
	}

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

FNeutronAssetPreviewSettings UNovaEquipmentDescription::GetPreviewSettings() const
{
	FNeutronAssetPreviewSettings Settings;

	Settings.Rotation                = FRotator(0, 180, 0);
	Settings.Class                   = ANovaSpacecraftPawn::StaticClass();
	Settings.RequireCustomPrimitives = true;

	return Settings;
}

void UNovaEquipmentDescription::ConfigurePreviewActor(AActor* Actor) const
{
	NCHECK(Actor->GetClass() == ANovaSpacecraftPawn::StaticClass());
	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(Actor);

	TSharedPtr<FNovaSpacecraft> Spacecraft = MakeShared<FNovaSpacecraft>();
	FNovaCompartment            Compartment(UNeutronAssetManager::Get()->GetDefaultAsset<UNovaCompartmentDescription>());
	Compartment.Equipment[0] = this;
	Spacecraft->Compartments.Add(Compartment);

	SpacecraftPawn->SetImmediateMode(true);
	SpacecraftPawn->SetSpacecraft(Spacecraft.Get());
}

TArray<FText> UNovaEquipmentDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(
		LOCTEXT("EquipmenteDescriptionFormat", "<img src=\"/Text/Mass\"/> {mass} T"), TEXT("mass"), FText::AsNumber(Mass)));

	if (RequiresPairing)
	{
		Result.Add(LOCTEXT("EquipmentPairing", "<img src=\"/Text/Paired\"/> Requires pairing"));
	}

	return Result;
}

/*----------------------------------------------------
    Equipment subclasses
----------------------------------------------------*/

FNeutronAssetPreviewSettings UNovaEngineDescription::GetPreviewSettings() const
{
	FNeutronAssetPreviewSettings Settings = Super::GetPreviewSettings();

	Settings.RelativeXOffset = 0.5;
	Settings.Scale *= 1.25f;

	return Settings;
}

TArray<FText> UNovaEngineDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("EngineDescriptionFormat", "<img src=\"/Text/Thrust\"/> {thrust} kN max thrust"), TEXT("thrust"),
		FText::AsNumber(FMath::RoundToInt(Thrust))));
	Result.Add(FText::FormatNamed(LOCTEXT("EngineImpulseDescriptionFormat", "<img src=\"/Text/Thrust\"/> {impulse} s specific impulse"), TEXT("impulse"),
		FText::AsNumber(FMath::RoundToInt(SpecificImpulse))));

	return Result;
}

TArray<FText> UNovaMiningEquipmentDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("MiningEquipmentDescriptionFormat", "<img src=\"/Text/Cargo\"/> Produces {rate} T/s"),
		TEXT("rate"), FText::AsNumber(ExtractionRate)));

	if (Power > 0)
	{
		Result.Add(FText::FormatNamed(
			LOCTEXT("MiningPowerFormat", "<img src=\"/Text/Power\"/> Requires {power} kW"), TEXT("power"), FText::AsNumber(Power)));
	}

	return Result;
}

TArray<FText> UNovaRadioMastDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	if (Power > 0)
	{
		Result.Add(FText::FormatNamed(
			LOCTEXT("RadioMastPowerFormat", "<img src=\"/Text/Power\"/> Requires {power} kW"), TEXT("power"), FText::AsNumber(Power)));
	}

	return Result;
}

TArray<FText> UNovaPowerEquipmentDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	if (Power > 0)
	{
		Result.Add(FText::FormatNamed(
			LOCTEXT("PowerEquipmentPowerFormat", "<img src=\"/Text/Power\"/> Produces {power} kW"), TEXT("power"), FText::AsNumber(Power)));
	}

	if (Capacity > 0)
	{
		Result.Add(FText::FormatNamed(LOCTEXT("PowerEquipmentEnergyFormat", "<img src=\"/Text/Power\"/> {capacity} kWH battery"),
			TEXT("capacity"), FText::AsNumber(Capacity)));
	}

	return Result;
}

FNeutronAssetPreviewSettings UNovaMiningEquipmentDescription::GetPreviewSettings() const
{
	FNeutronAssetPreviewSettings Settings = Super::GetPreviewSettings();

	Settings.RelativeXOffset = 0.5;
	Settings.Scale           = 0.85f;

	return Settings;
}

TArray<FText> UNovaPropellantEquipmentDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(
		FText::FormatNamed(LOCTEXT("PropellantEquipmentDescriptionFormat", "<img src=\"/Text/Propellant\"/> {propellant} T propellant"),
			TEXT("propellant"), FText::AsNumber(PropellantMass)));

	return Result;
}

#undef LOCTEXT_NAMESPACE
