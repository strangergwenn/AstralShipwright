// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaModuleGroupsPanel.h"

#include "Game/NovaGameTypes.h"
#include "Game/NovaGameState.h"
#include "Player/NovaPlayerController.h"

#include "Spacecraft/NovaSpacecraft.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/System/NovaSpacecraftProcessingSystem.h"

#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaModuleGroupsPanel"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaModuleGroupsPanel::Construct(const FArguments& InArgs)
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	SNeutronModalPanel::Construct(SNeutronModalPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	SAssignNew(InternalWidget, SHorizontalBox)

	+ SHorizontalBox::Slot()

	+ SHorizontalBox::Slot()
	.AutoWidth()
	[
		SAssignNew(ModuleGroupsTable, SNeutronTable<ENovaConstants::MaxCompartmentCount + 1>)
		.Width(2 * Theme.GenericMenuWidth)
	]
	
	+ SHorizontalBox::Slot();

	// clang-format on
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaModuleGroupsPanel::OpenModuleGroupsTable(const FNovaSpacecraft& Spacecraft)
{
	// Prepare headers, table contents
	TArray<FText> Headers;
	for (int32 Index = 1; Index <= ENovaConstants::MaxCompartmentCount; Index++)
	{
		Headers.Add(FText::AsNumber(Index));
	}
	TArray<TNeutronTableValue<FText>> TableContents[ENovaConstants::MaxModuleCount];
	for (int32 RowIndex = 0; RowIndex < ENovaConstants::MaxModuleCount; RowIndex++)
	{
		for (int32 ColIndex = 0; ColIndex < ENovaConstants::MaxCompartmentCount; ColIndex++)
		{
			TableContents[RowIndex].Add(INVTEXT("-\n\n"));
		}
	}

	// Process module groups
	for (const FNovaModuleGroup& Group : Spacecraft.GetModuleGroups())
	{
		for (const FNovaModuleGroupCompartment& GroupCompartment : Group.Compartments)
		{
			for (int32 ModuleIndex : GroupCompartment.ModuleIndices)
			{
				// Build the group name
				FText GroupTypeText = FNovaSpacecraft::GetModuleGroupDescription(Group.Type);

				// Get module
				const FNovaCompartmentModule& Module     = Spacecraft.Compartments[GroupCompartment.CompartmentIndex].Modules[ModuleIndex];
				const UNovaModuleDescription* ModuleDesc = Module.Description;

				// Format text
				FText Text = FText::FormatNamed(LOCTEXT("ModulesGroupsEntryFormat", "{module}\nGroup {group}\n{type}"), TEXT("module"),
					IsValid(ModuleDesc) ? ModuleDesc->Name : FText(), TEXT("group"), FText::AsNumber(Group.Index + 1), TEXT("type"),
					GroupTypeText);

				TableContents[ModuleIndex][GroupCompartment.CompartmentIndex] = TNeutronTableValue(Text, Group.Color);
			}
		}
	}

	// Show the table
	ModuleGroupsTable->Clear();
	ModuleGroupsTable->AddHeaders(LOCTEXT("ModuleGroupsCompartment", "Compartment"), Headers);
	for (int32 Index = 0; Index < ENovaConstants::MaxModuleCount; Index++)
	{
		FText RowTitle = FText::FormatNamed(LOCTEXT("ModuleIndexFormat", "Module {index}"), TEXT("index"), FText::AsNumber(Index + 1));
		ModuleGroupsTable->AddEntries(RowTitle, TableContents[Index]);
	}

	// Show the table
	Show(LOCTEXT("ModuleGroupsTitle", "Module groups"),
		LOCTEXT("ModuleGroupsDetails",
			"Modules are grouped together when they form a line of the same type, and can then share hatches or transfer cargo for "
			"processing."),
		FSimpleDelegate());
}

void SNovaModuleGroupsPanel::OpenModuleGroup(const struct FNovaSpacecraft& Spacecraft, int32 GroupIndex)
{
	Show(FText(), FText(), FSimpleDelegate());
}

void SNovaModuleGroupsPanel::OpenModuleGroup(const FNovaSpacecraft& Spacecraft, int32 CompartmentIndex, int32 ModuleIndex)
{
	for (const FNovaModuleGroup& Group : Spacecraft.GetModuleGroups())
	{
		for (const FNovaModuleGroupCompartment& GroupCompartment : Group.Compartments)
		{
			if (GroupCompartment.CompartmentIndex == CompartmentIndex)
			{
				if (GroupCompartment.ModuleIndices.Contains(ModuleIndex))
				{
					OpenModuleGroup(Spacecraft, Group.Index);
				}
			}
		}
	}
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

#undef LOCTEXT_NAMESPACE
