// Astral Shipwright - Gwennaël Arbona

#include "Nova.h"
#include "Game/NovaGameTypes.h"

#include "Engine.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FNovaModule, AstralShipwright, "AstralShipwright");

#define LOCTEXT_NAMESPACE "AstralShipwright"

/*----------------------------------------------------
    Debugging tools
----------------------------------------------------*/

FText GetDateText(struct FNovaTime Time)
{
	FDateTime DateTime = FDateTime(ENovaConstants::ZeroTime) + FTimespan(Time.AsMinutes() * ETimespan::TicksPerMinute);

	return FText::AsDateTime(DateTime);
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
	if ((Seconds && ComponentCount < MaxComponents) || ComponentCount == 0)
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

FText GetPriceText(FNovaCredits Credits)
{
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;

	return FText::FromString(FText::AsCurrency(Credits.GetValue(), TEXT("CUR"), &Options).ToString().Replace(TEXT("CUR"), TEXT("Ѥ")));
}

#undef LOCTEXT_NAMESPACE
