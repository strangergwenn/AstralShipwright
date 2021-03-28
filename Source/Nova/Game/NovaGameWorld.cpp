// Nova project - Gwennaël Arbona

#include "NovaGameWorld.h"

#include "NovaArea.h"
#include "NovaAssetCatalog.h"
#include "NovaGameInstance.h"
#include "NovaGameState.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Nova/Tools/NovaActorTools.h"
#include "Nova/Nova.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaGameWorld::ANovaGameWorld()
	: Super(), ServerTime(0), ServerTimeDilation(ENovaTimeDilation::Normal), ClientTime(0), ClientAdditionalTimeDilation(0), PreviousTime(0)
{
	// Setup simulation component
	OrbitalSimulationComponent = CreateDefaultSubobject<UNovaOrbitalSimulationComponent>(TEXT("OrbitalSimulationComponent"));

	// Settings
	bReplicates = true;
	SetReplicatingMovement(false);
	bAlwaysRelevant               = true;
	PrimaryActorTick.bCanEverTick = true;

	// Defaults
	MinimumTimeCorrectionThreshold = 0.25f;
	MaximumTimeCorrectionThreshold = 10.0f;
	TimeCorrectionFactor           = 0.2f;
	TimeDilationManeuverDelay      = 1.0f / 60.0f;
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

	// Process subsystems
	ProcessSpacecraftDatabase();
	ProcessWakeEvents();
	ProcessTime(DeltaTime);
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
			// TODO : should first look into deserialized save data, and then if nothing, fetch the default location from game mode

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

void ANovaGameWorld::SetTimeDilation(ENovaTimeDilation Dilation)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NCHECK(Dilation >= ENovaTimeDilation::Normal && Dilation <= ENovaTimeDilation::Level4);

	ServerTimeDilation = Dilation;
}

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

double ANovaGameWorld::GetPreviousTime() const
{
	return PreviousTime;
}

bool ANovaGameWorld::CanDilateTime(ENovaTimeDilation Dilation) const
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return false;
	}
	else if (Dilation == ENovaTimeDilation::Normal)
	{
		return true;
	}
	else
	{
		return OrbitalSimulationComponent->GetTimeLeftUntilManeuver() >
			   static_cast<double>(TimeDilationManeuverDelay) * GetTimeDilationValue(Dilation);
	}
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

void ANovaGameWorld::ProcessWakeEvents()
{
	if (GetLocalRole() == ROLE_Authority && !CanDilateTime(GetCurrentTimeDilation()))
	{
		DecreaseTimeDilation();
	}
}

void ANovaGameWorld::ProcessTime(float DeltaTime)
{
	double DilatedDeltaTime = static_cast<double>(DeltaTime) * GetCurrentTimeDilationValue() / 60.0;
	if (GetLocalRole() == ROLE_Authority)
	{
		PreviousTime = ServerTime;
		ServerTime += DilatedDeltaTime;
	}
	else
	{
		PreviousTime = ClientTime;
		ClientTime += DilatedDeltaTime * ClientAdditionalTimeDilation;
	}
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
		NLOG("ANovaGameWorld::OnServerTimeReplicated : time rollback from %.2f to %.2f", ClientTime, RealServerTime);
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
