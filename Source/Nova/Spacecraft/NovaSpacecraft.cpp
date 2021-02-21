// Nova project - Gwennaël Arbona

#include "NovaSpacecraft.h"
#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Nova.h"

#include "Dom/JsonObject.h"


/*----------------------------------------------------
	Spacecraft module
----------------------------------------------------*/

FNovaCompartmentModule::FNovaCompartmentModule()
	: Description(nullptr)
	, ForwardBulkheadType(ENovaBulkheadType::None)
	, AftBulkheadType(ENovaBulkheadType::None)
	, SkirtPipingType(ENovaSkirtPipingType::None)
	, NeedsWiring(false)
{}

bool FNovaCompartmentModule::operator==(const FNovaCompartmentModule& Other) const
{
	return Description == Other.Description;
}


/*----------------------------------------------------
	Spacecraft compartment
----------------------------------------------------*/

FNovaCompartment::FNovaCompartment()
	: Description(nullptr)
	, NeedsOuterSkirt(false)
	, NeedsMainPiping(false)
	, NeedsMainWiring(false)
	, HullType(ENovaHullType::None)
	, Modules{ FNovaCompartmentModule() }
	, Equipments{ nullptr }
{}

FNovaCompartment::FNovaCompartment(const class UNovaCompartmentDescription* K)
	: FNovaCompartment()
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

/*----------------------------------------------------
	Spacecraft implementation
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

			// TODO : main piping & wiring is probably always desired
			Compartment.NeedsMainPiping = true;
			Compartment.NeedsMainWiring = true;

			// Process modules
			for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
			{
				FNovaCompartmentModule& Module = Compartment.Modules[ModuleIndex];

				auto IsSameModuleInPreviousCompartment = [&]()
				{
					FName CurrentModuleSocketName = Compartment.Description->GetModuleSlot(ModuleIndex).SocketName;
					for (int32 Index = CompartmentIndex - 1; Index >= 0; Index--)
					{
						FNovaCompartment& PreviousCompartment = Compartments[Index];
						if (PreviousCompartment.IsValid())
						{
							return PreviousCompartment.GetModuleBySocket(CurrentModuleSocketName) == Module.Description;
						}
					}
					return false;
				};

				auto IsSameModuleInNextCompartment = [&]()
				{
					FName CurrentModuleSocketName = Compartment.Description->GetModuleSlot(ModuleIndex).SocketName;
					for (int32 Index = CompartmentIndex + 1; Index < Compartments.Num(); Index++)
					{
						FNovaCompartment& NextCompartment = Compartments[Index];
						if (NextCompartment.IsValid())
						{
							return NextCompartment.GetModuleBySocket(CurrentModuleSocketName) == Module.Description;
						}
					}
					return false;
				};

				// Reset state
				Module.ForwardBulkheadType = ENovaBulkheadType::Standard;
				Module.AftBulkheadType = ENovaBulkheadType::Standard;
				Module.SkirtPipingType = ENovaSkirtPipingType::None;
				Module.NeedsWiring = false;

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
					else if (IsSameModuleInPreviousCompartment())
					{
						Module.ForwardBulkheadType = ENovaBulkheadType::Skirt;
						Module.NeedsWiring = false;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> forward is Connected");
					}

					if (IsLastCompartment())
					{
						Module.AftBulkheadType = ENovaBulkheadType::Outer;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> aft is Outer");
					}
					else if (IsSameModuleInNextCompartment())
					{
						Module.AftBulkheadType = ENovaBulkheadType::Skirt;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> aft is Connected");
					}

					// Define piping
					if (Module.Description->NeedsPiping && !IsSameModuleInNextCompartment())
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

FNovaSpacecraft FNovaSpacecraft::GetSafeCopy() const
{
	FNovaSpacecraft NewSpacecraft = *this;

	NewSpacecraft.Compartments.Empty();
	for (const FNovaCompartment& Compartment : Compartments)
	{
		if (Compartment.Description != nullptr)
		{
			NewSpacecraft.Compartments.Add(Compartment);
		}
	}

	return NewSpacecraft;
}

TSharedPtr<FNovaSpacecraft> FNovaSpacecraft::GetSharedCopy() const
{
	TSharedPtr<FNovaSpacecraft> NewSpacecraft = MakeShareable(new FNovaSpacecraft);
	*NewSpacecraft = *this;
	return NewSpacecraft;
}

void FNovaSpacecraft::SerializeJson(TSharedPtr<FNovaSpacecraft>& This, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShareable(new FJsonObject);

		JsonData->SetStringField("Identifier", This->Identifier.ToString());

		TArray<TSharedPtr<FJsonValue>> SavedCompartments;
		for (const FNovaCompartment& Compartment : This->Compartments)
		{
			TSharedPtr<FJsonObject> CompartmentJsonData = MakeShareable(new FJsonObject);

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
					SaveAsset(CompartmentJsonData, FString("ModuleDescription") + FString::FromInt(Index), Compartment.Modules[Index].Description);
					CompartmentJsonData->SetNumberField(FString("ForwardBulkheadType") + FString::FromInt(Index), static_cast<uint8>(Compartment.Modules[Index].ForwardBulkheadType));
					CompartmentJsonData->SetNumberField(FString("AftBulkheadType") + FString::FromInt(Index), static_cast<uint8>(Compartment.Modules[Index].AftBulkheadType));
				}
				for (int32 Index = 0; Index < ENovaConstants::MaxEquipmentCount; Index++)
				{
					SaveAsset(CompartmentJsonData, FString("Equipment") + FString::FromInt(Index), Compartment.Equipments[Index]);
				}

				SavedCompartments.Add(MakeShareable(new FJsonValueObject(CompartmentJsonData)));
			}
		}

		JsonData->SetArrayField("Compartments", SavedCompartments);
	}
	else
	{
		This = MakeShareable(new FNovaSpacecraft);

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
				FNovaCompartment Compartment;
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
					Compartment.Modules[Index].Description
						= Cast<UNovaModuleDescription>(LoadAsset(CompartmentJsonData, FString("ModuleDescription") + FString::FromInt(Index)));
					Compartment.Modules[Index].ForwardBulkheadType
						= static_cast<ENovaBulkheadType>(CompartmentJsonData->GetNumberField(FString("ForwardBulkheadType") + FString::FromInt(Index)));
					Compartment.Modules[Index].AftBulkheadType
						= static_cast<ENovaBulkheadType>(CompartmentJsonData->GetNumberField(FString("AftBulkheadType") + FString::FromInt(Index)));
				}
				for (int32 Index = 0; Index < ENovaConstants::MaxEquipmentCount; Index++)
				{
					Compartment.Equipments[Index] = Cast<UNovaEquipmentDescription>(LoadAsset(CompartmentJsonData, FString("Equipment") + FString::FromInt(Index)));
				}

				This->Compartments.Add(Compartment);
			}
		}
	}
}
