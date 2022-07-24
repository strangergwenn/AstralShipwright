// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaModuleGroupsPanel.h"

#include "Game/NovaGameTypes.h"
#include "Game/NovaGameState.h"
#include "Player/NovaPlayerController.h"

#include "Spacecraft/NovaSpacecraft.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/System/NovaSpacecraftProcessingSystem.h"

#include "Neutron/UI/Widgets/NeutronFadingWidget.h"

#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaModuleGroupsPanel"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaModuleGroupsPanel::Construct(const FArguments& InArgs)
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	SNeutronModalPanel::Construct(SNeutronModalPanel::FArguments().Menu(InArgs._Menu));

	const int32 FullWidth = ENovaConstants::MaxCompartmentCount * FNeutronStyleSet::GetButtonSize("InventoryButtonSize").Width;

	// clang-format off
	SAssignNew(InternalWidget, SVerticalBox)

	+ SVerticalBox::Slot()

	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.MinDesiredWidth(FullWidth)
			.Padding(0)
			[
				SAssignNew(ProcessingChainsBox, SVerticalBox)
			]
		]

		+ SHorizontalBox::Slot()
	]

	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		SAssignNew(ModuleGroupsTable, SNeutronTable<ENovaConstants::MaxCompartmentCount + 1>)
		.Width(2 * Theme.GenericMenuWidth)
	]
	
	+ SVerticalBox::Slot();

	// clang-format on
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaModuleGroupsPanel::OpenModuleGroupsTable(const FNovaSpacecraft& Spacecraft)
{
	ProcessingChainsBox->ClearChildren();
	ModuleGroupsTable->Clear();

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

void SNovaModuleGroupsPanel::OpenModuleGroup(UNovaSpacecraftProcessingSystem* ProcessingSystem, int32 GroupIndex)
{
	ProcessingChainsBox->ClearChildren();
	ModuleGroupsTable->Clear();

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	for (const FNovaSpacecraftProcessingSystemChainState& Chain : ProcessingSystem->GetChainStates(GroupIndex))
	{
		// clang-format off
		ProcessingChainsBox->AddSlot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SHorizontalBox)
			
			// Text-based information
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SNeutronButtonLayout)
				.Size("OperationsButtonSize")
				[
					SNew(SHorizontalBox)
		
					// Production status
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SNeutronImage)
						.Image(FNeutronImageGetter::CreateLambda([=]()
						{
							switch (Chain.Status)
							{
								default:
								case ENovaSpacecraftProcessingSystemStatus::Stopped:
								case ENovaSpacecraftProcessingSystemStatus::Docked:
								case ENovaSpacecraftProcessingSystemStatus::Blocked:
									return FNeutronStyleSet::GetBrush("Icon/SB_Warning");
								case ENovaSpacecraftProcessingSystemStatus::Processing:
									return FNeutronStyleSet::GetBrush("Icon/SB_On");
							}
						}))
						.ColorAndOpacity_Lambda([=]()
						{
							switch (Chain.Status)
							{
								default:
								case ENovaSpacecraftProcessingSystemStatus::Stopped:
								case ENovaSpacecraftProcessingSystemStatus::Docked:
									return FLinearColor::White;
								case ENovaSpacecraftProcessingSystemStatus::Processing:
									return Theme.PositiveColor;
								case ENovaSpacecraftProcessingSystemStatus::Blocked:
									return Theme.NegativeColor;
							}
						})
					]
					
					// Resources breakdown
					+ SHorizontalBox::Slot()
					.Padding(Theme.ContentPadding)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text_Lambda([=]()
						{
							// Resource inputs
							FString InputList;
							for (const UNovaResource* Input : Chain.Inputs)
							{
								InputList += InputList.Len() ? TEXT(", ") : FString();
								InputList += Input->Name.ToString();
							}

							// Resource outputs
							FString OutputList;
							for (const UNovaResource* Output : Chain.Outputs)
							{
								OutputList += OutputList.Len() ? TEXT(", ") : FString();
								OutputList += Output->Name.ToString();
							}

							// Final string
							FText InputLine  = InputList.Len() ? FText::FormatNamed(LOCTEXT("ProcessingInput", "Input resources: {list}"), TEXT("list"),
																		FText::FromString(InputList))
																: FText();
							FText OutputLine = OutputList.Len() ? FText::FormatNamed(LOCTEXT("ProcessingOutput", "Output resources: {list}"), TEXT("list"),
																		FText::FromString(OutputList))
																: FText();
							return FText::FromString(InputLine.ToString() + "\n" + OutputLine.ToString());
						})
					]
				]
			]
		];
	}

	// clang-format on

	Show(FText(), FText(), FSimpleDelegate());
}

void SNovaModuleGroupsPanel::OpenModuleGroup(UNovaSpacecraftProcessingSystem* ProcessingSystem, int32 CompartmentIndex, int32 ModuleIndex)
{
	int32 ProcessingGroupIndex = ProcessingSystem->GetProcessingGroupIndex(CompartmentIndex, ModuleIndex);
	if (ProcessingGroupIndex != INDEX_NONE)
	{
		OpenModuleGroup(ProcessingSystem, ProcessingGroupIndex);
	}
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

#undef LOCTEXT_NAMESPACE
