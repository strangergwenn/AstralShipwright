// Nova project - Gwennaël Arbona

#include "NovaSpacecraft.h"
#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Nova.h"

#include "Dom/JsonObject.h"

/*----------------------------------------------------
    Spacecraft compartment
----------------------------------------------------*/

FNovaCompartmentModule::FNovaCompartmentModule()
	: Description(nullptr)
	, ForwardBulkheadType(ENovaBulkheadType::None)
	, AftBulkheadType(ENovaBulkheadType::None)
	, SkirtPipingType(ENovaSkirtPipingType::None)
	, NeedsWiring(false)
{}

FNovaCompartment::FNovaCompartment()
	: Description(nullptr)
	, NeedsOuterSkirt(false)
	, NeedsMainPiping(false)
	, NeedsMainWiring(false)
	, HullType(ENovaHullType::None)
	, Modules{FNovaCompartmentModule()}
	, Equipments{nullptr}
{}

FNovaCompartment::FNovaCompartment(const class UNovaCompartmentDescription* K) : FNovaCompartment()
{
	Description = K;
}

bool FNovaCompartment::operator==(const FNovaCompartment& Other) const
{
	if (Description != Other.Description || HullType != Other.HullType)
	{
		return false;
	}
	else
	{
		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			if (Modules[ModuleIndex] != Other.Modules[ModuleIndex])
			{
				return false;
			}
		}
		for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
		{
			if (Equipments[EquipmentIndex] != Other.Equipments[EquipmentIndex])
			{
				return false;
			}
		}

		return true;
	}
}

const UNovaModuleDescription* FNovaCompartment::GetModuleBySocket(FName SocketName) const
{
	if (Description)
	{
		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			if (Description->GetModuleSlot(ModuleIndex).SocketName == SocketName)
			{
				return Modules[ModuleIndex].Description;
			}
		}
	}
	return nullptr;
}

const UNovaEquipmentDescription* FNovaCompartment::GetEquipmentySocket(FName SocketName) const
{
	if (Description)
	{
		for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
		{
			if (Description->GetEquipmentSlot(EquipmentIndex).SocketName == SocketName)
			{
				return Equipments[EquipmentIndex];
			}
		}
	}
	return nullptr;
}

/*----------------------------------------------------
    Spacecraft constructor & operators
----------------------------------------------------*/

bool FNovaSpacecraft::operator==(const FNovaSpacecraft& Other) const
{
	if (Identifier != Other.Identifier)
	{
		return false;
	}
	else if (Compartments.Num() != Other.Compartments.Num())
	{
		return false;
	}
	else
	{
		for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
		{
			if (Compartments[CompartmentIndex] != Other.Compartments[CompartmentIndex])
			{
				return false;
			}
		}

		return true;
	}
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void FNovaSpacecraft::UpdateProceduralElements()
{
	for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
	{
		FNovaCompartment& Compartment = Compartments[CompartmentIndex];

		auto IsFirstCompartment = [&]()
		{
			for (int32 Index = 0; Index < CompartmentIndex; Index++)
			{
				if (Compartments[Index].IsValid())
				{
					return false;
				}
			}
			return true;
		};

		auto IsLastCompartment = [&]()
		{
			for (int32 Index = CompartmentIndex + 1; Index < Compartments.Num(); Index++)
			{
				if (Compartments[Index].IsValid())
				{
					return false;
				}
			}
			return true;
		};

		if (Compartment.IsValid())
		{
			// TODO : outer skirt would be used for compartment that have side modules, when the following (aft) compartment does not
			Compartment.NeedsOuterSkirt = false;

			// TODO : review whether main piping & wiring can ever be undesired - maybe on extremities with no module ?
			Compartment.NeedsMainPiping = true;
			Compartment.NeedsMainWiring = true;

			// Process modules
			for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
			{
				FNovaCompartmentModule& Module = Compartment.Modules[ModuleIndex];

				// Reset state
				Module.ForwardBulkheadType = ENovaBulkheadType::Standard;
				Module.AftBulkheadType     = ENovaBulkheadType::Standard;
				Module.SkirtPipingType     = ENovaSkirtPipingType::None;
				Module.NeedsWiring         = false;

				// Handle forced piping
				if (Compartment.Description->GetModuleSlot(ModuleIndex).ForceSkirtPiping)
				{
					Module.SkirtPipingType = ENovaSkirtPipingType::Simple;
				}

				if (Module.Description)
				{
					Module.NeedsWiring = true;
					NLOG("FNovaSpacecraft::UpdateProceduralElements : compartment %d, module %d", CompartmentIndex, ModuleIndex);

					// Define bulkheads
					if (IsFirstCompartment())
					{
						Module.ForwardBulkheadType = ENovaBulkheadType::Outer;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> forward is Outer");
					}
					else if (IsSameModuleInPreviousCompartment(CompartmentIndex, ModuleIndex))
					{
						Module.ForwardBulkheadType = ENovaBulkheadType::Skirt;
						Module.NeedsWiring         = false;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> forward is Connected");
					}

					if (IsLastCompartment())
					{
						Module.AftBulkheadType = ENovaBulkheadType::Outer;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> aft is Outer");
					}
					else if (IsSameModuleInNextCompartment(CompartmentIndex, ModuleIndex))
					{
						Module.AftBulkheadType = ENovaBulkheadType::Skirt;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> aft is Connected");
					}

					// Define piping
					if (Module.Description->NeedsPiping && !IsSameModuleInNextCompartment(CompartmentIndex, ModuleIndex))
					{
						Module.SkirtPipingType = ENovaSkirtPipingType::Connection;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> skirt piping is Connection");
					}
					else
					{
						Module.SkirtPipingType = ENovaSkirtPipingType::Simple;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> skirt piping is Simple");
					}
				}
			}
		}
	}
}

void FNovaSpacecraft::UpdatePropulsionMetrics()
{
	PropulsionMetrics = FNovaSpacecraftPropulsionMetrics();

	// Constants
	constexpr float StandardGravity     = 9.807f;
	constexpr float FuelSkirtMultiplier = 1.1f;

	float TotalEngineISPTimesThrust = 0;

	// Iterate over compartments
	for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
	{
		FNovaCompartment& Compartment = Compartments[CompartmentIndex];
		if (Compartment.IsValid())
		{
			PropulsionMetrics.DryMass += Compartment.Description->Mass;

			// Iterate over modules
			for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
			{
				FNovaCompartmentModule& Module = Compartment.Modules[ModuleIndex];
				if (Module.Description)
				{
					PropulsionMetrics.DryMass += Module.Description->Mass;

					// Handle fuel modules
					const UNovaPropellantModuleDescription* FuelModule = Cast<UNovaPropellantModuleDescription>(Module.Description);
					if (FuelModule)
					{
						float FuelMass = FuelModule->FuelMass;

						if (IsSameModuleInPreviousCompartment(CompartmentIndex, ModuleIndex))
						{
							FuelMass *= FuelSkirtMultiplier;
						}

						PropulsionMetrics.FuelMass += FuelMass;
					}

					// Handle cargo modules
					const UNovaCargoModuleDescription* CargoModule = Cast<UNovaCargoModuleDescription>(Module.Description);
					if (CargoModule)
					{
						PropulsionMetrics.CargoMass += CargoModule->CargoMass;
					}
				}
			}

			// Iterate over equipments
			for (const UNovaEquipmentDescription* Equipment : Compartment.Equipments)
			{
				if (Equipment)
				{
					PropulsionMetrics.DryMass += Equipment->Mass;

					// Handle engine equipments
					const UNovaEngineDescription* Engine = Cast<UNovaEngineDescription>(Equipment);
					if (Engine)
					{
						PropulsionMetrics.Thrust += Engine->Thrust;
						TotalEngineISPTimesThrust += Engine->SpecificImpulse * Engine->Thrust;
					}
				}
			}
		}
	}

	// Compute metrics
	PropulsionMetrics.TotalMass = PropulsionMetrics.DryMass + PropulsionMetrics.FuelMass + PropulsionMetrics.CargoMass;
	if (PropulsionMetrics.Thrust > 0)
	{
		PropulsionMetrics.SpecificImpulse = TotalEngineISPTimesThrust / PropulsionMetrics.Thrust;
		PropulsionMetrics.ExhaustVelocity = StandardGravity * PropulsionMetrics.SpecificImpulse;
		PropulsionMetrics.FuelRate        = PropulsionMetrics.Thrust / PropulsionMetrics.ExhaustVelocity;
		PropulsionMetrics.TotalDeltaV = PropulsionMetrics.ExhaustVelocity * log((PropulsionMetrics.TotalMass) / PropulsionMetrics.DryMass);
		PropulsionMetrics.TotalBurnTime = PropulsionMetrics.FuelMass / PropulsionMetrics.FuelRate;
	}

#if 1
	NLOG("--------------------------------------------------------------------------------");
	NLOG("Mass specifications : dry %.1fT, fuel %.1fT, cargo %.1fT, total %.1fT", PropulsionMetrics.DryMass, PropulsionMetrics.FuelMass,
		PropulsionMetrics.CargoMass, PropulsionMetrics.TotalMass);
	NLOG("Engine specifications : thrust %.1fkN, ISP %.1fm/s, EV %.1fm/s", PropulsionMetrics.Thrust, PropulsionMetrics.SpecificImpulse,
		PropulsionMetrics.ExhaustVelocity);
	NLOG("Delta-V specifications : total %.1fm/s, burn time %.1fs", PropulsionMetrics.TotalDeltaV, PropulsionMetrics.TotalBurnTime);
	NLOG("--------------------------------------------------------------------------------");
#endif
}

void FNovaSpacecraft::SerializeJson(TSharedPtr<FNovaSpacecraft>& This, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	// Writing to JSON
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShared<FJsonObject>();

		JsonData->SetStringField("Identifier", This->Identifier.ToString());

		TArray<TSharedPtr<FJsonValue>> SavedCompartments;
		for (const FNovaCompartment& Compartment : This->Compartments)
		{
			TSharedPtr<FJsonObject> CompartmentJsonData = MakeShared<FJsonObject>();

			auto SaveAsset = [](TSharedPtr<FJsonObject> Save, FString Name, const UNovaAssetDescription* Asset)
			{
				Save->SetStringField(Name, Asset ? Asset->Identifier.ToString() : FGuid().ToString());
			};

			if (Compartment.Description)
			{
				SaveAsset(CompartmentJsonData, "Description", Compartment.Description);
				CompartmentJsonData->SetNumberField("HullType", static_cast<uint8>(Compartment.HullType));
				for (int32 Index = 0; Index < ENovaConstants::MaxModuleCount; Index++)
				{
					SaveAsset(CompartmentJsonData, FString("ModuleDescription") + FString::FromInt(Index),
						Compartment.Modules[Index].Description);
					CompartmentJsonData->SetNumberField(FString("ForwardBulkheadType") + FString::FromInt(Index),
						static_cast<uint8>(Compartment.Modules[Index].ForwardBulkheadType));
					CompartmentJsonData->SetNumberField(FString("AftBulkheadType") + FString::FromInt(Index),
						static_cast<uint8>(Compartment.Modules[Index].AftBulkheadType));
				}
				for (int32 Index = 0; Index < ENovaConstants::MaxEquipmentCount; Index++)
				{
					SaveAsset(CompartmentJsonData, FString("Equipment") + FString::FromInt(Index), Compartment.Equipments[Index]);
				}

				SavedCompartments.Add(MakeShared<FJsonValueObject>(CompartmentJsonData));
			}
		}

		JsonData->SetArrayField("Compartments", SavedCompartments);
	}

	// Reading from JSON
	else
	{
		This = MakeShared<FNovaSpacecraft>();
		This->Create();

		FGuid Identifier;
		if (FGuid::Parse(JsonData->GetStringField("Identifier"), Identifier))
		{
			This->Identifier = Identifier;
		}

		const TArray<TSharedPtr<FJsonValue>>* SavedCompartments;
		if (JsonData->TryGetArrayField("Compartments", SavedCompartments))
		{
			for (TSharedPtr<FJsonValue> CompartmentObject : *SavedCompartments)
			{
				FNovaCompartment        Compartment;
				TSharedPtr<FJsonObject> CompartmentJsonData = CompartmentObject->AsObject();

				auto LoadAsset = [](TSharedPtr<FJsonObject> Save, FString Name)
				{
					const UNovaAssetDescription* Asset = nullptr;

					FGuid Identifier;
					if (FGuid::Parse(Save->GetStringField(Name), Identifier))
					{
						Asset = UNovaAssetCatalog::Get()->GetAsset(Identifier);
					}

					return Asset;
				};

				Compartment.Description = Cast<UNovaCompartmentDescription>(LoadAsset(CompartmentJsonData, "Description"));
				NCHECK(Compartment.Description);
				Compartment.HullType = static_cast<ENovaHullType>(CompartmentJsonData->GetNumberField("HullType"));

				for (int32 Index = 0; Index < ENovaConstants::MaxModuleCount; Index++)
				{
					Compartment.Modules[Index].Description = Cast<UNovaModuleDescription>(
						LoadAsset(CompartmentJsonData, FString("ModuleDescription") + FString::FromInt(Index)));
					Compartment.Modules[Index].ForwardBulkheadType = static_cast<ENovaBulkheadType>(
						CompartmentJsonData->GetNumberField(FString("ForwardBulkheadType") + FString::FromInt(Index)));
					Compartment.Modules[Index].AftBulkheadType = static_cast<ENovaBulkheadType>(
						CompartmentJsonData->GetNumberField(FString("AftBulkheadType") + FString::FromInt(Index)));
				}
				for (int32 Index = 0; Index < ENovaConstants::MaxEquipmentCount; Index++)
				{
					Compartment.Equipments[Index] =
						Cast<UNovaEquipmentDescription>(LoadAsset(CompartmentJsonData, FString("Equipment") + FString::FromInt(Index)));
				}

				This->Compartments.Add(Compartment);
			}
		}

		This->UpdateProceduralElements();
		This->UpdatePropulsionMetrics();
	}
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

bool FNovaSpacecraft::IsSameModuleInPreviousCompartment(int32 CompartmentIndex, int32 ModuleIndex) const
{
	const FNovaCompartment&       Compartment             = Compartments[CompartmentIndex];
	const FNovaCompartmentModule& Module                  = Compartment.Modules[ModuleIndex];
	FName                         CurrentModuleSocketName = Compartment.Description->GetModuleSlot(ModuleIndex).SocketName;

	for (int32 Index = CompartmentIndex - 1; Index >= 0; Index--)
	{
		const FNovaCompartment& PreviousCompartment = Compartments[Index];
		if (PreviousCompartment.IsValid())
		{
			return PreviousCompartment.GetModuleBySocket(CurrentModuleSocketName) == Module.Description;
		}
	}

	return false;
};

bool FNovaSpacecraft::IsSameModuleInNextCompartment(int32 CompartmentIndex, int32 ModuleIndex) const
{
	const FNovaCompartment&       Compartment             = Compartments[CompartmentIndex];
	const FNovaCompartmentModule& Module                  = Compartment.Modules[ModuleIndex];
	FName                         CurrentModuleSocketName = Compartment.Description->GetModuleSlot(ModuleIndex).SocketName;

	for (int32 Index = CompartmentIndex + 1; Index < Compartments.Num(); Index++)
	{
		const FNovaCompartment& NextCompartment = Compartments[Index];
		if (NextCompartment.IsValid())
		{
			return NextCompartment.GetModuleBySocket(CurrentModuleSocketName) == Module.Description;
		}
	}

	return false;
};
