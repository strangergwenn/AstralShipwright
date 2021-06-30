// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/EngineBaseTypes.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/*----------------------------------------------------
    Debugging tools
----------------------------------------------------*/

DECLARE_LOG_CATEGORY_EXTERN(LogNova, Log, All);

#define NDIS(Format, ...) FNovaModule::DisplayLog(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define NLOG(Format, ...) UE_LOG(LogNova, Display, TEXT(Format), ##__VA_ARGS__)
#define NERR(Format, ...) UE_LOG(LogNova, Error, TEXT(Format), ##__VA_ARGS__)

FString GetEnumText(ENetMode Mode);

FString GetEnumText(ENetRole Role);

FString GetRoleString(const AActor* Actor);

FString GetRoleString(const UActorComponent* Component);

FText GetDurationText(struct FNovaTime Time, int32 MaxComponents = 4);

FText GetPriceText(struct FNovaCredits Credits);

/*----------------------------------------------------
    Error reporting
----------------------------------------------------*/

/** Ensure an expression is true */
#define NCHECK(Expression) verify(Expression)

/*----------------------------------------------------
    Game module definition
----------------------------------------------------*/

class FNovaModule : public FDefaultGameModuleImpl
{

public:
	void StartupModule() override;

	void ShutdownModule() override;

	/** Display a log on the screen */
	static void DisplayLog(FString Log);
};
