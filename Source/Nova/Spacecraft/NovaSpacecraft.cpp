// Nova project - Gwennaël Arbona

#include "NovaSpacecraft.h"
#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Nova.h"

#include "Dom/JsonObject.h"


/*----------------------------------------------------
	Spacecraft compartment
----------------------------------------------------*/

FNovaCompartment::FNovaCompartment()
	: Description(nullptr)
	, NeedsOuterSkirt(false)
	, NeedsMainPiping(false)
	, NeedsMainWiring(false)
	, HullType(ENovaAssemblyHullType::None)
	, Modules{ FNovaCompartmentModule() }
	, Equipments{ nullptr }
{}

FNovaCompartment::FNovaCompartment(const class UNovaCompartmentDescription* K)
	: FNovaCompartment()
{
	Description = K;
}


/*----------------------------------------------------
	Spacecraft implementation
----------------------------------------------------*/

void FNovaSpacecraft::UpdateProceduralElements()
{
	for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
	{
		FNovaCompartment& Compartment = Compartments[CompartmentIndex];

		auto IsFirstCompartment = [=]()
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

		auto IsLastCompartment = [=]()
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

				auto IsSameModuleInPreviousCompartment = [=]()
				{
					FName CurrentModuleSocketName = Compartment.Description->GetModuleSlot(ModuleIndex).SocketName;
					for (int32 Index = CompartmentIndex - 1; Index >= 0; Index--)
					{
						FNovaCompartment& PreviousAssembly = Compartments[Index];
						if (PreviousAssembly.IsValid())
						{
							return PreviousAssembly.GetModuleBySocket(CurrentModuleSocketName) == Module.Description;
						}
					}
					return false;
				};

				auto IsSameModuleInNextCompartment = [=]()
				{
					FName CurrentModuleSocketName = Compartment.Description->GetModuleSlot(ModuleIndex).SocketName;
					for (int32 Index = CompartmentIndex + 1; Index < Compartments.Num(); Index++)
					{
						FNovaCompartment& NextAssembly = Compartments[Index];
						if (NextAssembly.IsValid())
						{
							return NextAssembly.GetModuleBySocket(CurrentModuleSocketName) == Module.Description;
						}
					}
					return false;
				};

				// Reset state
				Module.ForwardBulkheadType = ENovaAssemblyBulkheadType::Standard;
				Module.AftBulkheadType = ENovaAssemblyBulkheadType::Standard;
				Module.SkirtPipingType = ENovaAssemblySkirtPipingType::None;
				Module.NeedsWiring = false;

				// Handle forced piping
				if (Compartment.Description->GetModuleSlot(ModuleIndex).ForceSkirtPiping)
				{
					Module.SkirtPipingType = ENovaAssemblySkirtPipingType::Simple;
				}

				if (Module.Description)
				{
					Module.NeedsWiring = true;
					NLOG("FNovaSpacecraft::UpdateProceduralElements : compartment %d, module %d", CompartmentIndex, ModuleIndex);

					// Define bulkheads
					if (IsFirstCompartment())
					{
						Module.ForwardBulkheadType = ENovaAssemblyBulkheadType::Outer;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> forward is Outer");
					}
					else if (IsSameModuleInPreviousCompartment())
					{
						Module.ForwardBulkheadType = ENovaAssemblyBulkheadType::Skirt;
						Module.NeedsWiring = false;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> forward is Connected");
					}

					if (IsLastCompartment())
					{
						Module.AftBulkheadType = ENovaAssemblyBulkheadType::Outer;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> aft is Outer");
					}
					else if (IsSameModuleInNextCompartment())
					{
						Module.AftBulkheadType = ENovaAssemblyBulkheadType::Skirt;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> aft is Connected");
					}

					// Define piping
					if (Module.Description->NeedsPiping && !IsSameModuleInNextCompartment())
					{
						Module.SkirtPipingType = ENovaAssemblySkirtPipingType::Connection;
						NLOG("FNovaSpacecraft::UpdateProceduralElements : -> skirt piping is Connection");
					}
					else
					{
						Module.SkirtPipingType = ENovaAssemblySkirtPipingType::Simple;
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

		TArray<TSharedPtr<FJsonValue>> SavedCompartments;
		for (const FNovaCompartment& CompartmentAssembly : This->Compartments)
		{
			TSharedPtr<FJsonObject> CompartmentJsonData = MakeShareable(new FJsonObject);

			auto SaveAsset = [=](TSharedPtr<FJsonObject> Save, FString Name, const UNovaAssetDescription* Asset)
			{
				Save->SetStringField(Name, Asset ? Asset->Identifier.ToString() : FGuid().ToString());
			};

			if (CompartmentAssembly.Description)
			{
				SaveAsset(CompartmentJsonData, "Description", CompartmentAssembly.Description);
				CompartmentJsonData->SetNumberField("HullType", static_cast<uint8>(CompartmentAssembly.HullType));
				for (int32 Index = 0; Index < ENovaConstants::MaxModuleCount; Index++)
				{
					SaveAsset(CompartmentJsonData, FString("ModuleDescription") + FString::FromInt(Index), CompartmentAssembly.Modules[Index].Description);
					CompartmentJsonData->SetNumberField(FString("ForwardBulkheadType") + FString::FromInt(Index), static_cast<uint8>(CompartmentAssembly.Modules[Index].ForwardBulkheadType));
					CompartmentJsonData->SetNumberField(FString("AftBulkheadType") + FString::FromInt(Index), static_cast<uint8>(CompartmentAssembly.Modules[Index].AftBulkheadType));
				}
				for (int32 Index = 0; Index < ENovaConstants::MaxEquipmentCount; Index++)
				{
					SaveAsset(CompartmentJsonData, FString("Equipment") + FString::FromInt(Index), CompartmentAssembly.Equipments[Index]);
				}

				SavedCompartments.Add(MakeShareable(new FJsonValueObject(CompartmentJsonData)));
			}
		}

		JsonData->SetArrayField("Compartments", SavedCompartments);
	}
	else
	{
		This = MakeShareable(new FNovaSpacecraft);

		const TArray<TSharedPtr<FJsonValue>>* SavedCompartments;
		if (JsonData->TryGetArrayField("Compartments", SavedCompartments))
		{
			for (TSharedPtr<FJsonValue> CompartmentObject : *SavedCompartments)
			{
				FNovaCompartment CompartmentAssembly;
				TSharedPtr<FJsonObject> CompartmentJsonData = CompartmentObject->AsObject();

				auto LoadAsset = [=](TSharedPtr<FJsonObject> Save, FString Name)
				{
					const UNovaAssetDescription* Asset = nullptr;

					FGuid Identifier;
					if (FGuid::Parse(Save->GetStringField(Name), Identifier))
					{
						Asset = UNovaAssetCatalog::Get()->GetAsset(Identifier);
					}

					return Asset;
				};

				CompartmentAssembly.Description = Cast<UNovaCompartmentDescription>(LoadAsset(CompartmentJsonData, "Description"));
				NCHECK(CompartmentAssembly.Description);
				CompartmentAssembly.HullType = static_cast<ENovaAssemblyHullType>(CompartmentJsonData->GetNumberField("HullType"));

				for (int32 Index = 0; Index < ENovaConstants::MaxModuleCount; Index++)
				{
					CompartmentAssembly.Modules[Index].Description
						= Cast<UNovaModuleDescription>(LoadAsset(CompartmentJsonData, FString("ModuleDescription") + FString::FromInt(Index)));
					CompartmentAssembly.Modules[Index].ForwardBulkheadType
						= static_cast<ENovaAssemblyBulkheadType>(CompartmentJsonData->GetNumberField(FString("ForwardBulkheadType") + FString::FromInt(Index)));
					CompartmentAssembly.Modules[Index].AftBulkheadType
						= static_cast<ENovaAssemblyBulkheadType>(CompartmentJsonData->GetNumberField(FString("AftBulkheadType") + FString::FromInt(Index)));
				}
				for (int32 Index = 0; Index < ENovaConstants::MaxEquipmentCount; Index++)
				{
					CompartmentAssembly.Equipments[Index] = Cast<UNovaEquipmentDescription>(LoadAsset(CompartmentJsonData, FString("Equipment") + FString::FromInt(Index)));
				}

				This->Compartments.Add(CompartmentAssembly);
			}
		}
	}
}
