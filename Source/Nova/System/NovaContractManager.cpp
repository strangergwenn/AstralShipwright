// Nova project - Gwennaël Arbona

#include "NovaContractManager.h"
#include "NovaGameInstance.h"

#include "Nova/Game/Contract/NovaContract.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Nova.h"

#define LOCTEXT_NAMESPACE "UNovaContractManager"

// Statics
UNovaContractManager* UNovaContractManager::Singleton = nullptr;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaContractManager::UNovaContractManager()
{}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

struct FNovaContractManagerSave
{
	TMap<ENovaContractType, TSharedPtr<FJsonObject>> CurrentContracts;

	int32 CurrentTrackedContract;
};

TSharedPtr<FNovaContractManagerSave> UNovaContractManager::Save() const
{
	TSharedPtr<FNovaContractManagerSave> SaveData = MakeShared<FNovaContractManagerSave>();

	// Save contracts
	for (TSharedPtr<FNovaContract> Contract : CurrentContracts)
	{
		SaveData->CurrentContracts.Add(TPair<ENovaContractType, TSharedPtr<FJsonObject>>(Contract->GetType(), Contract->Save()));
	}

	// Save the tracked contract
	SaveData->CurrentTrackedContract = CurrentTrackedContract;

	return SaveData;
}

void UNovaContractManager::Load(TSharedPtr<FNovaContractManagerSave> SaveData)
{
	// Reset the state from a potential previous session
	CurrentContracts.Empty();
	GeneratedContract.Reset();

	// Load contracts
	if (SaveData)
	{
		for (TPair<ENovaContractType, TSharedPtr<FJsonObject>> ContractData : SaveData->CurrentContracts)
		{
			ENovaContractType ContractType = ContractData.Key;

			TSharedPtr<FNovaContract> Contract = FNovaContract::New(ContractType, GameInstance);
			Contract->Load(ContractData.Value);

			CurrentContracts.Add(Contract);
		}

		// Get the tracked contract
		CurrentTrackedContract = SaveData->CurrentTrackedContract;
	}

	// No contract structure was found, so it's a new game
	else
	{
		NLOG("UNovaContractManager::Load : adding tutorial contract");

		// TODO : tutorial contract and track it
		// TSharedPtr<FNovaContract> Tutorial = FNovaContract::New(ENovaContractType::Tutorial, GameInstance);
		// CurrentContracts.Add(Tutorial);

		CurrentTrackedContract = INDEX_NONE;
	}
}

void UNovaContractManager::SerializeJson(
	TSharedPtr<FNovaContractManagerSave>& SaveData, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShared<FJsonObject>();

		TArray<TSharedPtr<FJsonValue>> SavedContracts;
		for (TPair<ENovaContractType, TSharedPtr<FJsonObject>> ContractData : SaveData->CurrentContracts)
		{
			TSharedPtr<FJsonObject> ContractJsonData = MakeShared<FJsonObject>();

			ContractJsonData->SetNumberField("Type", static_cast<uint8>(ContractData.Key));
			ContractJsonData->SetObjectField("Object", ContractJsonData);

			SavedContracts.Add(MakeShared<FJsonValueObject>(ContractJsonData));
		}

		JsonData->SetArrayField("Contracts", SavedContracts);
		JsonData->SetNumberField("TrackedContract", SaveData->CurrentTrackedContract);
	}
	else
	{
		SaveData = MakeShared<FNovaContractManagerSave>();

		const TArray<TSharedPtr<FJsonValue>>* SavedContracts;
		if (JsonData->TryGetArrayField("Contracts", SavedContracts))
		{
			for (TSharedPtr<FJsonValue> ContractValue : *SavedContracts)
			{
				TSharedPtr<FJsonObject> ContractEntry = ContractValue->AsObject();

				SaveData->CurrentContracts.Add(TPair<ENovaContractType, TSharedPtr<FJsonObject>>(
					static_cast<ENovaContractType>((uint8) ContractEntry->GetNumberField("Type")),
					ContractEntry->GetObjectField("Object")));
			}
		}

		SaveData->CurrentTrackedContract = 0;
		JsonData->TryGetNumberField("TrackedContract", SaveData->CurrentTrackedContract);
	}
}

/*----------------------------------------------------
    System interface
----------------------------------------------------*/

void UNovaContractManager::Initialize(class UNovaGameInstance* Instance)
{
	Singleton    = this;
	GameInstance = Instance;
}

void UNovaContractManager::OnEvent(FNovaContractEvent Event)
{
	TArray<TSharedPtr<FNovaContract>> SafeCurrentContracts = CurrentContracts;
	for (TSharedPtr<FNovaContract> Contract : SafeCurrentContracts)
	{
		Contract->OnEvent(Event);
	}
}

/*----------------------------------------------------
    Game interface
----------------------------------------------------*/

bool UNovaContractManager::CanGenerateContract() const
{
	return GetContractCount() < ENovaConstants::MaxContractsCount;
}

FNovaContractDetails UNovaContractManager::GenerateNewContract()
{
	NLOG("UNovaContractManager::GenerateNewContract");

	NCHECK(false);

	// TODO : create contracts here
	// GeneratedContract = FNovaContract::New(Type, GameInstance);

	return GeneratedContract->GetDisplayDetails();
}

void UNovaContractManager::AcceptContract()
{
	NLOG("UNovaContractManager::AcceptContract");

	CurrentContracts.Add(GeneratedContract);
	GeneratedContract.Reset();

	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GameInstance->GetFirstLocalPlayerController());
	PC->Notify(LOCTEXT("ContractAccepted", "Contract accepted"), FText(), ENovaNotificationType::Info);
}

void UNovaContractManager::DeclineContract()
{
	NLOG("UNovaContractManager::DeclineContract");

	GeneratedContract.Reset();
}

void UNovaContractManager::ProgressContract(TSharedPtr<class FNovaContract> Contract)
{
	NLOG("UNovaContractManager::ProgressContract");

	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GameInstance->GetFirstLocalPlayerController());
	PC->Notify(LOCTEXT("ContractUpdated", "Contract updated"), FText(), ENovaNotificationType::Info);
}

void UNovaContractManager::CompleteContract(TSharedPtr<class FNovaContract> Contract)
{
	NLOG("UNovaContractManager::CompleteContract");

	CurrentContracts.Remove(Contract);

	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GameInstance->GetFirstLocalPlayerController());
	PC->Notify(LOCTEXT("ContractComplete", "Contract complete"), FText(), ENovaNotificationType::Info);
}

uint32 UNovaContractManager::GetContractCount() const
{
	return CurrentContracts.Num();
}

FNovaContractDetails UNovaContractManager::GetContractDetails(int32 Index) const
{
	NCHECK(Index >= 0 && Index < CurrentContracts.Num());

	return CurrentContracts[Index]->GetDisplayDetails();
}

void UNovaContractManager::SetTrackedContract(int32 Index)
{
	NLOG("UNovaContractManager::SetTrackedContract %d", Index);

	CurrentTrackedContract = Index;

	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GameInstance->GetFirstLocalPlayerController());
	if (Index >= 0)
	{
		PC->Notify(LOCTEXT("ContractTracked", "Contract tracked"), FText(), ENovaNotificationType::Info);
	}
	else
	{
		PC->Notify(LOCTEXT("ContractUntracked", "Contract untracked"), FText(), ENovaNotificationType::Info);
	}
}

void UNovaContractManager::AbandonContract(int32 Index)
{
	NLOG("UNovaContractManager::AbandonContract %d", Index);

	NCHECK(Index >= 0 && Index < CurrentContracts.Num());

	CurrentContracts.RemoveAt(Index);
	if (CurrentTrackedContract == Index)
	{
		CurrentTrackedContract = INDEX_NONE;
	}

	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GameInstance->GetFirstLocalPlayerController());
	PC->Notify(LOCTEXT("ContractAbandoned", "Contract abandoned"), FText(), ENovaNotificationType::Info);
}

int32 UNovaContractManager::GetTrackedContract()
{
	return CurrentContracts.Num() > 0 ? CurrentTrackedContract : INDEX_NONE;
}

#undef LOCTEXT_NAMESPACE
