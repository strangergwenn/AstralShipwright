// Nova project - Gwennaël Arbona

#include "Nova.h"
#include "UI/Style/NovaStyleSet.h"

#include "Runtime/Online/HTTP/Public/Http.h"
#include "Runtime/Online/HTTP/Public/HttpManager.h"
#include "HAL/PlatformStackWalk.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "HAL/ThreadSafeBool.h"
#include "Engine.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FNovaModule, Nova, "Nova");

DEFINE_LOG_CATEGORY(LogNova)

#define LOCTEXT_NAMESPACE "Nova"

/*----------------------------------------------------
    Module loading / unloading code
----------------------------------------------------*/

void FNovaModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();
	FNovaStyleSet::Initialize();
}

void FNovaModule::ShutdownModule()
{
	FDefaultGameModuleImpl::ShutdownModule();
	FNovaStyleSet::Shutdown();
}

/*----------------------------------------------------
    Debugging tools
----------------------------------------------------*/

FString GetEnumText(ENetMode Mode)
{
	switch (Mode)
	{
		case NM_Client:
			return "Client";
		case NM_DedicatedServer:
			return "DedicatedServer";
		case NM_ListenServer:
			return "ListenServer";
		case NM_Standalone:
			return "Standalone";
		default:
			return "ERROR";
	}
}

FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
		case ROLE_None:
			return "None";
		case ROLE_SimulatedProxy:
			return "SimulatedProxy";
		case ROLE_AutonomousProxy:
			return "AutonomousProxy";
		case ROLE_Authority:
			return "Authority";
		default:
			return "ERROR";
	}
}

FString GetRoleString(const AActor* Actor)
{
	return GetEnumText(Actor->GetLocalRole());
}

FString GetRoleString(const UActorComponent* Component)
{
	return GetEnumText(Component->GetOwnerRole());
}

FText GetDurationText(float SourceMinutes, int32 MaxComponents)
{
	FString Result;

	if (SourceMinutes > 365 * 24 * 60)
	{
		return LOCTEXT("VeryLongTime", "A very long time");
	}

	// Break up the time
	int32 Days = FMath::Floor(SourceMinutes / (24 * 60));
	SourceMinutes -= Days * 24 * 60;
	int32 Hours = FMath::Floor(SourceMinutes / 60);
	SourceMinutes -= Hours * 60;
	int32 Minutes = FMath::Floor(SourceMinutes);
	SourceMinutes -= Minutes;
	int32 Seconds = FMath::Floor(SourceMinutes * 60.0f);

	// Format the time
	int32 ComponentCount = 0;
	if (Days)
	{
		if (ComponentCount == MaxComponents - 1)
		{
			Days++;
		}

		Result += FText::FormatNamed(LOCTEXT("DaysFormat", "{days} {days}|plural(one=day,other=days)"), TEXT("days"), FText::AsNumber(Days))
					  .ToString();
		ComponentCount++;
	}
	if (Hours && ComponentCount < MaxComponents)
	{
		if (ComponentCount > 0)
		{
			Result += TEXT(", ");
		}
		if (ComponentCount == MaxComponents - 1)
		{
			Hours++;
		}

		Result += FText::FormatNamed(
			LOCTEXT("HoursFormat", "{hours} {hours}|plural(one=hour,other=hours)"), TEXT("hours"), FText::AsNumber(Hours))
					  .ToString();
		ComponentCount++;
	}
	if (Minutes && ComponentCount < MaxComponents)
	{
		if (ComponentCount > 0)
		{
			Result += TEXT(", ");
		}
		if (ComponentCount == MaxComponents - 1)
		{
			Minutes++;
		}

		Result += FText::FormatNamed(
			LOCTEXT("MinutesFormat", "{minutes} {minutes}|plural(one=minute,other=minutes)"), TEXT("minutes"), FText::AsNumber(Minutes))
					  .ToString();
		ComponentCount++;
	}
	if (Seconds && ComponentCount < MaxComponents)
	{
		if (ComponentCount > 0)
		{
			Result += TEXT(", ");
		}
		if (ComponentCount == MaxComponents - 1)
		{
			Seconds++;
		}

		Result += FText::FormatNamed(
			LOCTEXT("SecondsFormat", "{seconds} {seconds}|plural(one=second,other=seconds)"), TEXT("seconds"), FText::AsNumber(Seconds))
					  .ToString();
		ComponentCount++;
	}

	return FText::FromString(Result);
}

/*----------------------------------------------------
    Module code
----------------------------------------------------*/

FThreadSafeBool HasCrashed = false;

void FNovaModule::DisplayLog(FString Log)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, Log);
	UE_LOG(LogNova, Display, TEXT("%s"), *Log);
}

void FNovaModule::ReportError(FString Function)
{
	// Ensure no duplicates
	if (HasCrashed)
	{
		return;
	}
	HasCrashed = true;

	// Get stack trace
	ANSICHAR Callstack[16384];
	Callstack[0] = 0;
	TCHAR CallstackString[16384];
	CallstackString[0] = 0;
	FPlatformStackWalk::StackWalkAndDumpEx(
		Callstack, UE_ARRAY_COUNT(Callstack), 1, FGenericPlatformStackWalk::EStackWalkFlags::AccurateStackWalk);
	FCString::Strncat(CallstackString, ANSI_TO_TCHAR(Callstack), UE_ARRAY_COUNT(CallstackString) - 1);

#if UE_BUILD_SHIPPING

	// Parameters
	FString ReportURL      = TEXT("https://deimos.games/report.php");
	FString GameString     = TEXT("Nova");
	FString GameParameter  = TEXT("game");
	FString TitleParameter = TEXT("title");
	FString StackParameter = TEXT("callstack");

	// Format data
	FString RequestContent = GameParameter + TEXT("=") + GameString + TEXT("&") + TitleParameter + TEXT("=") + Function + TEXT("&") +
							 StackParameter + TEXT("=") + FString(CallstackString);

	// Report to server
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ReportURL);
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-DeimosGames-Agent");
	Request->SetHeader("Content-Type", TEXT("application/x-www-form-urlencoded"));
	Request->SetContentAsString(RequestContent);
	Request->ProcessRequest();

	// Wait for end
	double CurrentTime = FPlatformTime::Seconds();
	while (EHttpRequestStatus::Processing == Request->GetStatus())
	{
		const double AppTime = FPlatformTime::Seconds();
		FHttpModule::Get().GetHttpManager().Tick(AppTime - CurrentTime);
		CurrentTime = AppTime;
		FPlatformProcess::Sleep(0.1f);
	}

#endif

	// Report to user
	FPlatformMisc::MessageBoxExt(EAppMsgType::Ok,
		TEXT("An automated report has been sent. Please report this issue yourself with more information."), TEXT("The game has crashed."));

	// Crash
	FPlatformMisc::RequestExit(0);
}

#undef LOCTEXT_NAMESPACE
