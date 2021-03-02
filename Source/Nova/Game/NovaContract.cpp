// Nova project - Gwennaël Arbona

#include "NovaContract.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaAssetCatalog.h"

#include "Nova/Nova.h"

#define LOCTEXT_NAMESPACE "FNovaContract"

/*----------------------------------------------------
    Base class
----------------------------------------------------*/

TSharedPtr<FNovaContract> FNovaContract::New(ENovaContractType Type, UNovaGameInstance* CurrentGameInstance)
{
	// Create the contract
	TSharedPtr<FNovaContract> Contract;
	if (Type == ENovaContractType::Tutorial)
	{
		Contract = MakeShared<FNovaTutorialContract>();
	}
	else
	{
		NCHECK(false);
	}

	// Create the contract
	Contract->Initialize(CurrentGameInstance);

	return Contract;
}

void FNovaContract::Initialize(UNovaGameInstance* CurrentGameInstance)
{
	GameInstance = CurrentGameInstance;
}

TSharedRef<FJsonObject> FNovaContract::Save() const
{
	TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();

	return Data;
}

void FNovaContract::Load(const TSharedPtr<FJsonObject>& Data)
{}

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
