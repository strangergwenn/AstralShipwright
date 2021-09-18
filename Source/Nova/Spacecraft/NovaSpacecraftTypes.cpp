// Nova project - Gwennaël Arbona

#include "NovaSpacecraftTypes.h"
#include "NovaSpacecraftPawn.h"
#include "Nova/Actor/NovaCaptureActor.h"
#include "Nova/Game/NovaGameMode.h"
#include "Nova/System/NovaAssetManager.h"

#define LOCTEXT_NAMESPACE "NovaSpacecraftTypes"

/*----------------------------------------------------
    Compartment data asset
----------------------------------------------------*/

TArray<const UNovaHullDescription*> UNovaCompartmentDescription::GetSupportedHulls() const
{
	TArray<const UNovaHullDescription*> Result;

	Result.Add(nullptr);

	Result.Append(UNovaAssetManager::Get()->GetAssets<UNovaHullDescription>());

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

TSoftObjectPtr<UStaticMesh> UNovaCompartmentDescription::GetOuterHull(const UNovaHullDescription* Hull) const
{
	return IsValid(Hull) ? OuterHull : nullptr;
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

FNovaAssetPreviewSettings UNovaCompartmentDescription::GetPreviewSettings() const
{
	FNovaAssetPreviewSettings Settings;

	Settings.Class                   = ANovaSpacecraftPawn::StaticClass();
	Settings.RequireCustomPrimitives = true;
	Settings.UsePowerfulLight        = true;
	Settings.Scale *= 0.9f;

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
	SpacecraftPawn->UpdateAssembly();
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

FNovaAssetPreviewSettings UNovaModuleDescription::GetPreviewSettings() const
{
	FNovaAssetPreviewSettings Settings;

	Settings.Class                   = ANovaSpacecraftPawn::StaticClass();
	Settings.RequireCustomPrimitives = true;
	Settings.UsePowerfulLight        = true;

	return Settings;
}

void UNovaModuleDescription::ConfigurePreviewActor(AActor* Actor) const
{
	NCHECK(Actor->GetClass() == ANovaSpacecraftPawn::StaticClass());
	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(Actor);

	TSharedPtr<FNovaSpacecraft> Spacecraft = MakeShared<FNovaSpacecraft>();
	FNovaCompartment            Compartment(SpacecraftPawn->EmptyCompartmentDescription);
	Compartment.Modules[0].Description = this;
	Spacecraft->Compartments.Add(Compartment);

	SpacecraftPawn->SetImmediateMode(true);
	SpacecraftPawn->SetSpacecraft(Spacecraft.Get());
	SpacecraftPawn->UpdateAssembly();
}

TArray<FText> UNovaModuleDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(
		FText::FormatNamed(LOCTEXT("ModuleDescriptionFormat", "<img src=\"/Text/Mass\"/> {mass} T"), TEXT("mass"), FText::AsNumber(Mass)));

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

FNovaAssetPreviewSettings UNovaEquipmentDescription::GetPreviewSettings() const
{
	FNovaAssetPreviewSettings Settings;

	Settings.Class                   = ANovaSpacecraftPawn::StaticClass();
	Settings.RequireCustomPrimitives = true;
	Settings.UsePowerfulLight        = true;

	return Settings;
}

void UNovaEquipmentDescription::ConfigurePreviewActor(AActor* Actor) const
{
	NCHECK(Actor->GetClass() == ANovaSpacecraftPawn::StaticClass());
	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(Actor);

	TSharedPtr<FNovaSpacecraft> Spacecraft = MakeShared<FNovaSpacecraft>();
	FNovaCompartment            Compartment(SpacecraftPawn->EmptyCompartmentDescription);
	Compartment.Equipment[0] = this;
	Spacecraft->Compartments.Add(Compartment);

	SpacecraftPawn->SetImmediateMode(true);
	SpacecraftPawn->SetSpacecraft(Spacecraft.Get());
	SpacecraftPawn->UpdateAssembly();
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

FNovaAssetPreviewSettings UNovaEngineDescription::GetPreviewSettings() const
{
	FNovaAssetPreviewSettings Settings = Super::GetPreviewSettings();

	Settings.Scale *= 1.25f;

	return Settings;
}

TArray<FText> UNovaEngineDescription::GetDescription() const
{
	TArray<FText> Result = Super::GetDescription();

	Result.Add(FText::FormatNamed(LOCTEXT("EngineDescriptionFormat", "<img src=\"/Text/Thrust\"/> {thrust} kN max thrust"), TEXT("thrust"),
		FText::AsNumber(FMath::RoundToInt(Thrust))));

	return Result;
}

#undef LOCTEXT_NAMESPACE
