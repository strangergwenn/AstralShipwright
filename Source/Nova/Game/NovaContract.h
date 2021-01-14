// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NovaContractManager.h"


/*----------------------------------------------------
	Base contract definitions
----------------------------------------------------*/

/** Contract types */
enum class ENovaContractType : uint8
{
	Tutorial
};

/** Contracts internal*/
class FNovaContract : public TSharedFromThis<FNovaContract>
{
public:

	FNovaContract()
		: GameInstance(nullptr)
	{}

	virtual ~FNovaContract()
	{}

	/** Create a new contract */
	static TSharedPtr<FNovaContract> New(ENovaContractType Type, class UNovaGameInstance* CurrentGameInstance);

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
	ENovaContractType                             Type;
	FNovaContractDetails                          Details;
	class UNovaGameInstance*                      GameInstance;

};


/*----------------------------------------------------
	Contracts implementations
----------------------------------------------------*/

/** Test class for contracts */
class FNovaTutorialContract : public FNovaContract
{
public:

	FNovaTutorialContract();

	virtual void Initialize(class UNovaGameInstance* CurrentGameInstance) override;

	virtual TSharedRef<FJsonObject> Save() const override;

	virtual void Load(const TSharedPtr<FJsonObject>& Data) override;

	virtual void OnEvent(const FNovaContractEvent& Event) override;

};

