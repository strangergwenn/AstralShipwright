// Nova project - Gwennaël Arbona

#include "NovaSaveManager.h"

#include "Nova/Game/NovaGameTypes.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Nova.h"

#include "Dom/JsonObject.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Core/Public/HAL/FileManager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Async/AsyncWork.h"
#include "Runtime/Json/Public/Serialization/JsonWriter.h"
#include "Runtime/Json/Public/Serialization/JsonSerializer.h"

/*----------------------------------------------------
    Asynchronous task
----------------------------------------------------*/

class FNovaAsyncSave : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FNovaAsyncSave>;

public:
	FNovaAsyncSave(
		UNovaSaveManager* SaveSystemParam, const FString SaveNameParam, TSharedPtr<FNovaGameSave> SaveDataParam, bool CompressParam)
		: SaveName(SaveNameParam), SaveData(SaveDataParam), SaveSystem(SaveSystemParam), Compress(CompressParam)
	{}

protected:
	void DoWork()
	{
		NLOG("FNovaAsyncSave::DoWork : started");

		SaveSystem->SaveGame(SaveName, SaveData, Compress);

		NLOG("FNovaAsyncSave::DoWork : done");
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FNovaAsyncSave, STATGROUP_ThreadPoolAsyncTasks);
	}

protected:
	FString                   SaveName;
	TSharedPtr<FNovaGameSave> SaveData;
	UNovaSaveManager*         SaveSystem;
	bool                      Compress;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSaveManager::UNovaSaveManager() : Super()
{}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

bool UNovaSaveManager::DoesSaveExist(const FString SaveName)
{
	return IFileManager::Get().FileSize(*GetSaveGamePath(SaveName, true)) >= 0 ||
		   IFileManager::Get().FileSize(*GetSaveGamePath(SaveName, false)) >= 0;
}

void UNovaSaveManager::SaveGameAsync(const FString SaveName, TSharedPtr<FNovaGameSave> SaveData, bool Compress)
{
	NCHECK(SaveData.IsValid());

	SaveListLock.Lock();

	if (SaveList.Find(SaveData) != INDEX_NONE)
	{
		NLOG("UNovaSaveManager::SaveGameAsync : save to '%s' already in progress, aborting", *SaveName);
		SaveListLock.Unlock();
		return;
	}
	else
	{
		SaveList.Add(SaveData);
		SaveListLock.Unlock();
	}

	(new FAutoDeleteAsyncTask<FNovaAsyncSave>(this, SaveName, SaveData, Compress))->StartBackgroundTask();
}

bool UNovaSaveManager::SaveGame(const FString SaveName, TSharedPtr<FNovaGameSave> SaveData, bool Compress)
{
	NLOG("UNovaSaveManager::SaveGame : saving to '%s'", *SaveName);

	NCHECK(SaveData.IsValid());

	SaveLock.Lock();

	// Initialize
	bool                    Result = false;
	TSharedPtr<FJsonObject> JsonData;
	UNovaGameInstance::SerializeJson(SaveData, JsonData, ENovaSerialize::DataToJson);
	FString FileContents = JsonToString(JsonData);

	// Serialize the JSON objects and compress
	if (Compress)
	{
		// Estimate sizes - 4 bytes to store size, 512 byes of margin for small files with higher compressed sizes
		uint32 DataSize               = FCStringAnsi::Strlen(TCHAR_TO_UTF8(*FileContents));
		int32  ContentsCompressedSize = DataSize + 512;
		int32  FullCompressedSize     = ContentsCompressedSize + 4;

		// Allocate a buffer
		uint8* CompressedData         = new uint8[FullCompressedSize];
		uint8* CompressedDataContents = CompressedData + 4;

		// Write size
		CompressedData[0] = (DataSize >> 24) & 0xFF;
		CompressedData[1] = (DataSize >> 16) & 0xFF;
		CompressedData[2] = (DataSize >> 8) & 0xFF;
		CompressedData[3] = (DataSize >> 0) & 0xFF;

		// Compress contents
		if (FCompression::CompressMemory(NAME_Zlib, CompressedDataContents, ContentsCompressedSize, TCHAR_TO_UTF8(*FileContents), DataSize))
		{
			int32 EffectiveCompressedSize = ContentsCompressedSize + 4;
			Result                        = FFileHelper::SaveArrayToFile(
                TArrayView<const uint8>(CompressedData, EffectiveCompressedSize), *GetSaveGamePath(SaveName, true));
		}
		else
		{
			NERR("UNovaSaveManager::SaveGame : failed to compress data");
		}

		delete[] CompressedData;
	}
	else
	{
		Result = FFileHelper::SaveStringToFile(FileContents, *GetSaveGamePath(SaveName, false));
	}

	NLOG("UNovaSaveManager::SaveGame : done with result %d", Result);

	SaveLock.Unlock();

	// Pop the request from the list
	SaveListLock.Lock();
	SaveList.Remove(SaveData);
	SaveListLock.Unlock();

	return Result;
}

TSharedPtr<FNovaGameSave> UNovaSaveManager::LoadGame(const FString SaveName)
{
	NLOG("UNovaSaveManager::LoadGame : loading from '%s'", *SaveName);

	// Initialize
	FString       SaveString;
	TArray<uint8> Data;
	bool          SaveStringLoaded = false;

	auto LoadCompressedFileToString = [&](FString& Result, const TCHAR* Filename)
	{
		TArray<uint8> CompressedData;
		if (!FFileHelper::LoadFileToArray(CompressedData, Filename))
		{
			NLOG("UNovaSaveManager::LoadGame : no compressed save file found");
			return false;
		}

		// Read uncompressed size
		int32 UncompressedSize = (CompressedData[0] << 24) + (CompressedData[1] << 16) + (CompressedData[2] << 8) + CompressedData[3];
		Data.SetNum(UncompressedSize + 1);

		// Uncompress
		if (!FCompression::UncompressMemory(
				NAME_Zlib, Data.GetData(), UncompressedSize, CompressedData.GetData() + 4, CompressedData.Num() - 4))
		{
			NERR("UNovaSaveManager::LoadGame : failed to uncompress with compressed size %d and uncompressed size %d", CompressedData.Num(),
				UncompressedSize);
			return false;
		}

		// Convert to string
		Data[UncompressedSize] = 0;
		Result                 = UTF8_TO_TCHAR(Data.GetData());
		return true;
	};

	// Check which file to load
	if (LoadCompressedFileToString(SaveString, *GetSaveGamePath(SaveName, true)))
	{
		NLOG("UNovaSaveManager::LoadGame : read '%s'", *GetSaveGamePath(SaveName, true));
		SaveStringLoaded = true;
	}
	else if (FFileHelper::LoadFileToString(SaveString, *GetSaveGamePath(SaveName, false)))
	{
		NLOG("UNovaSaveManager::LoadGame : read '%s'", *GetSaveGamePath(SaveName, false));
		SaveStringLoaded = true;
	}

	// Deserialize a JSON object from the string
	if (SaveStringLoaded)
	{
		TSharedPtr<FNovaGameSave> SaveData;
		TSharedPtr<FJsonObject>   JsonData = StringToJson(SaveString);
		UNovaGameInstance::SerializeJson(SaveData, JsonData, ENovaSerialize::JsonToData);

		return SaveData;
	}
	else
	{
		NERR("UNovaSaveManager::LoadGame : failed to read either '%s' or '%s'", *GetSaveGamePath(SaveName, true),
			*GetSaveGamePath(SaveName, false));
	}

	return TSharedPtr<FNovaGameSave>();
}

bool UNovaSaveManager::DeleteGame(const FString SaveName)
{
	bool Result = IFileManager::Get().Delete(*GetSaveGamePath(SaveName, false), true) |
				  IFileManager::Get().Delete(*GetSaveGamePath(SaveName, true), true);
	return Result;
}

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

FString UNovaSaveManager::GetSaveGamePath(const FString SaveName, bool Compressed)
{
	if (Compressed)
	{
		return FString::Printf(TEXT("%s/%s.sav"), *FPaths::ProjectSavedDir(), *SaveName);
	}
	else
	{
		return FString::Printf(TEXT("%s/%s.json"), *FPaths::ProjectSavedDir(), *SaveName);
	}
}

FString UNovaSaveManager::JsonToString(const TSharedPtr<FJsonObject>& SaveData)
{
	FString                   SerializedSaveData;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&SerializedSaveData);
	if (!FJsonSerializer::Serialize(SaveData.ToSharedRef(), JsonWriter))
	{
		NCHECK(false);
	}

	JsonWriter->Close();

	return SerializedSaveData;
}

TSharedPtr<FJsonObject> UNovaSaveManager::StringToJson(const FString& SerializedSaveData)
{
	TSharedPtr<FJsonObject>   SaveData;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SerializedSaveData);
	if (!FJsonSerializer::Deserialize(Reader, SaveData) || !SaveData.IsValid())
	{
		NCHECK(false);
	}

	return SaveData;
}

FGuid UNovaSaveManager::DeserializeGuid(const TSharedPtr<FJsonObject>& SaveData, const FString& FieldName)
{
	FGuid Identifier;

	if (SaveData->HasField(FieldName))
	{
		FGuid::Parse(SaveData->GetStringField(FieldName), Identifier);
	}

	return Identifier;
}
