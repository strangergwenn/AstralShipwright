// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "NovaSaveManager.generated.h"

/** Base type for save data */
USTRUCT()
struct FNovaSaveDataBase
{
	GENERATED_BODY()
};

/** Game interface to load and write saves */
UCLASS(ClassGroup = (Nova))
class UNovaSaveManager : public UObject
{
	GENERATED_BODY()

	friend class FNovaAsyncSave;

public:

	UNovaSaveManager();

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNovaSaveManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNovaGameInstance* GameInstance)
	{
		Singleton = this;
	}

	/** Confirm if a save slot does exist */
	bool DoesSaveExist(const FString SaveName);

	/** Delete a game save */
	bool DeleteGame(const FString SaveName);

	/** Check for currently loaded save data */
	bool HasLoadedSaveData() const
	{
		return CurrentSaveData.IsValid();
	}

	/** Get the current save data */
	template <typename SaveDataType>
	TSharedPtr<SaveDataType> GetCurrentSaveData() const
	{
		return StaticCastSharedPtr<SaveDataType>(CurrentSaveData);
	}

	/** Set the current save data */
	template <typename SaveDataType>
	void SetCurrentSaveData(TSharedPtr<SaveDataType> SaveData)
	{
		NLOG("UNovaSaveManager::SetCurrentSaveData");

		CurrentSaveData = SaveData;
	}

	/** Reset the current save data */
	void ReleaseCurrentSaveData()
	{
		NLOG("UNovaSaveManager::ReleaseSaveData");

		CurrentSaveData = nullptr;
	}

	/** Write the current save data to the currently loaded slot */
	template <typename SaveDataType>
	void WriteCurrentSaveData(bool Synchronous = false, bool Compress = true)
	{
		NLOG("UNovaSaveManager::SaveGameToFile : synchronous %d", Synchronous);

		if (HasLoadedSaveData())
		{
			if (Synchronous)
			{
				SaveGame<SaveDataType>(CurrentSaveFileName, GetCurrentSaveData<SaveDataType>(), Compress);
			}
			else
			{
				SaveGameAsync<SaveDataType>(CurrentSaveFileName, GetCurrentSaveData<SaveDataType>(), Compress);
			}
		}
	}

	/** Get the time in minutes since the last loading or saving */
	double GetMinutesSinceLastSave() const
	{
		double CurrentTime = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());

		return (CurrentTime - TimeOfLastSave) / (1000.0 * 60.0);
	}

	/** Start an asynchronous process to save data */
	template <typename SaveDataType>
	void SaveGameAsync(const FString SaveName, TSharedPtr<SaveDataType> SaveData, bool Compress = true)
	{
		// Serialize
		TSharedPtr<class FJsonObject> JsonData = FJsonObjectConverter::UStructToJsonObject<SaveDataType>(*SaveData);

		// Save
		SaveGameAsync(SaveName, JsonData, Compress);

		// Reset the save time
		TimeOfLastSave = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());
	}

	/** Serialize and save a game state structure synchronously to the filesystem with optional compression */
	template <typename SaveDataType>
	void SaveGame(const FString SaveName, TSharedPtr<SaveDataType> SaveData, bool Compress = true)
	{
		// Serialize
		TSharedPtr<class FJsonObject> JsonData = FJsonObjectConverter::UStructToJsonObject<SaveDataType>(*SaveData);

		// Save
		SaveGame(SaveName, JsonData, Compress);

		// Reset the save time
		TimeOfLastSave = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());
	}

	/** Load a game state structure synchronously from the filesystem */
	template <typename SaveDataType>
	TSharedPtr<SaveDataType> LoadGame(const FString SaveName)
	{
		CurrentSaveFileName = SaveName;

		// Load
		TSharedPtr<class FJsonObject> JsonData = LoadGameInternal(SaveName);

		// Reset the save time
		TimeOfLastSave = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());

		// Deserialize the data
		CurrentSaveData = MakeShared<SaveDataType>();
		FJsonObjectConverter::JsonObjectToUStruct<SaveDataType>(JsonData.ToSharedRef(), static_cast<SaveDataType*>(CurrentSaveData.Get()));
		return StaticCastSharedPtr<SaveDataType>(CurrentSaveData);
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:

	/** Start an asynchronous process to save data */
	void SaveGameAsync(const FString SaveName, TSharedPtr<class FJsonObject> JsonData, bool Compress = true);

	/** Serialize and save a game state structure synchronously to the filesystem with optional compression */
	bool SaveGame(const FString SaveName, TSharedPtr<class FJsonObject> JsonData, bool Compress = true);

	/** Implementation of game loading */
	TSharedPtr<FJsonObject> LoadGameInternal(const FString SaveName);

public:

	/*----------------------------------------------------
	    Helpers
	----------------------------------------------------*/

	/** Get the path to save game file for the given name */
	static FString GetSaveGamePath(const FString SaveName, bool Compressed);

	/** Serialize a save data object into a string */
	static FString JsonToString(const TSharedPtr<class FJsonObject>& SaveData);

	/** Deserialize a string into a save data object */
	static TSharedPtr<class FJsonObject> StringToJson(const FString& SerializedSaveData);

	/** De-serialize an FGuid description into an asset pointer */
	static FGuid DeserializeGuid(const TSharedPtr<class FJsonObject>& SaveData, const FString& FieldName);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Singleton pointer
	static UNovaSaveManager* Singleton;

	// Critical sections
	FCriticalSection SaveLock;
	FCriticalSection SaveListLock;

	// Save data
	TSharedPtr<FNovaSaveDataBase> CurrentSaveData;
	FString                       CurrentSaveFileName;
	double                        TimeOfLastSave;

	// Prepared game save data
	TArray<TSharedPtr<class FJsonObject>> SaveList;
};
