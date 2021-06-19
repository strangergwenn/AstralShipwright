// Nova project - Gwennaël Arbona

#include "Nova.h"
#include "Game/NovaGameTypes.h"
#include "UI/Style/NovaStyleSet.h"

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

FText GetDurationText(FNovaTime Time, int32 MaxComponents)
{
	FString Result;

	double SourceMinutes = Time.AsMinutes();
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

FText GetPriceText(double Credits)
{
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;

	return FText::FromString(FText::AsCurrency(Credits, TEXT("CUR"), &Options).ToString().Replace(TEXT("CUR"), TEXT("Ѥ")));
}

/*----------------------------------------------------
    Module code
----------------------------------------------------*/

void FNovaModule::DisplayLog(FString Log)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, Log);
	UE_LOG(LogNova, Display, TEXT("%s"), *Log);
}

#undef LOCTEXT_NAMESPACE
