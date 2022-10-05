// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronTable.h"

#define LOCTEXT_NAMESPACE "SNovaSpacecraftDatasheet"

/** Spacecraft datasheet class */
class SNovaSpacecraftDatasheet : public SNeutronTable<2>
{
	SLATE_BEGIN_ARGS(SNovaSpacecraftDatasheet) : _TargetSpacecraft(), _ComparisonSpacecraft(nullptr)
	{}

	SLATE_ARGUMENT(FText, Title)
	SLATE_ARGUMENT(FNovaSpacecraft, TargetSpacecraft)
	SLATE_ARGUMENT(const FNovaSpacecraft*, ComparisonSpacecraft)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		// Fetch the required data
		const FNovaSpacecraft&                  Target                      = InArgs._TargetSpacecraft;
		const FNovaSpacecraft*                  Comparison                  = InArgs._ComparisonSpacecraft;
		const FNovaSpacecraftPropulsionMetrics& TargetPropulsionMetrics     = Target.GetPropulsionMetrics();
		const FNovaSpacecraftPropulsionMetrics* ComparisonPropulsionMetrics = Comparison ? &Comparison->GetPropulsionMetrics() : nullptr;
		const FNovaSpacecraftPowerMetrics&      TargetPowerMetrics          = Target.GetPowerMetrics();
		const FNovaSpacecraftPowerMetrics*      ComparisonPowerMetrics      = Comparison ? &Comparison->GetPowerMetrics() : nullptr;

		SNeutronTable::Construct(SNeutronTable::FArguments().Title(InArgs._Title).Width(500));

		// Units
		FText Tonnes            = FText::FromString("T");
		FText MetersPerSeconds  = FText::FromString("m/s");
		FText MetersPerSeconds2 = FText::FromString("m/s-2");
		FText TonnesPerSecond   = FText::FromString("T/s");
		FText Seconds           = FText::FromString("s");
		FText KiloNewtons       = FText::FromString("kN");
		FText KiloWatts         = FText::FromString("kW");
		FText KiloWattHours     = FText::FromString("kWh");

		// Build the mass table
		AddHeader(LOCTEXT("Overview", "<img src=\"/Text/Module\"/> Overview"));
		AddEntry(LOCTEXT("Name", "Name"), TNeutronTableValue(Target.GetName(), Comparison ? Comparison->GetName() : FText()));
		AddEntry(LOCTEXT("Classification", "Classification"),
			TNeutronTableValue(Target.GetClassification(), Comparison ? Comparison->GetClassification() : FText()));
		AddEntry(LOCTEXT("Compartments", "Compartments"),
			TNeutronTableValue(Target.Compartments.Num(), Comparison ? Comparison->Compartments.Num() : -1, FText()));

		// Build the mass table
		AddHeader(LOCTEXT("MassMetrics", "<img src=\"/Text/Mass\"/> Mass metrics"));
		AddEntry(LOCTEXT("DryMass", "Dry mass"), TNeutronTableValue(TargetPropulsionMetrics.DryMass,
													 ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->DryMass : -1, Tonnes));
		AddEntry(LOCTEXT("PropellantMassCapacity", "Propellant capacity"),
			TNeutronTableValue(TargetPropulsionMetrics.PropellantMassCapacity,
				ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->PropellantMassCapacity : -1, Tonnes));
		AddEntry(LOCTEXT("CargoMassCapacity", "Cargo capacity"),
			TNeutronTableValue(TargetPropulsionMetrics.CargoMassCapacity,
				ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->CargoMassCapacity : -1, Tonnes));
		AddEntry(LOCTEXT("MaximumMass", "Full-load mass"),
			TNeutronTableValue(
				TargetPropulsionMetrics.MaximumMass, ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumMass : -1, Tonnes));

		// Build the propulsion table
		AddHeader(LOCTEXT("PropulsionMetrics", "<img src=\"/Text/Thrust\"/> Propulsion metrics"));
		AddEntry(LOCTEXT("SpecificImpulse", "Specific impulse"),
			TNeutronTableValue(TargetPropulsionMetrics.SpecificImpulse,
				ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->SpecificImpulse : -1, Seconds));
		AddEntry(LOCTEXT("Thrust", "Engine thrust"),
			TNeutronTableValue(TargetPropulsionMetrics.EngineThrust,
				ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->EngineThrust : -1, KiloNewtons));
		AddEntry(LOCTEXT("LinearAcceleration", "Attitude control thrust"),
			TNeutronTableValue(TargetPropulsionMetrics.ThrusterThrust,
				ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->ThrusterThrust : -1, KiloNewtons));
		AddEntry(LOCTEXT("MaximumDeltaV", "Full-load delta-v"),
			TNeutronTableValue(TargetPropulsionMetrics.MaximumDeltaV,
				ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumDeltaV : -1, MetersPerSeconds));

		AddEntry(LOCTEXT("MaximumDeltaV", "Full-load delta-v"),
			TNeutronTableValue(TargetPropulsionMetrics.MaximumDeltaV,
				ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumDeltaV : -1, MetersPerSeconds));

		// Build the power table
		AddHeader(LOCTEXT("PowerMetrics", "<img src=\"/Text/Power\"/> Power metrics"));
		AddEntry(
			LOCTEXT("PowerUsage", "Power usage"), TNeutronTableValue(TargetPowerMetrics.TotalPowerUsage,
													  ComparisonPowerMetrics ? ComparisonPowerMetrics->TotalPowerUsage : -1.0f, KiloWatts));
		AddEntry(LOCTEXT("PowerProduction", "Power production"),
			TNeutronTableValue(TargetPowerMetrics.TotalPowerProduction,
				ComparisonPowerMetrics ? ComparisonPowerMetrics->TotalPowerProduction : -1.0f, KiloWatts));
		AddEntry(LOCTEXT("EnergyStorage", "Battery capacity"),
			TNeutronTableValue(
				TargetPowerMetrics.EnergyCapacity, ComparisonPowerMetrics ? ComparisonPowerMetrics->EnergyCapacity : -1.0f, KiloWattHours));
	}
};

#undef LOCTEXT_NAMESPACE
