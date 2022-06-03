// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Dom/JsonObject.h"

#include "Game/NovaGameTypes.h"

#include "NovaContractManager.generated.h"

/*----------------------------------------------------
    Supporting types
----------------------------------------------------*/

/** Contracts presentation details */
struct FNovaContractDetails
{
	FText Title;
	FText Description;
	float Progress;
};

/** Contract types */
enum class ENovaContractType : uint8
{
	Tutorial
};

/** Contract event type */
enum ENovaContratEventType
{
	Tick
};

/** Contract event data */
struct FNovaContractEvent
{
	FNovaContractEvent(ENovaContratEventType T) : Type(T)
	{}

	ENovaContratEventType Type;
};

// Contract creation delegate
DECLARE_DELEGATE_RetVal_TwoParams(
	TSharedPtr<class FNovaContract>, FNovaContractCreationCallback, ENovaContractType Type, UNovaGameInstance* CurrentGameInstance);

/*----------------------------------------------------
    Base contract definitions
----------------------------------------------------*/

/** Contracts internal*/
class FNovaContract : public TSharedFromThis<FNovaContract>
{
public:

	FNovaContract() : GameInstance(nullptr)
	{}

	virtual ~FNovaContract()
	{}

	/** Create the contract */
	virtual void Initialize(class UNovaGameInstance* CurrentGameInstance);

	/** Save this object to save data */
	virtual TSharedRef<FJsonObject> Save() const;

	/** Load this object from save data */
	virtual void Load(const TSharedPtr<FJsonObject>& Data);

	/** Get a numerical type identifier for this contract */
	virtual ENovaContractType GetType() const
	{
		return Type;
	}

	/** Get the display text, progress, etc for this contract */
	FNovaContractDetails GetDisplayDetails() const
	{
		return Details;
	}

	/** Update this contract */
	virtual void OnEvent(const FNovaContractEvent& Event) = 0;

protected:

	// Local state
	ENovaContractType        Type;
	FNovaContractDetails     Details;
	class UNovaGameInstance* GameInstance;
};

/*----------------------------------------------------
    Contract manager
----------------------------------------------------*/

/** Contract manager to interact with contract objects */
UCLASS(ClassGroup = (Nova))
class UNovaContractManager
	: public UObject
	, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UNovaContractManager();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaContractManagerSave> Save() const;

	void Load(TSharedPtr<struct FNovaContractManagerSave> SaveData);

	static void SerializeJson(
		TSharedPtr<struct FNovaContractManagerSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

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

	/** Start playing on a new level */
	void BeginPlay(class ANovaPlayerController* PC, FNovaContractCreationCallback CreationCallback);

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
	    Tick
	----------------------------------------------------*/

	virtual void              Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UNovaSoundManager, STATGROUP_Tickables);
	}
	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Singleton pointer
	static UNovaContractManager* Singleton;

	// Player owner
	UPROPERTY()
	class ANovaPlayerController* PlayerController;

	// Game instance pointer
	UPROPERTY()
	class UNovaGameInstance* GameInstance;

	// State
	FNovaContractCreationCallback           ContractGenerator;
	TSharedPtr<class FNovaContract>         GeneratedContract;
	TArray<TSharedPtr<class FNovaContract>> CurrentContracts;
	int32                                   CurrentTrackedContract;
};
