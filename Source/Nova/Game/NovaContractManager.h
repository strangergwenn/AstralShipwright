// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Nova/Game/NovaGameTypes.h"
#include "NovaContractManager.generated.h"


/** Contracts presentation details */
struct FNovaContractDetails
{
	FText Title;
	FText Description;
	float Progress;
};

/** Contract event type */
enum ENovaContratEventType
{
	Tick
};

/** Contract event data */
struct FNovaContractEvent
{
	FNovaContractEvent(ENovaContratEventType T)
		: Type(T)
	{}

	ENovaContratEventType Type;
};


/** Contract manager to interact with contract objects */
UCLASS(ClassGroup = (Nova))
class UNovaContractManager : public UObject
{
	GENERATED_BODY()

public:

	UNovaContractManager();


	/*----------------------------------------------------
		Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaContractManagerSave> Save() const;

	void Load(TSharedPtr<struct FNovaContractManagerSave> SaveData);

	static void SerializeJson(TSharedPtr<struct FNovaContractManagerSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);


	/*----------------------------------------------------
		System interface
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNovaContractManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNovaGameInstance* Instance);

	/** Update all contracts */
	void OnEvent(FNovaContractEvent Event);


	/*----------------------------------------------------
		Game interface
	----------------------------------------------------*/

	/** Is it still possible to generate new contracts */
	bool CanGenerateContract() const;

	/** Generate a new contract that will sit in temporary memory until it is accepted or declined */
	FNovaContractDetails GenerateNewContract();

	/** Accept the newly generated contract and save it to persistent storage */
	void AcceptContract();

	/** Decline the newly generated contract and destroy it */
	void DeclineContract();

	/** Update a contract */
	void ProgressContract(TSharedPtr<class FNovaContract> Contract);

	/** Complete a contract and destroy it */
	void CompleteContract(TSharedPtr<class FNovaContract> Contract);


	/** Fetch the number of active contracts */
	uint32 GetContractCount() const;

	/** Get the details text of a contract based on its index */
	FNovaContractDetails GetContractDetails(int32 Index) const;

	/** Set a particular contract as the tracked one, pass INDEX_NONE to untrack */
	void SetTrackedContract(int32 Index);

	/** Abandon a contract */
	void AbandonContract(int32 Index);

	/** Get the tracked contract, or INDEX_NONE if none */
	int32 GetTrackedContract();


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:

	// Singleton pointer
	static UNovaContractManager*                  Singleton;

	// Game instance pointer
	UPROPERTY()
	class UNovaGameInstance*                      GameInstance;

	// State
	TSharedPtr<class FNovaContract>               GeneratedContract;
	TArray<TSharedPtr<class FNovaContract>>       CurrentContracts;
	int32                                         CurrentTrackedContract;

};
