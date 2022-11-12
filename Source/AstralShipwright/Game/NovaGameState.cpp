// Astral Shipwright - Gwennaël Arbona

#include "NovaGameState.h"

#include "NovaArea.h"
#include "NovaGameMode.h"
#include "NovaGameTypes.h"
#include "NovaSaveData.h"
#include "NovaAISimulationComponent.h"
#include "NovaAsteroidSimulationComponent.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Player/NovaPlayerState.h"
#include "Player/NovaPlayerController.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"
#include "Spacecraft/System/NovaSpacecraftSystemInterface.h"
#include "Spacecraft/System/NovaSpacecraftProcessingSystem.h"

#include "Neutron/Actor/NeutronActorTools.h"
#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronSessionsManager.h"

#include "Nova.h"

#include "Engine/LevelStreaming.h"
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
	, ServerTime(0)
	, ServerTimeDilation(ENovaTimeDilation::Normal)

	, CurrentPriceRotation(1)

	, ClientTime(0)
	, ClientAdditionalTimeDilation(0)
	, IsFastForward(false)
	, TimeSinceLastFastForward(0)

	, TimeSinceEvent(0)
{
	// Setup simulation component
	OrbitalSimulationComponent  = CreateDefaultSubobject<UNovaOrbitalSimulationComponent>(TEXT("OrbitalSimulationComponent"));
	AsteroidSimulationComponent = CreateDefaultSubobject<UNovaAsteroidSimulationComponent>(TEXT("AsteroidSimulationComponent"));
	AISimulationComponent       = CreateDefaultSubobject<UNovaAISimulationComponent>(TEXT("AISimulationComponent"));

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
	FastForwardDelay           = 0.5;

	// Time defaults
	EventNotificationDelay     = 0.5f;
	TrajectoryEarlyRequirement = 5.0;
}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

FNovaGameStateSave ANovaGameState::Save() const
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	FNovaGameStateSave SaveData;

	// Save general state
	SaveData.CurrentArea          = GetCurrentArea();
	SaveData.Time                 = GetCurrentTime();
	SaveData.CurrentPriceRotation = CurrentPriceRotation;
	SaveData.TimeOfLastRotation   = TimeOfLastRotation;

	// Save AI
	SaveData.AIData = AISimulationComponent->Save();

	// Ensure consistency
	NCHECK(SaveData.Time > 0);
	NCHECK(IsValid(SaveData.CurrentArea));

	return SaveData;
}

void ANovaGameState::Load(const FNovaGameStateSave& SaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::Load");

	// Ensure consistency
	NCHECK(SaveData.Time >= 0);

	// Default area
	if (IsValid(SaveData.CurrentArea) && IsValid(SaveData.CurrentArea->Body))
	{
		SetCurrentArea(SaveData.CurrentArea);
	}
	else
	{
		SetCurrentArea(UNeutronAssetManager::Get()->GetDefaultAsset<UNovaArea>());
	}

	// Save general state
	ServerTime           = SaveData.Time.AsMinutes();
	CurrentPriceRotation = SaveData.CurrentPriceRotation;
	TimeOfLastRotation   = SaveData.TimeOfLastRotation;

	// Load AI
	AISimulationComponent->Load(SaveData.AIData);
}

/*----------------------------------------------------
    General game state
----------------------------------------------------*/

void ANovaGameState::BeginPlay()
{
	Super::BeginPlay();

	// Startup the simulation components
	AsteroidSimulationComponent->Initialize(UNeutronAssetManager::Get()->GetDefaultAsset<UNovaAsteroidConfiguration>());
}

void ANovaGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Process fast forward simulation
	if (IsFastForward)
	{
		int64     Cycles         = FPlatformTime::Cycles64();
		FNovaTime InitialTime    = GetCurrentTime();
		TimeSinceLastFastForward = 0;

		// Run FastForwardUpdatesPerFrame loops of world updates
		ENovaSimulationDecision Decision = ENovaSimulationDecision::Continue;
		for (int32 Index = 0; Index < FastForwardUpdatesPerFrame; Index++)
		{
			Decision = ProcessGameSimulation(FNovaTime::FromMinutes(FastForwardUpdateTime), Decision);
			if (Decision == ENovaSimulationDecision::AbortImmediately)
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
		ProcessGameSimulation(FNovaTime::FromSeconds(static_cast<double>(DeltaTime)), ENovaSimulationDecision::Continue);

		TimeSinceLastFastForward += DeltaTime;
	}

	// Update event notification
	ProcessPlayerEvents(DeltaTime);

	// Update sessions
	UNeutronSessionsManager* SessionsManager = UNeutronSessionsManager::Get();
	SessionsManager->SetSessionAdvertised(IsJoinable());
}

void ANovaGameState::SetCurrentArea(const UNovaArea* Area)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	CurrentArea = Area;

	AreaChangeEvents.Add(CurrentArea);
	TimeSinceEvent = 0;
}

void ANovaGameState::RotatePrices()
{
	CurrentPriceRotation++;

	NLOG("ANovaGameState::RotatePrices (now %d)", CurrentPriceRotation);
}

FName ANovaGameState::GetCurrentLevelName() const
{
	return CurrentArea ? CurrentArea->LevelName : NAME_None;
}

bool ANovaGameState::IsLevelStreamingComplete() const
{
	for (const ULevelStreaming* Level : GetWorld()->GetStreamingLevels())
	{
		if (Level->IsStreamingStatePending())
		{
			return false;
		}
		else if (Level->IsLevelLoaded())
		{
			FString LoadedLevelName = Level->GetWorldAssetPackageFName().ToString();
			return LoadedLevelName.EndsWith(GetCurrentLevelName().ToString());
		}
	}

	return true;
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

/*----------------------------------------------------
    Resources
----------------------------------------------------*/

bool ANovaGameState::IsResourceSold(const UNovaResource* Resource, const class UNovaArea* Area) const
{
	const UNovaArea* TargetArea = IsValid(Area) ? Area : CurrentArea;
	if (IsValid(TargetArea))
	{
		return TargetArea->IsResourceSold(Resource);
	}

	return false;
}

TArray<const UNovaResource*> ANovaGameState::GetResourcesBought(const class UNovaArea* Area) const
{
	const UNovaArea* TargetArea = IsValid(Area) ? Area : CurrentArea;
	if (IsValid(TargetArea))
	{
		return TargetArea->GetResourcesBought();
	}

	return {};
}

TArray<const UNovaResource*> ANovaGameState::GetResourcesSold(const class UNovaArea* Area) const
{
	const UNovaArea* TargetArea = IsValid(Area) ? Area : CurrentArea;
	if (IsValid(TargetArea))
	{
		return TargetArea->GetResourcesSold();
	}

	return {};
}

ENovaPriceModifier ANovaGameState::GetCurrentPriceModifier(const UNovaTradableAssetDescription* Asset, const UNovaArea* Area) const
{
	NCHECK(Area);

	// Rotate pricing without affecting the current area since the player came here for a reason
	auto RotatePrice = [Area, this](ENovaPriceModifier Input, bool IsForSale)
	{
		uint8 Result = static_cast<uint8>(Input);

		// Handle rotation
		const int32 PriceRotation = CurrentPriceRotation % 5;
		switch (PriceRotation)
		{
			// Initial situation
			case 0:
				break;

			// High selling price, high buying price
			case 1:
				Result += 1;
				break;

			// Very high selling price, high buying price
			case 2:
				if (IsForSale)
				{
					Result += 2;
				}
				else
				{
					Result += 1;
				}
				break;

			// High selling price, low buying price
			case 3:
				if (IsForSale)
				{
					Result += 1;
				}
				else
				{
					Result -= 1;
				}
				break;

			// Low selling price, low buying price
			case 4:
				Result -= 1;
				break;
		}

		return static_cast<ENovaPriceModifier>(FMath::Clamp<uint8>(
			Result, static_cast<uint8>(ENovaPriceModifier::VeryCheap), static_cast<uint8>(ENovaPriceModifier::VeryExpensive)));
	};

	// Find the relevant trade metadata if any, indicating a sale
	if (IsValid(Area))
	{
		for (const FNovaResourceTrade& Trade : Area->ResourceTradeMetadata)
		{
			if (Trade.Resource == Asset)
			{
				//NLOG("ANovaGameState::GetCurrentPriceModifier : '%s' was %d", *Asset->Name.ToString(),
				//	static_cast<uint8>(Trade.PriceModifier));
				return RotatePrice(Trade.PriceModifier, Trade.ForSale);
			}
		}
	}

	// Default to average price and assume a sale
	//NLOG("ANovaGameState::GetCurrentPriceModifier : '%s' was %d (average)", *Asset->Name.ToString(),
	//	static_cast<uint8>(ENovaPriceModifier::Average));
	return RotatePrice(ENovaPriceModifier::Average, false);
}

FNovaCredits ANovaGameState::GetCurrentPrice(
	const UNovaTradableAssetDescription* Asset, const UNovaArea* Area, bool SpacecraftPartForSale) const
{
	NCHECK(IsValid(Asset));
	float Multiplier = 1.0f;

	// Resources have a price rotation
	if (Asset->IsA<UNovaResource>())
	{
		// Define price modifiers
		auto GetPriceModifierValue = [](ENovaPriceModifier Modifier) -> double
		{
			switch (Modifier)
			{
				case ENovaPriceModifier::VeryCheap:
					return 0.5f;
				case ENovaPriceModifier::Cheap:
					return 0.7f;
				case ENovaPriceModifier::BelowAverage:
					return 0.85f;
				case ENovaPriceModifier::Average:
					return 1.0f;
				case ENovaPriceModifier::AboveAverage:
					return 1.15f;
				case ENovaPriceModifier::Expensive:
					return 1.3f;
				case ENovaPriceModifier::VeryExpensive:
					return 1.5f;
			}

			return 0.0f;
		};

		// Find out the current modifier for this transaction
		Multiplier *= GetPriceModifierValue(GetCurrentPriceModifier(Asset, Area));
	}

	// Non-resource assets have a large depreciation value when re-sold
	else if (SpacecraftPartForSale)
	{
		Multiplier *= ENovaConstants::ResaleDepreciation;
	}

	return Multiplier * FNovaCredits(Asset->BasePrice);
}

/*----------------------------------------------------
    Spacecraft management
----------------------------------------------------*/

void ANovaGameState::UpdatePlayerSpacecraft(const FNovaSpacecraft& Spacecraft, bool MergeWithPlayer)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::UpdatePlayerSpacecraft");

	PlayerSpacecraftIdentifiers.AddUnique(Spacecraft.Identifier);

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
				OrbitalSimulationComponent->MergeOrbit(GameState->GetPlayerSpacecraftIdentifiers(), *CurrentOrbit);
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

void ANovaGameState::UpdateSpacecraft(const FNovaSpacecraft& Spacecraft, const FNovaOrbit* Orbit)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::UpdateSpacecraft");

	bool IsNew = SpacecraftDatabase.Add(Spacecraft);
	if (IsNew && Orbit != nullptr && Orbit->IsValid())
	{
		OrbitalSimulationComponent->SetOrbit({Spacecraft.Identifier}, *Orbit);
	}
}

bool ANovaGameState::IsAnySpacecraftOperating() const
{
	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		const UNovaSpacecraftMovementComponent* MovementComponent = SpacecraftPawn->GetSpacecraftMovement();
		if (SpacecraftPawn->GetPlayerState() && IsValid(MovementComponent) &&
			(MovementComponent->IsAnchoring() || MovementComponent->IsAnchored() || MovementComponent->IsOrbiting()))
		{
			return true;
		}
	}

	return false;
}

bool ANovaGameState::IsAnySpacecraftDocked() const
{
	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (SpacecraftPawn->GetPlayerState() && SpacecraftPawn->IsDocked())
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
		if (SpacecraftPawn->GetPlayerState() && !SpacecraftPawn->IsDocked())
		{
			return false;
		}
	}

	return true;
}

UActorComponent* ANovaGameState::GetSpacecraftSystem(
	const struct FNovaSpacecraft* Spacecraft, const TSubclassOf<UActorComponent> ComponentClass) const
{
	NCHECK(Spacecraft);

	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (::IsValid(SpacecraftPawn) && SpacecraftPawn->GetSpacecraftIdentifier() == Spacecraft->Identifier)
		{
			return SpacecraftPawn->FindComponentByClass(ComponentClass);
		}
	}

	return nullptr;
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

bool ANovaGameState::CanFastForward(FText* AbortReason) const
{
	if (GetLocalRole() == ROLE_Authority && !IsFastForward)
	{
		// No event upcoming
		if (GetAllowedFastFowardTime() > FNovaTime::FromDays(ENovaConstants::MaxTrajectoryDurationDays) ||
			GetAllowedFastFowardTime() <= FNovaTime())
		{
			if (AbortReason)
			{
				*AbortReason = LOCTEXT("NoEvent", "No upcoming event");
			}
			return false;
		}

		// Blocked production
		for (const ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
		{
			if (Pawn->GetPlayerState())
			{
				const UNovaSpacecraftProcessingSystem* ProcessingSystem = Pawn->FindComponentByClass<UNovaSpacecraftProcessingSystem>();
				for (int32 GroupIndex = 0; GroupIndex < ProcessingSystem->GetProcessingGroupCount(); GroupIndex++)
				{
					if (ProcessingSystem->GetProcessingGroupStatus(GroupIndex).Contains(ENovaSpacecraftProcessingSystemStatus::Blocked))
					{
						if (AbortReason)
						{
							*AbortReason = LOCTEXT("BlockedProduction", "At least one module group has blocked resource processing");
						}
						return false;
					}
				}
			}
		}

		// Maneuvering
		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
		if (PlayerTrajectory && PlayerTrajectory->GetManeuver(GetCurrentTime()))
		{
			if (AbortReason)
			{
				*AbortReason = LOCTEXT("Maneuvering", "A maneuver is ongoing");
			}
			return false;
		}

		// Check trajectory issues
		return PlayerTrajectory == nullptr || CheckTrajectoryAbort(AbortReason) == ENovaTrajectoryAction::Continue;
	}
	else
	{
		return false;
	}
}

FNovaTime ANovaGameState::GetAllowedFastFowardTime() const
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	const ANovaGameMode* GameMode = GetWorld()->GetAuthGameMode<ANovaGameMode>();
	NCHECK(IsValid(GameMode));

	// Handle trajectories
	FNovaTime MaxAllowedDeltaTime           = OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver(GameMode->GetManeuverWarnTime());
	const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
	if (PlayerTrajectory)
	{
		const FNovaTime TimeBeforeArrival = PlayerTrajectory->GetArrivalTime() - GetCurrentTime() - GameMode->GetArrivalWarningTime();

		MaxAllowedDeltaTime = FMath::Min(MaxAllowedDeltaTime, TimeBeforeArrival);
	}

	// Handle production remaining time
	for (const ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (Pawn->GetPlayerState())
		{
			const UNovaSpacecraftProcessingSystem* ProcessingSystem = Pawn->FindComponentByClass<UNovaSpacecraftProcessingSystem>();

			FNovaTime RemainingProductionTime = ProcessingSystem->GetRemainingProductionTime();
			MaxAllowedDeltaTime               = FMath::Min(MaxAllowedDeltaTime, RemainingProductionTime);
		}
	}

	return MaxAllowedDeltaTime;
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

ENovaSimulationDecision ANovaGameState::ProcessGameSimulation(FNovaTime DeltaTime, ENovaSimulationDecision PreviousDecision)
{
	// Update spacecraft
	SpacecraftDatabase.UpdateCache();
	for (FNovaSpacecraft& Spacecraft : SpacecraftDatabase.Get())
	{
		Spacecraft.UpdatePropulsionMetrics();
		Spacecraft.UpdatePowerMetrics();
	}

	// Update the time with the base delta time that will be affected by time dilation
	FNovaTime               InitialTime = GetCurrentTime();
	ENovaSimulationDecision Decision    = ProcessGameTime(DeltaTime, PreviousDecision);

	// Update the orbital simulation
	OrbitalSimulationComponent->UpdateSimulation();

	if (GetLocalRole() == ROLE_Authority)
	{
		// Abort trajectories when a player didn't commit in time
		ProcessTrajectoryAbort();

		// Update spacecraft systems
		for (ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
		{
			if (Pawn->GetPlayerState() != nullptr)
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

		// Update prices if necessary
		if (GetTimeLeftUntilPriceRotation() <= 0)
		{
			RotatePrices();
			TimeOfLastRotation = GetCurrentTime();
		}
	}

	// Abort after simulation
	if (Decision == ENovaSimulationDecision::AbortAfterSimulation)
	{
		Decision = ENovaSimulationDecision::AbortImmediately;
	}

	return Decision;
}

ENovaSimulationDecision ANovaGameState::ProcessGameTime(FNovaTime DeltaTime, ENovaSimulationDecision PreviousDecision)
{
	ENovaSimulationDecision Decision     = ENovaSimulationDecision::Continue;
	const double            TimeDilation = GetCurrentTimeDilationValue();

	// Under fast forward, stop on events
	if (IsFastForward && GetLocalRole() == ROLE_Authority)
	{
		// Check for upcoming events
		FNovaTime MaxAllowedDeltaTime = GetAllowedFastFowardTime();
		NCHECK(TimeDilation == 1.0);
		if (MaxAllowedDeltaTime <= FNovaTime())
		{
			NLOG("ANovaGameState::ProcessGameTime : no delta, stopping processing");

			DeltaTime = FNovaTime();
			Decision  = ENovaSimulationDecision::AbortImmediately;
		}

		// Break simulation if we have an event soon
		else if (DeltaTime > MaxAllowedDeltaTime)
		{
			NLOG("ANovaGameState::ProcessGameTime : delta too large at %.2f, cutting (%.2f to %.2f)", ServerTime, DeltaTime.AsMinutes(),
				MaxAllowedDeltaTime.AsMinutes());

			DeltaTime = MaxAllowedDeltaTime;
			Decision  = PreviousDecision == ENovaSimulationDecision::Continue ? ENovaSimulationDecision::ContinueOneStep
			                                                                  : ENovaSimulationDecision::AbortAfterSimulation;
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

	return Decision;
}

void ANovaGameState::ProcessPlayerEvents(float DeltaTime)
{
	FText PrimaryText, SecondaryText;

	TimeSinceEvent += DeltaTime;

	if (TimeSinceEvent > EventNotificationDelay)
	{
		ENeutronNotificationType NotificationType = ENeutronNotificationType::Info;

		// Handle area changes as the primary information
		if (AreaChangeEvents.Num())
		{
			PrimaryText      = AreaChangeEvents[AreaChangeEvents.Num() - 1]->Name;
			NotificationType = ENeutronNotificationType::World;
		}

		// Handle time skips as the secondary information
		if (TimeJumpEvents.Num())
		{
			FText& Text = PrimaryText.IsEmpty() ? PrimaryText : SecondaryText;

			if (PrimaryText.IsEmpty())
			{
				NotificationType = ENeutronNotificationType::Time;
			}

			Text = FText::FormatNamed(LOCTEXT("SharedTransitionTimeFormat", "{duration} have passed"), TEXT("duration"),
				GetDurationText(TimeJumpEvents[TimeJumpEvents.Num() - 1], 1));
		}

		if (!PrimaryText.IsEmpty())
		{
			ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
			if (IsValid(PC) && PC->IsLocalController())
			{
				PC->Notify(PrimaryText, SecondaryText, NotificationType);
			}
		}

		AreaChangeEvents.Empty();
		TimeJumpEvents.Empty();
	}
}

void ANovaGameState::ProcessTrajectoryAbort()
{
	const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
	if (PlayerTrajectory)
	{
		// Check all spacecraft for issues
		FText                 AbortReason;
		ENovaTrajectoryAction TrajectoryState = CheckTrajectoryAbort(&AbortReason);

		// Check whether the trajectory is less than a few seconds away from starting
		bool IsTrajectoryStarted =
			PlayerTrajectory && PlayerTrajectory->GetManeuver(GetCurrentTime()) == nullptr &&
			(PlayerTrajectory->GetNextManeuverStartTime(GetCurrentTime()) - GetCurrentTime()).AsSeconds() < TrajectoryEarlyRequirement;

		// Invalidate the trajectory if a player doesn't match conditions
		if (TrajectoryState == ENovaTrajectoryAction::AbortImmediately ||
			(TrajectoryState == ENovaTrajectoryAction::AbortIfStarted && IsTrajectoryStarted))
		{
			NLOG("ANovaGameState::ProcessTrajectoryAbort : aborting trajectory");

			OrbitalSimulationComponent->AbortTrajectory(GetPlayerSpacecraftIdentifiers());
			ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
			PC->Notify(LOCTEXT("TrajectoryAborted", "Trajectory aborted"), AbortReason, ENeutronNotificationType::Error);

			return;
		}
	}
}

ENovaTrajectoryAction ANovaGameState::CheckTrajectoryAbort(FText* AbortReason) const
{
	for (const ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (Pawn->GetPlayerState())
		{
			// Docking or undocking
			if (Pawn->GetSpacecraftMovement()->IsDockingUndocking() || Pawn->GetSpacecraftMovement()->IsDocked())
			{
				if (AbortReason)
				{
					*AbortReason =
						FText::FormatNamed(LOCTEXT("SpacecraftDocking", "{spacecraft}|plural(one=The,other=A) spacecraft is docking"),
							TEXT("spacecraft"), PlayerArray.Num());
				}

				return ENovaTrajectoryAction::AbortImmediately;
			}

			// Maneuvers not cleared by player
			else if (!Pawn->GetSpacecraftMovement()->CanManeuver())
			{
				if (AbortReason)
				{
					*AbortReason = FText::FormatNamed(
						LOCTEXT("SpacecraftNotManeuvering", "{spacecraft}|plural(one=The,other=A) spacecraft isn't correctly oriented"),
						TEXT("spacecraft"), PlayerArray.Num());
				}

				return ENovaTrajectoryAction::AbortIfStarted;
			}
		}
	}

	return ENovaTrajectoryAction::Continue;
}

void ANovaGameState::OnServerTimeReplicated()
{
	const APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	NCHECK(IsValid(PC) && PC->IsLocalController());

	// Evaluate the current server time
	const double PingSeconds      = UNeutronActorTools::GetPlayerLatency(PC);
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
	DOREPLIFETIME(ANovaGameState, PlayerSpacecraftIdentifiers);

	DOREPLIFETIME(ANovaGameState, ServerTime);
	DOREPLIFETIME(ANovaGameState, ServerTimeDilation);

	DOREPLIFETIME(ANovaGameState, CurrentPriceRotation);
	DOREPLIFETIME(ANovaGameState, TimeOfLastRotation);
}

#undef LOCTEXT_NAMESPACE
