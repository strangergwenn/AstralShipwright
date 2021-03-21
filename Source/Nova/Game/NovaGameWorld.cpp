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

ANovaGameWorld::ANovaGameWorld() : Super(), ServerTime(0), ServerTimeDilation(1), ClientTime(0), ClientTimeDilation(0)
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

	SpacecraftDatabase.UpdateCache();

	// Process time
	double DilatedDeltaTime = static_cast<double>(DeltaTime * ServerTimeDilation / 60.0);
	if (GetLocalRole() == ROLE_Authority)
	{
		ServerTime += DilatedDeltaTime;
	}
	else
	{
		ClientTime += DilatedDeltaTime * ClientTimeDilation;
	}
}

ANovaGameWorld* ANovaGameWorld::Get(const UObject* Outer)
{
	ANovaGameState* GameState = Outer->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);

	return GameWorld;
}

void ANovaGameWorld::UpdateSpacecraft(const FNovaSpacecraft Spacecraft, bool IsPlayerSpacecraft)
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

void ANovaGameWorld::SetTimeDilation(float Dilation)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NCHECK(Dilation >= 0);

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

/*----------------------------------------------------
    Networking
----------------------------------------------------*/

void ANovaGameWorld::OnServerTimeReplicated()
{
	const APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	NCHECK(PC);

	// Evaluate the current server time
	const double PingSeconds      = UNovaActorTools::GetPlayerLatency(PC);
	const double RealServerTime   = ServerTime + PingSeconds / 60.0;
	const double TimeDeltaSeconds = (RealServerTime - ClientTime) * 60.0 / ServerTimeDilation;

	// We can never go back in time
	NCHECK(TimeDeltaSeconds > -MaximumTimeCorrectionThreshold);

	// Hard correct if the change is large
	if (TimeDeltaSeconds > MaximumTimeCorrectionThreshold)
	{
		NLOG("ANovaGameWorld::OnServerTimeReplicated : time rollback from %.2f to %.2f", ClientTime, RealServerTime);
		ClientTime         = RealServerTime;
		ClientTimeDilation = 1.0;
	}

	// Smooth correct if it isn't
	else
	{
		const float TimeDeltaRatio = FMath::Clamp(
			(TimeDeltaSeconds - MinimumTimeCorrectionThreshold) / (MaximumTimeCorrectionThreshold - MinimumTimeCorrectionThreshold), 0.0,
			1.0);

		ClientTimeDilation = 1.0 + TimeDeltaRatio * TimeCorrectionFactor * FMath::Sign(TimeDeltaSeconds);
	}
}

void ANovaGameWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameWorld, SpacecraftDatabase);
	DOREPLIFETIME(ANovaGameWorld, ServerTime);
	DOREPLIFETIME(ANovaGameWorld, ServerTimeDilation);
};
