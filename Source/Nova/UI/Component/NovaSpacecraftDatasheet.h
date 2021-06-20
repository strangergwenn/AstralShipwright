// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTable.h"

#define LOCTEXT_NAMESPACE "SNovaSpacecraftDatasheet"

/** Spacecraft datasheet class */
class SNovaSpacecraftDatasheet : public SNovaTable
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

		SNovaTable::Construct(SNovaTable::FArguments().Title(InArgs._Title).Width(500));

		// Units
		FText Tonnes           = LOCTEXT("Tonnes", "T");
		FText MetersPerSeconds = LOCTEXT("MetersPerSecond", "m/s");
		FText TonnesPerSecond  = LOCTEXT("TonnesPerSecond", "T/s");
		FText Seconds          = LOCTEXT("Seconds", "s");
		FText KiloNewtons      = LOCTEXT("KiloNewtons", "kN");

		// Build the mass table
		AddHeader(LOCTEXT("Overview", "<img src=\"/Text/Module\"/> Overview"));
		AddEntry(LOCTEXT("Name", "Name"), Target.GetName(), Comparison ? Comparison->GetName() : FText());
		AddEntry(LOCTEXT("Classification", "Classification"), Target.GetClassification(),
			Comparison ? Comparison->GetClassification() : FText());
		AddEntry(LOCTEXT("Compartments", "Compartments"), Target.Compartments.Num(), Comparison ? Comparison->Compartments.Num() : -1);

		// Build the mass table
		AddHeader(LOCTEXT("MassMetrics", "<img src=\"/Text/Mass\"/> Mass metrics"));
		AddEntry(LOCTEXT("DryMass", "Dry mass"), TargetPropulsionMetrics.DryMass,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->DryMass : -1, Tonnes);
		AddEntry(LOCTEXT("PropellantMassCapacity", "Propellant capacity"), TargetPropulsionMetrics.PropellantMassCapacity,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->PropellantMassCapacity : -1, Tonnes);
		AddEntry(LOCTEXT("CargoMassCapacity", "Cargo capacity"), TargetPropulsionMetrics.CargoMassCapacity,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->CargoMassCapacity : -1, Tonnes);
		AddEntry(LOCTEXT("MaximumMass", "Full-load mass"), TargetPropulsionMetrics.MaximumMass,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumMass : -1, Tonnes);

		// Build the propulsion table
		AddHeader(LOCTEXT("PropulsionMetrics", "<img src=\"/Text/Thrust\"/> Propulsion metrics"));
		AddEntry(LOCTEXT("SpecificImpulse", "Specific impulse"), TargetPropulsionMetrics.SpecificImpulse,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->SpecificImpulse : -1, Seconds);
		AddEntry(LOCTEXT("Thrust", "Maximum thrust"), TargetPropulsionMetrics.Thrust,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->Thrust : -1, KiloNewtons);
		AddEntry(LOCTEXT("PropellantRate", "Propellant mass rate"), TargetPropulsionMetrics.PropellantRate,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->PropellantRate : -1, TonnesPerSecond);
		AddEntry(LOCTEXT("MaximumDeltaV", "Full-load delta-v"), TargetPropulsionMetrics.MaximumDeltaV,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumDeltaV : -1, MetersPerSeconds);
		AddEntry(LOCTEXT("MaximumBurnTime", "Full-load burn time"), TargetPropulsionMetrics.MaximumBurnTime,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumBurnTime : -1, Seconds);
	}
};

#undef LOCTEXT_NAMESPACE
