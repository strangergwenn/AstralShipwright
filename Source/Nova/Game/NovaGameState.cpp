// Nova project - Gwennaël Arbona

#include "NovaGameState.h"

#include "NovaArea.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Nova/Actor/NovaActorTools.h"

#include "Nova/Player/NovaPlayerState.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaSessionsManager.h"

#include "Nova/Nova.h"

#include "Net/UnrealNetwork.h"
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
};

TSharedPtr<struct FNovaGameStateSave> ANovaGameState::Save() const
{
	TSharedPtr<FNovaGameStateSave> SaveData = MakeShared<FNovaGameStateSave>();

	return SaveData;
}

void ANovaGameState::Load(TSharedPtr<struct FNovaGameStateSave> SaveData)
{}

void ANovaGameState::SerializeJson(
	TSharedPtr<struct FNovaGameStateSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction)
{}

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
		int64  Cycles      = FPlatformTime::Cycles64();
		double InitialTime = GetCurrentTime();

		// Run FastForwardUpdatesPerFrame loops of world updates
		for (int32 Index = 0; Index < FastForwardUpdatesPerFrame; Index++)
		{
			ProcessSpacecraftDatabase();
			bool ContinueProcessing = ProcessTime(static_cast<double>(FastForwardUpdateTime));
			OrbitalSimulationComponent->UpdateSimulation();

			if (!ContinueProcessing)
			{
				NLOG("ANovaGameState::ProcessTime : fast-forward stopping at %.2f", ServerTime);
				IsFastForward = false;
				break;
			}
		}

		// Check the time jump
		double FastForwardDeltaTime = GetCurrentTime() - InitialTime;
		if (FastForwardDeltaTime > 1)
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
		ProcessSpacecraftDatabase();
		ProcessTime(static_cast<double>(DeltaTime) / 60.0);
		OrbitalSimulationComponent->UpdateSimulation();
	}

	// Update event notification
	ProcessEvents(DeltaTime);

	// Update sessions
	FText                 Unused;
	UNovaSessionsManager* SessionsManager = GetGameInstance<UNovaGameInstance>()->GetSessionsManager();
	SessionsManager->SetSessionAdvertised(IsJoinable(Unused));
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

bool ANovaGameState::IsJoinable(FText& Error) const
{
	bool AllSpacecraftDocked = true;
	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (SpacecraftPawn->GetSpacecraftMovement()->GetState() != ENovaMovementState::Docked)
		{
			AllSpacecraftDocked = false;
			Error               = LOCTEXT("AllSpacecraftNotDocked", "The players you are joining are not docked");
			break;
		}
	}

	return AllSpacecraftDocked;
}

/*----------------------------------------------------
    Spacecraft management
----------------------------------------------------*/

void ANovaGameState::UpdateSpacecraft(const FNovaSpacecraft& Spacecraft, bool IsPlayerSpacecraft)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::UpdateSpacecraft");

	bool IsNew = SpacecraftDatabase.Add(Spacecraft);

	if (IsNew)
	{
		// Attempt orbit merging for player spacecraft joining the game
		bool HasMergedOrbits = false;
		if (IsPlayerSpacecraft)
		{
			ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
			NCHECK(GameState);
			const FNovaOrbit* CurrentOrbit = OrbitalSimulationComponent->GetPlayerOrbit();

			if (CurrentOrbit)
			{
				OrbitalSimulationComponent->MergeOrbit(GameState->GetPlayerSpacecraftIdentifiers(), MakeShared<FNovaOrbit>(*CurrentOrbit));
				HasMergedOrbits = true;
			}
		}

		// Load a default
		if (!HasMergedOrbits)
		{
			// TODO : should first look into de-serialized save data, and then if nothing, fetch the default location from game mode

			const class UNovaArea* StationA = GetGameInstance<UNovaGameInstance>()->GetAssetManager()->GetAsset<UNovaArea>(
				FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
#if 0
			OrbitalSimulationComponent->SetOrbit(
				{Spacecraft.Identifier}, MakeShared<FNovaOrbit>(FNovaOrbitGeometry(StationA->Planet, 300, 200, 0, 360), 0));
#else
			OrbitalSimulationComponent->SetOrbit({Spacecraft.Identifier}, OrbitalSimulationComponent->GetAreaOrbit(StationA));
#endif
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

double ANovaGameState::GetCurrentTime() const
{
	if (GetLocalRole() == ROLE_Authority)
	{
		return ServerTime;
	}
	else
	{
		return ClientTime;
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
		   OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver() > 0;
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

void ANovaGameState::ProcessSpacecraftDatabase()
{
	SpacecraftDatabase.UpdateCache();
	for (FNovaSpacecraft& Spacecraft : SpacecraftDatabase.Get())
	{
		Spacecraft.UpdateIfDirty();
	}
}

bool ANovaGameState::ProcessTime(double DeltaTimeMinutes)
{
	bool         ContinueProcessing = true;
	const double TimeDilation       = GetCurrentTimeDilationValue();

	// Under fast forward, stop on events
	if (IsFastForward && GetLocalRole() == ROLE_Authority)
	{
		const double MaxAllowedDeltaTime = OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver();

		NCHECK(TimeDilation == 1.0);
		NCHECK(MaxAllowedDeltaTime > 0);

		if (DeltaTimeMinutes > MaxAllowedDeltaTime)
		{
			DeltaTimeMinutes   = MaxAllowedDeltaTime;
			ContinueProcessing = false;
		}
	}

	// Update the time
	const double DilatedDeltaTime = TimeDilation * DeltaTimeMinutes;
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

void ANovaGameState::ProcessEvents(float DeltaTime)
{
	FText PrimaryText, SecondaryText;

	TimeSinceEvent += DeltaTime;

	if (TimeSinceEvent > EventNotificationDelay)
	{
		// Handle area changes as the primary information
		if (AreaChangeEvents.Num())
		{
			PrimaryText = FText::FormatNamed(
				LOCTEXT("SharedTransitionAreaFormat", "{area}"), TEXT("area"), AreaChangeEvents[AreaChangeEvents.Num() - 1]->Name);
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
			NCHECK(IsValid(PC) && PC->IsLocalController());
			PC->ShowTitle(PrimaryText, SecondaryText);
		}

		AreaChangeEvents.Empty();
		TimeJumpEvents.Empty();
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

		TimeJumpEvents.Add(RealServerTime - ClientTime);
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
