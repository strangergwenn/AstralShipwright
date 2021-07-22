// Nova project - Gwennaël Arbona

#include "NovaGameState.h"

#include "NovaArea.h"
#include "NovaGameTypes.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Nova/Actor/NovaActorTools.h"
#include "Nova/Player/NovaPlayerState.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"
#include "Nova/Spacecraft/System/NovaSpacecraftSystemInterface.h"

#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaSessionsManager.h"

#include "Nova/Nova.h"

#include "Net/UnrealNetwork.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "ANovaGameState"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaGameState::ANovaGameState()
	: Super()

	, CurrentArea(nullptr)
	, UseTrajectoryMovement(false)
	, ServerTime(0)
	, ServerTimeDilation(ENovaTimeDilation::Normal)

	, ClientTime(0)
	, ClientAdditionalTimeDilation(0)
	, IsFastForward(false)

	, TimeSinceEvent(0)
{
	// Setup simulation component
	OrbitalSimulationComponent = CreateDefaultSubobject<UNovaOrbitalSimulationComponent>(TEXT("OrbitalSimulationComponent"));

	// Settings
	bReplicates = true;
	SetReplicatingMovement(false);
	bAlwaysRelevant               = true;
	PrimaryActorTick.bCanEverTick = true;

	// General defaults
	MinimumTimeCorrectionThreshold = 0.25f;
	MaximumTimeCorrectionThreshold = 10.0f;
	TimeCorrectionFactor           = 1.0f;

	// Fast forward defaults : 2 days per frame in 2h steps
	FastForwardUpdateTime      = 2 * 60;
	FastForwardUpdatesPerFrame = 24;
	EventNotificationDelay     = 0.5f;
}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

struct FNovaGameStateSave
{
	double           TimeAsMinutes;
	const UNovaArea* CurrentArea;
};

TSharedPtr<struct FNovaGameStateSave> ANovaGameState::Save() const
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	TSharedPtr<FNovaGameStateSave> SaveData = MakeShared<FNovaGameStateSave>();

	// Save time & area
	SaveData->TimeAsMinutes = GetCurrentTime().AsMinutes();
	SaveData->CurrentArea   = GetCurrentArea();

	// Ensure consistency
	NCHECK(SaveData->TimeAsMinutes > 0);
	NCHECK(IsValid(SaveData->CurrentArea));

	return SaveData;
}

void ANovaGameState::Load(TSharedPtr<struct FNovaGameStateSave> SaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::Load");

	// Ensure consistency
	NCHECK(SaveData != nullptr);
	NCHECK(SaveData->TimeAsMinutes >= 0);
	NCHECK(IsValid(SaveData->CurrentArea));

	// Load time & area
	ServerTime = SaveData->TimeAsMinutes;
	SetCurrentArea(SaveData->CurrentArea);
}

void ANovaGameState::SerializeJson(
	TSharedPtr<struct FNovaGameStateSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction)
{
	// Writing to save
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShared<FJsonObject>();

		// Time
		JsonData->SetNumberField("TimeAsMinutes", SaveData->TimeAsMinutes);

		// Area
		UNovaAssetDescription::SaveAsset(JsonData, "CurrentArea", SaveData->CurrentArea);
	}

	// Reading from save
	else
	{
		SaveData = MakeShared<FNovaGameStateSave>();

		// Time
		double Time;
		if (JsonData->TryGetNumberField("TimeAsMinutes", Time))
		{
			SaveData->TimeAsMinutes = Time;
		}

		// Area
		SaveData->CurrentArea = UNovaAssetDescription::LoadAsset<UNovaArea>(JsonData, "CurrentArea");
		if (!IsValid(SaveData->CurrentArea))
		{
			SaveData->CurrentArea = UNovaAssetManager::Get()->GetDefaultAsset<UNovaArea>();
		}
	}
}

/*----------------------------------------------------
    General game state
----------------------------------------------------*/

void ANovaGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Get a player state
	CurrentPlayerState = nullptr;
	for (const ANovaPlayerState* PlayerState : TActorRange<ANovaPlayerState>(GetWorld()))
	{
		if (IsValid(PlayerState) && PlayerState->GetSpacecraftIdentifier().IsValid())
		{
			CurrentPlayerState = PlayerState;
			break;
		}
	}

	// Process fast forward simulation
	if (IsFastForward)
	{
		int64     Cycles      = FPlatformTime::Cycles64();
		FNovaTime InitialTime = GetCurrentTime();

		// Run FastForwardUpdatesPerFrame loops of world updates
		for (int32 Index = 0; Index < FastForwardUpdatesPerFrame; Index++)
		{
			bool ContinueProcessing = ProcessGameSimulation(FNovaTime::FromMinutes(FastForwardUpdateTime));
			if (!ContinueProcessing)
			{
				NLOG("ANovaGameState::ProcessTime : fast-forward stopping at %.2f", ServerTime);
				IsFastForward = false;
				break;
			}
		}

		// Check the time jump
		FNovaTime FastForwardDeltaTime = GetCurrentTime() - InitialTime;
		if (FastForwardDeltaTime > FNovaTime::FromMinutes(1))
		{
			TimeJumpEvents.Add(FastForwardDeltaTime);
			TimeSinceEvent = 0;
		}

		NLOG("ANovaGameState::Tick : processed fast-forward frame in %.2fms",
			FPlatformTime::ToMilliseconds(FPlatformTime::Cycles64() - Cycles));
	}

	// Process real-time simulation
	else
	{
		ProcessGameSimulation(FNovaTime::FromSeconds(static_cast<double>(DeltaTime)));
	}

	// Update event notification
	ProcessPlayerEvents(DeltaTime);

	// Update sessions
	UNovaSessionsManager* SessionsManager = GetGameInstance<UNovaGameInstance>()->GetSessionsManager();
	SessionsManager->SetSessionAdvertised(IsJoinable());
}

void ANovaGameState::SetCurrentArea(const UNovaArea* Area)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	CurrentArea = Area;

	AreaChangeEvents.Add(CurrentArea);
	TimeSinceEvent = 0;
}

FName ANovaGameState::GetCurrentLevelName() const
{
	return CurrentArea ? CurrentArea->LevelName : NAME_None;
}

bool ANovaGameState::IsJoinable(FText* Help) const
{
	bool AllSpacecraftDocked = AreAllSpacecraftDocked();

	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		return true;
	}
	else if (!AllSpacecraftDocked && Help)
	{
		*Help = LOCTEXT("AllSpacecraftNotDocked", "All players need to be docked to allow joining");
	}

	return AllSpacecraftDocked;
}

bool ANovaGameState::IsAnySpacecraftDocked() const
{
	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (SpacecraftPawn->IsDocked())
		{
			return true;
		}
	}

	return false;
}

bool ANovaGameState::AreAllSpacecraftDocked() const
{
	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (!SpacecraftPawn->IsDocked())
		{
			return false;
		}
	}
	return true;
}

bool ANovaGameState::IsResourceSold(const UNovaResource* Asset, const class UNovaArea* Area) const
{
	const UNovaArea* TargetArea = IsValid(Area) ? Area : CurrentArea;

	if (IsValid(TargetArea))
	{
		for (const FNovaResourceTrade& Trade : TargetArea->ResourceTradeMetadata)
		{
			if (Trade.Resource == Asset)
			{
				return Trade.ForSale;
			}
		}
	}

	return false;
}

TArray<const UNovaResource*> ANovaGameState::GetResourcesSold() const
{
	TArray<const UNovaResource*> Result;

	if (IsValid(CurrentArea))
	{
		for (const FNovaResourceTrade& Trade : CurrentArea->ResourceTradeMetadata)
		{
			if (Trade.ForSale)
			{
				Result.Add(Trade.Resource);
			}
		}
	}

	return Result;
}

ENovaPriceModifier ANovaGameState::GetCurrentPriceModifier(const UNovaTradableAssetDescription* Asset) const
{
	if (IsValid(CurrentArea))
	{
		for (const FNovaResourceTrade& Trade : CurrentArea->ResourceTradeMetadata)
		{
			if (Trade.Resource == Asset)
			{
				return Trade.PriceModifier;
			}
		}
	}

	return ENovaPriceModifier::Average;
}

FNovaCredits ANovaGameState::GetCurrentPrice(const UNovaTradableAssetDescription* Asset, bool SpacecraftPartForSale) const
{
	NCHECK(IsValid(Asset));
	float Multiplier = 1.0f;

	// Define price modifiers
	auto GetPriceModifierValue = [](ENovaPriceModifier Modifier) -> double
	{
		switch (Modifier)
		{
			case ENovaPriceModifier::Cheap:
				return 0.75f;
			case ENovaPriceModifier::BelowAverage:
				return 0.9f;
			case ENovaPriceModifier::Average:
				return 1.0f;
			case ENovaPriceModifier::AboveAverage:
				return 1.1f;
			case ENovaPriceModifier::Expensive:
				return 1.25f;
		}

		return 0.0f;
	};

	// Find out the current modifier for this transaction
	Multiplier *= GetPriceModifierValue(GetCurrentPriceModifier(Asset));

	// Non-resource assets have a large depreciation value when re-sold
	if (!Asset->IsA<UNovaResource>() && SpacecraftPartForSale)
	{
		Multiplier *= ENovaConstants::ResaleDepreciation;
	}

	return Multiplier * FNovaCredits(Asset->BasePrice);
}

/*----------------------------------------------------
    Spacecraft management
----------------------------------------------------*/

void ANovaGameState::UpdateSpacecraft(const FNovaSpacecraft& Spacecraft, bool MergeWithPlayer)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::UpdateSpacecraft");

	bool IsNew = SpacecraftDatabase.Add(Spacecraft);

	if (IsNew)
	{
		// Attempt orbit merging for player spacecraft joining the game
		bool WasMergedWithPlayer = false;
		if (MergeWithPlayer)
		{
			ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
			NCHECK(GameState);
			const FNovaOrbit* CurrentOrbit = OrbitalSimulationComponent->GetPlayerOrbit();

			if (CurrentOrbit)
			{
				OrbitalSimulationComponent->MergeOrbit(GameState->GetPlayerSpacecraftIdentifiers(), MakeShared<FNovaOrbit>(*CurrentOrbit));
				WasMergedWithPlayer = true;
			}
		}

		// Load a default
		if (!WasMergedWithPlayer)
		{
			NCHECK(IsValid(CurrentArea));
			OrbitalSimulationComponent->SetOrbit({Spacecraft.Identifier}, OrbitalSimulationComponent->GetAreaOrbit(GetCurrentArea()));
		}
	}
}

FGuid ANovaGameState::GetPlayerSpacecraftIdentifier() const
{
	return CurrentPlayerState ? CurrentPlayerState->GetSpacecraftIdentifier() : FGuid();
}

TArray<FGuid> ANovaGameState::GetPlayerSpacecraftIdentifiers() const
{
	TArray<FGuid> Result;

	for (const ANovaPlayerState* PlayerState : TActorRange<ANovaPlayerState>(GetWorld()))
	{
		if (IsValid(PlayerState))
		{
			Result.Add(PlayerState->GetSpacecraftIdentifier());
		}
	}

	return Result;
}

/*----------------------------------------------------
    Time management
----------------------------------------------------*/

FNovaTime ANovaGameState::GetCurrentTime() const
{
	if (GetLocalRole() == ROLE_Authority)
	{
		return FNovaTime::FromMinutes(ServerTime);
	}
	else
	{
		return FNovaTime::FromMinutes(ClientTime);
	}
}

void ANovaGameState::FastForward()
{
	NLOG("ANovaGameState::FastForward");
	SetTimeDilation(ENovaTimeDilation::Normal);
	IsFastForward = true;
}

bool ANovaGameState::CanFastForward() const
{
	return GetLocalRole() == ROLE_Authority && OrbitalSimulationComponent->GetPlayerTrajectory() != nullptr &&
		   OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver() > FNovaTime();
}

void ANovaGameState::SetTimeDilation(ENovaTimeDilation Dilation)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NCHECK(Dilation >= ENovaTimeDilation::Normal && Dilation <= ENovaTimeDilation::Level3);

	ServerTimeDilation = Dilation;
}

bool ANovaGameState::CanDilateTime(ENovaTimeDilation Dilation) const
{
	return GetLocalRole() == ROLE_Authority;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

bool ANovaGameState::ProcessGameSimulation(FNovaTime DeltaTime)
{
	// Update spacecraft
	SpacecraftDatabase.UpdateCache();
	for (FNovaSpacecraft& Spacecraft : SpacecraftDatabase.Get())
	{
		Spacecraft.UpdatePropulsionMetrics();
	}

	// Update the time with the base delta time that will be affected by time dilation
	FNovaTime InitialTime        = GetCurrentTime();
	bool      ContinueProcessing = ProcessGameTime(DeltaTime);

	// Update the orbital simulation
	OrbitalSimulationComponent->UpdateSimulation();

	// Abort trajectories when a player didn't commit the main drive before 10s
	if (GetLocalRole() == ROLE_Authority)
	{
		ProcessTrajectoryAbort();
	}

	// Update spacecraft systems
	if (GetLocalRole() == ROLE_Authority)
	{
		for (ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
		{
			TArray<UActorComponent*> Components = Pawn->GetComponentsByInterface(UNovaSpacecraftSystemInterface::StaticClass());
			for (UActorComponent* Component : Components)
			{
				INovaSpacecraftSystemInterface* System = Cast<INovaSpacecraftSystemInterface>(Component);
				NCHECK(System);
				System->Update(InitialTime, GetCurrentTime());
			}
		}
	}

	return ContinueProcessing;
}

bool ANovaGameState::ProcessGameTime(FNovaTime DeltaTime)
{
	bool         ContinueProcessing = true;
	const double TimeDilation       = GetCurrentTimeDilationValue();

	// Under fast forward, stop on events
	if (IsFastForward && GetLocalRole() == ROLE_Authority)
	{
		const FNovaTime MaxAllowedDeltaTime = OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver();

		NCHECK(TimeDilation == 1.0);
		NCHECK(MaxAllowedDeltaTime > FNovaTime());

		if (DeltaTime > MaxAllowedDeltaTime)
		{
			DeltaTime          = MaxAllowedDeltaTime;
			ContinueProcessing = false;
		}
	}

	// Update the time
	const double DilatedDeltaTime = TimeDilation * DeltaTime.AsMinutes();
	if (GetLocalRole() == ROLE_Authority)
	{
		ServerTime += DilatedDeltaTime;
	}
	else
	{
		ClientTime += DilatedDeltaTime * ClientAdditionalTimeDilation;
	}

	return ContinueProcessing;
}

void ANovaGameState::ProcessPlayerEvents(float DeltaTime)
{
	FText PrimaryText, SecondaryText;

	TimeSinceEvent += DeltaTime;

	if (TimeSinceEvent > EventNotificationDelay)
	{
		// Handle area changes as the primary information
		if (AreaChangeEvents.Num())
		{
			PrimaryText = AreaChangeEvents[AreaChangeEvents.Num() - 1]->Name;
		}

		// Handle time skips as the secondary information
		if (TimeJumpEvents.Num())
		{
			FText& Text = PrimaryText.IsEmpty() ? PrimaryText : SecondaryText;

			Text = FText::FormatNamed(LOCTEXT("SharedTransitionTimeFormat", "{duration} have passed"), TEXT("duration"),
				GetDurationText(TimeJumpEvents[TimeJumpEvents.Num() - 1], 1));
		}

		if (!PrimaryText.IsEmpty())
		{
			ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
			if (IsValid(PC) && PC->IsLocalController())
			{
				PC->Notify(PrimaryText, SecondaryText, ENovaNotificationType::World);
			}
		}

		AreaChangeEvents.Empty();
		TimeJumpEvents.Empty();
	}
}

void ANovaGameState::ProcessTrajectoryAbort()
{
	// Check for a main drive on all players
	bool InvalidTrajectory = false;
	for (const ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (!Pawn->GetSpacecraftMovement()->IsMainDriveEnabled())
		{
			InvalidTrajectory = true;
			break;
		}
	}

	// Invalidate the trajectory if a player doesn't have a powered main drive
	const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
	if (InvalidTrajectory && PlayerTrajectory && PlayerTrajectory->GetCurrentManeuver(GetCurrentTime()) == nullptr &&
		(PlayerTrajectory->GetNextManeuverStartTime(GetCurrentTime()) - GetCurrentTime()).AsSeconds() < 10)
	{
		NLOG("ANovaGameState::ProcessTrajectoryAbort : aborting trajectory");

		OrbitalSimulationComponent->CompleteTrajectory(GetPlayerSpacecraftIdentifiers());

		for (ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
		{
			if (Pawn->GetSpacecraftMovement()->IsMainDriveEnabled())
			{
				Pawn->GetSpacecraftMovement()->DisableMainDrive();
			}
		}

		ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		PC->Notify(LOCTEXT("TrajectoryAborted", "Trajectory aborted"),
			PlayerArray.Num() > 1 ? LOCTEXT("PlayerNotReady", "Spacecraft not authorized to maneuver")
								  : LOCTEXT("NotAllPlayersReady", "A spacecraft wasn't authorized to maneuver"),
			ENovaNotificationType::Error);

		return;
	}

	// Disable main drive as soon as the maneuver is ongoing
	if (PlayerTrajectory && PlayerTrajectory->GetCurrentManeuver(GetCurrentTime()) != nullptr)
	{
		for (ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
		{
			if (Pawn->GetSpacecraftMovement()->IsMainDriveEnabled())
			{
				Pawn->GetSpacecraftMovement()->DisableMainDrive();
			}
		}
	}
}

void ANovaGameState::OnServerTimeReplicated()
{
	const APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	NCHECK(IsValid(PC) && PC->IsLocalController());

	// Evaluate the current server time
	const double PingSeconds      = UNovaActorTools::GetPlayerLatency(PC);
	const double RealServerTime   = ServerTime + PingSeconds / 60.0;
	const double TimeDeltaSeconds = (RealServerTime - ClientTime) * 60.0 / GetCurrentTimeDilationValue();

	// We can never go back in time
	NCHECK(TimeDeltaSeconds > -MaximumTimeCorrectionThreshold);

	// Hard correct if the change is large
	if (TimeDeltaSeconds > MaximumTimeCorrectionThreshold)
	{
		NLOG("ANovaGameState::OnServerTimeReplicated : time jump from %.2f to %.2f", ClientTime, RealServerTime);

		TimeJumpEvents.Add(FNovaTime::FromMinutes(RealServerTime - ClientTime));
		TimeSinceEvent = 0;

		ClientTime                   = RealServerTime;
		ClientAdditionalTimeDilation = 1.0;
	}

	// Smooth correct if it isn't
	else
	{
		const float TimeDeltaRatio = FMath::Clamp(
			(TimeDeltaSeconds - MinimumTimeCorrectionThreshold) / (MaximumTimeCorrectionThreshold - MinimumTimeCorrectionThreshold), 0.0,
			1.0);

		ClientAdditionalTimeDilation = 1.0 + TimeDeltaRatio * TimeCorrectionFactor * FMath::Sign(TimeDeltaSeconds);
	}
}

void ANovaGameState::OnCurrentAreaReplicated()
{
	NLOG("ANovaGameState::OnCurrentAreaReplicated");

	AreaChangeEvents.Add(CurrentArea);
	TimeSinceEvent = 0;
}

void ANovaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameState, CurrentArea);

	DOREPLIFETIME(ANovaGameState, SpacecraftDatabase);
	DOREPLIFETIME(ANovaGameState, UseTrajectoryMovement);
	DOREPLIFETIME(ANovaGameState, ServerTime);
	DOREPLIFETIME(ANovaGameState, ServerTimeDilation);
}

#undef LOCTEXT_NAMESPACE
