// Astral Shipwright - Gwennaël Arbona

#include "NovaContract.h"
#include "Player/NovaPlayerController.h"
#include "Nova.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronGameInstance.h"

#define LOCTEXT_NAMESPACE "FNovaContract"

/*----------------------------------------------------
    Tutorial contract
----------------------------------------------------*/

FNovaTutorialContract::FNovaTutorialContract()
{
	Type = ENeutronContractType::Tutorial;
}

void FNovaTutorialContract::Initialize(UNeutronGameInstance* CurrentGameInstance)
{
	FNeutronContract::Initialize(CurrentGameInstance);

	// TODO : actually implement contracts

	// Build the description texts
	Details.Title       = LOCTEXT("TutorialContract", "Tutorial contract");
	Details.Description = FText::FormatNamed(LOCTEXT("TutorialContractDescription", "Buy {quantity} containers of {resource}"),
		TEXT("quantity"), FText::AsNumber(2), TEXT("resource"), LOCTEXT("Test", "Test"));
}

TSharedRef<FJsonObject> FNovaTutorialContract::Save() const
{
	TSharedRef<FJsonObject> Data = FNeutronContract::Save();

	return Data;
}

void FNovaTutorialContract::Load(const TSharedPtr<FJsonObject>& Data)
{
	FNeutronContract::Load(Data);

	Details.Progress = 0.23f;
}

void FNovaTutorialContract::OnEvent(const FNeutronContractEvent& Event)
{
	if (Event.Type == Tick)
	{
		Details.Progress = 0.23f;

		/*if (false)
		{
		    UNovaContractManager::Get()->ProgressContract(SharedThis(this));
		}
		else
		{
		    UNovaContractManager::Get()->CompleteContract(SharedThis(this));
		}*/
	}
}

#undef LOCTEXT_NAMESPACE
