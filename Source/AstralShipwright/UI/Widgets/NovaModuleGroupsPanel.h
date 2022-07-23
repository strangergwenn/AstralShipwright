// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Game/NovaGameTypes.h"

#include "Neutron/UI/Widgets/NeutronInfoText.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"
#include "Neutron/UI/Widgets/NeutronTable.h"

/** Module groups inspection panel */
class SNovaModuleGroupsPanel : public SNeutronModalPanel
{
	/*----------------------------------------------------
	    Slate args
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaModuleGroupsPanel)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(FNeutronAsyncCondition, IsConfirmEnabled)

	SLATE_END_ARGS()

public:

	SNovaModuleGroupsPanel()
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

public:

	/** Open the module groups table */
	void OpenModuleGroupsTable(const struct FNovaSpacecraft& Spacecraft);

	/** Open the modal panel with a view into a specific module group identified by a group index */
	void OpenModuleGroup(const struct FNovaSpacecraft& Spacecraft, int32 GroupIndex);

	/** Open the modal panel with a view into a specific module group identified by one module */
	void OpenModuleGroup(const struct FNovaSpacecraft& Spacecraft, int32 CompartmentIndex, int32 ModuleIndex);

protected:

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Module groups table
	TSharedPtr<SNeutronTable<ENovaConstants::MaxCompartmentCount + 1>> ModuleGroupsTable;
};
