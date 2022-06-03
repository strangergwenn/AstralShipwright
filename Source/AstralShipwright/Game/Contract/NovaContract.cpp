// Astral Shipwright - Gwennaël Arbona

#include "NovaContract.h"

#include "Player/NovaPlayerController.h"
#include "System/NovaGameInstance.h"
#include "System/NovaAssetManager.h"

#include "Nova.h"

#define LOCTEXT_NAMESPACE "FNovaContract"

/*----------------------------------------------------
    Tutorial contract
----------------------------------------------------*/

FNovaTutorialContract::FNovaTutorialContract()
{
	Type = ENovaContractType::Tutorial;
}

void FNovaTutorialContract::Initialize(UNovaGameInstance* CurrentGameInstance)
{
	FNovaContract::Initialize(CurrentGameInstance);

	// TODO : actually implement contracts

	// Build the description texts
	Details.Title       = LOCTEXT("TutorialContract", "Tutorial contract");
	Details.Description = FText::FormatNamed(LOCTEXT("TutorialContractDescription", "Buy {quantity} containers of {resource}"),
		TEXT("quantity"), FText::AsNumber(2), TEXT("resource"), LOCTEXT("Test", "Test"));
}

TSharedRef<FJsonObject> FNovaTutorialContract::Save() const
{
	TSharedRef<FJsonObject> Data = FNovaContract::Save();

	return Data;
}

void FNovaTutorialContract::Load(const TSharedPtr<FJsonObject>& Data)
{
	FNovaContract::Load(Data);

	Details.Progress = 0.23f;
}

void FNovaTutorialContract::OnEvent(const FNovaContractEvent& Event)
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
