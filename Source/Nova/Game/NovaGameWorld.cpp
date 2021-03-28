// Nova project - Gwennaël Arbona

#include "NovaGameWorld.h"

#include "NovaArea.h"
#include "NovaAssetCatalog.h"
#include "NovaGameInstance.h"
#include "NovaGameState.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaPlayerState.h"
#include "Nova/Tools/NovaActorTools.h"
#include "Nova/Nova.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaGameWorld::ANovaGameWorld()
	: Super()
	, ServerTime(0)
	, ServerTimeDilation(ENovaTimeDilation::Normal)
	, ClientTime(0)
	, ClientAdditionalTimeDilation(0)
	, IsFastForward(false)
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
	TimeCorrectionFactor           = 0.2f;
	TimeMarginBeforeManeuver       = 1.0f;

	// Fast forward defaults : 2 days per frame in 2h steps
	FastForwardUpdateTime      = 2 * 60;
	FastForwardUpdatesPerFrame = 24;
}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

struct FNovaWorldSave
{
};

TSharedPtr<struct FNovaWorldSave> ANovaGameWorld::Save() const
{
	TSharedPtr<FNovaWorldSave> SaveData = MakeShared<FNovaWorldSave>();

	return SaveData;
}

void ANovaGameWorld::Load(TSharedPtr<struct FNovaWorldSave> SaveData)
{}

void ANovaGameWorld::SerializeJson(
	TSharedPtr<struct FNovaWorldSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction)
{}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaGameWorld::Tick(float DeltaTime)
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
		int64 Cycles = FPlatformTime::Cycles64();

		for (int32 Index = 0; Index < FastForwardUpdatesPerFrame; Index++)
		{
			ProcessSpacecraftDatabase();
			bool ContinueProcessing = ProcessTime(static_cast<double>(FastForwardUpdateTime));
			OrbitalSimulationComponent->UpdateSimulation();

			if (!ContinueProcessing)
			{
				NLOG("ANovaGameWorld::ProcessTime : fast-forward stopping at %.2f", ServerTime);
				IsFastForward = false;
				break;
			}
		}

		NLOG("ANovaGameWorld::Tick : processed fast-forward frame in %.2fms",
			FPlatformTime::ToMilliseconds(FPlatformTime::Cycles64() - Cycles));
	}

	// Process real-time simulation
	else
	{
		ProcessSpacecraftDatabase();
		ProcessTime(static_cast<double>(DeltaTime) / 60.0);
		OrbitalSimulationComponent->UpdateSimulation();
	}
}

ANovaGameWorld* ANovaGameWorld::Get(const UObject* Outer)
{
	ANovaGameState* GameState = Outer->GetWorld()->GetGameState<ANovaGameState>();
	if (IsValid(GameState))
	{
		ANovaGameWorld* GameWorld = GameState->GetGameWorld();
		if (IsValid(GameWorld))
		{
			return GameWorld;
		}
	}

	return nullptr;
}

void ANovaGameWorld::UpdateSpacecraft(const FNovaSpacecraft& Spacecraft, bool IsPlayerSpacecraft)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameWorld::UpdateSpacecraft");

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

			const class UNovaArea* StationA =
				GetGameInstance<UNovaGameInstance>()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
#if 0
			OrbitalSimulationComponent->SetOrbit(
				{Spacecraft.Identifier}, MakeShared<FNovaOrbit>(FNovaOrbitGeometry(StationA->Planet, 300, 200, 0, 360), 0));
#else
			OrbitalSimulationComponent->SetOrbit({Spacecraft.Identifier}, OrbitalSimulationComponent->GetAreaOrbit(StationA));
#endif
		}
	}
}

FGuid ANovaGameWorld::GetPlayerSpacecraftIdentifier() const
{
	return CurrentPlayerState ? CurrentPlayerState->GetSpacecraftIdentifier() : FGuid();
}

TArray<FGuid> ANovaGameWorld::GetPlayerSpacecraftIdentifiers() const
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

double ANovaGameWorld::GetCurrentTime() const
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

void ANovaGameWorld::FastForward()
{
	SetTimeDilation(ENovaTimeDilation::Normal);
	IsFastForward = true;
}

bool ANovaGameWorld::CanFastForward() const
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return false;
	}
	else if (OrbitalSimulationComponent->GetPlayerTrajectory() == nullptr)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void ANovaGameWorld::SetTimeDilation(ENovaTimeDilation Dilation)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NCHECK(Dilation >= ENovaTimeDilation::Normal && Dilation <= ENovaTimeDilation::Level3);

	ServerTimeDilation = Dilation;
}

bool ANovaGameWorld::CanDilateTime(ENovaTimeDilation Dilation) const
{
	return GetLocalRole() == ROLE_Authority;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaGameWorld::ProcessSpacecraftDatabase()
{
	SpacecraftDatabase.UpdateCache();
	for (FNovaSpacecraft& Spacecraft : SpacecraftDatabase.Get())
	{
		Spacecraft.UpdateIfDirty();
	}
}

bool ANovaGameWorld::ProcessTime(double DeltaTimeMinutes)
{
	bool         ContinueProcessing = true;
	const double TimeDilation       = GetCurrentTimeDilationValue();

	// Under fast forward, stop on events
	if (IsFastForward && GetLocalRole() == ROLE_Authority)
	{
		const double MaxAllowedDeltaTime =
			OrbitalSimulationComponent->GetTimeOfNextPlayerManeuver() - GetCurrentTime() - TimeMarginBeforeManeuver;

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

void ANovaGameWorld::OnServerTimeReplicated()
{
	const APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	NCHECK(PC);

	// Evaluate the current server time
	const double PingSeconds      = UNovaActorTools::GetPlayerLatency(PC);
	const double RealServerTime   = ServerTime + PingSeconds / 60.0;
	const double TimeDeltaSeconds = (RealServerTime - ClientTime) * 60.0 / GetCurrentTimeDilationValue();

	// We can never go back in time
	NCHECK(TimeDeltaSeconds > -MaximumTimeCorrectionThreshold);

	// Hard correct if the change is large
	if (TimeDeltaSeconds > MaximumTimeCorrectionThreshold)
	{
		NLOG("ANovaGameWorld::OnServerTimeReplicated : time jump from %.2f to %.2f", ClientTime, RealServerTime);
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

void ANovaGameWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameWorld, SpacecraftDatabase);
	DOREPLIFETIME(ANovaGameWorld, ServerTime);
	DOREPLIFETIME(ANovaGameWorld, ServerTimeDilation);
};
