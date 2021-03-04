// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSaveManager.generated.h"

/** Game interface to load and write saves */
UCLASS(ClassGroup = (Nova))
class UNovaSaveManager : public UObject
{
	GENERATED_BODY()

public:
	UNovaSaveManager();

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	/** Confirm if a save slot does exist */
	bool DoesSaveExist(const FString SaveName);

	/** Start an asynchronous process to save data */
	void SaveGameAsync(const FString SaveName, TSharedPtr<struct FNovaGameSave> SaveData, bool Compress = true);

	/** Serialize and save a game state structure synchronously to the filesystem with optional compression */
	bool SaveGame(const FString SaveName, TSharedPtr<struct FNovaGameSave> SaveData, bool Compress = true);

	/** Load a game state structure synchronously from the filesystem */
	TSharedPtr<struct FNovaGameSave> LoadGame(const FString SaveName);

	/** Delete a game save */
	bool DeleteGame(const FString SaveName);

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
	// Prepared game save data
	TArray<TSharedPtr<struct FNovaGameSave>> SaveList;

	// Critical sections
	FCriticalSection SaveLock;
	FCriticalSection SaveListLock;
};
