// Astral Shipwright - Gwennaël Arbona

#include "NovaGameMode.h"

#include "NovaArea.h"
#include "NovaGameModeStates.h"
#include "NovaGameState.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Game/Settings/NovaWorldSettings.h"

#include "Actor/NovaPlayerStart.h"
#include "Player/NovaPlayerController.h"
#include "Player/NovaPlayerState.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

#include "System/NovaAssetManager.h"
#include "System/NovaGameInstance.h"

#include "Nova.h"

#include "GameFramework/SpectatorPawn.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "ANovaGameMode"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaGameMode::ANovaGameMode()
	: Super()
	, DesiredStateIdentifier(ENovaGameStateIdentifier::Area)
	, CurrentStateIdentifier(ENovaGameStateIdentifier::Area)
	, CurrentStreamingLevelIndex(0)
{
	// Defaults
	GameStateClass        = ANovaGameState::StaticClass();
	PlayerStateClass      = ANovaPlayerState::StaticClass();
	PlayerControllerClass = ANovaPlayerController::StaticClass();
	DefaultPawnClass      = ANovaSpacecraftPawn::StaticClass();

	// Settings
	PrimaryActorTick.bCanEverTick = true;
	bUseSeamlessTravel            = true;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void ANovaGameMode::InitGameState()
{
	NLOG("ANovaGameMode::InitGameState");
	Super::InitGameState();
}

void ANovaGameMode::StartPlay()
{
	NLOG("ANovaGameMode::StartPlay");

	// Menu level
	if (Cast<ANovaWorldSettings>(GetWorld()->GetWorldSettings())->IsMainMenuMap())
	{
		Super::StartPlay();
	}

	// Game level
	else
	{
		ANovaGameState* NovaGameState = GetGameState<ANovaGameState>();
		NCHECK(IsValid(NovaGameState));
		UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
		NCHECK(GameInstance);

#if WITH_EDITOR

		// Ensure valid save data exists even if the game was loaded directly on a map (PIE server)
		if (GetLocalRole() == ROLE_Authority && !GameInstance->HasSave())
		{
			GameInstance->LoadGame("1");
		}

#endif    // WITH_EDITOR

		// Load the game world
		if (!IsValid(NovaGameState->GetCurrentArea()))
		{
			NCHECK(GameInstance->HasSave())
			NovaGameState->Load(GameInstance->GetWorldSave());
		}

		// Start players
		Super::StartPlay();

		// Load the level
		const UNovaArea* CurrentArea = NovaGameState->GetCurrentArea();
		NCHECK(IsValid(CurrentArea) && CurrentArea != OrbitArea);
		LoadStreamingLevel(CurrentArea);

		// Startup the state machine
		InitializeStateMachine();
	}
}

void ANovaGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	NLOG("ANovaGameMode::PreLogin");

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (ErrorMessage.IsEmpty())
	{
		FText Error;
		GetGameState<ANovaGameState>()->IsJoinable(&Error);
		ErrorMessage = Error.ToString();
	}
}

void ANovaGameMode::PostLogin(APlayerController* Player)
{
	NLOG("ANovaGameMode::PostLogin");
	Super::PostLogin(Player);
}

void ANovaGameMode::Logout(AController* Player)
{
	NLOG("ANovaGameMode::Logout");
	Super::Logout(Player);
}

UClass* ANovaGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (Cast<ANovaWorldSettings>(GetWorld()->GetWorldSettings())->IsMainMenuMap())
	{
		return ASpectatorPawn::StaticClass();
	}
	else
	{
		return Super::GetDefaultPawnClassForController_Implementation(InController);
	}
}

AActor* ANovaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	UClass*                   PawnClass = GetDefaultPawnClassForController(Player);
	APawn*                    PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<ANovaPlayerStart*> Candidates;

	// Iterate over player starts
	for (ANovaPlayerStart* PlayerStart : TActorRange<ANovaPlayerStart>(GetWorld()))
	{
		// Check that this player start is not part of a level being unloaded
		bool IsValidPlayerStart = true;
		if (IsValid(PlayerStart) && IsValid(PlayerStart->GetLevel()))
		{
			for (const ULevelStreaming* Level : GetWorld()->GetStreamingLevels())
			{
				if (Level->GetLoadedLevel() &&
					Level->GetLoadedLevel()->GetOuter()->GetFName() == PlayerStart->GetLevel()->GetOuter()->GetFName() &&
					Level->IsStreamingStatePending())
				{
					IsValidPlayerStart = false;
				}
			}
		}
		else
		{
			IsValidPlayerStart = false;
		}

		// Check that the player start is not already in use
		for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
		{
			const UNovaSpacecraftMovementComponent* MovementComponent = SpacecraftPawn->GetSpacecraftMovement();
			NCHECK(MovementComponent);

			if (PlayerStart == MovementComponent->GetPlayerStart())
			{
				IsValidPlayerStart = false;
				break;
			}
		}

		if (IsValidPlayerStart)
		{
			Candidates.Add(PlayerStart);
		}
	}

	if (Candidates.Num() > 0)
	{
		return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	}

	return nullptr;
}

void ANovaGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (StateMap.Num())
	{
		ProcessStateMachine();
	}
}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaGameMode::FastForward()
{
	DesiredStateIdentifier = ENovaGameStateIdentifier::FastForward;
}

bool ANovaGameMode::CanFastForward(FText* AbortReason) const
{
	const ANovaGameState* NovaGameState = GetGameState<ANovaGameState>();
	NCHECK(IsValid(NovaGameState));

	return StateMap.Num() && StateMap[CurrentStateIdentifier]->CanFastForward() && NovaGameState->CanFastForward(AbortReason);
}

void ANovaGameMode::ResetSpacecraft()
{
	for (ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		SpacecraftPawn->GetSpacecraftMovement()->Reset();
	}
}

void ANovaGameMode::ChangeArea(const UNovaArea* Area)
{
	NCHECK(IsValid(Area));

	NLOG("ANovaGameMode::ChangeArea : '%s'", *Area->LevelName.ToString());

	// Compare with the current area and process the change if needed
	const UNovaArea* CurrentArea = GetGameState<ANovaGameState>()->GetCurrentArea();
	if (Area != CurrentArea)
	{
		UnloadStreamingLevel(CurrentArea);
		LoadStreamingLevel(Area,    //
			FSimpleDelegate::CreateLambda(
				[this]()
				{
					ResetSpacecraft();
				}));
	}

	ResetSpacecraft();
}

void ANovaGameMode::SetCurrentAreaVisible(bool Visibility)
{
	const UNovaArea* CurrentArea = GetGameState<ANovaGameState>()->GetCurrentArea();
	NCHECK(IsValid(CurrentArea));

	NLOG("ANovaGameMode::SetAreaVisible : '%s' -> %d", *CurrentArea->LevelName.ToString(), Visibility);

	ULevelStreaming* Level = UGameplayStatics::GetStreamingLevel(this, CurrentArea->LevelName);
	NCHECK(Level)
	{
		Level->SetShouldBeVisible(Visibility);

		GetWorld()->FlushLevelStreaming(EFlushLevelStreamingType::Visibility);
	}
}

void ANovaGameMode::ChangeAreaToOrbit()
{
	ChangeArea(OrbitArea);
}

bool ANovaGameMode::IsInOrbit() const
{
	return GetGameState<ANovaGameState>()->GetCurrentArea() == OrbitArea;
}

FNovaTime ANovaGameMode::GetManeuverWarnTime() const
{
	TSharedPtr<FNovaGameModeState> CurrentState = StateMap[CurrentStateIdentifier];
	return CurrentState->GetManeuverWarningTime();
}

FNovaTime ANovaGameMode::GetArrivalWarningTime() const
{
	TSharedPtr<FNovaGameModeState> CurrentState = StateMap[CurrentStateIdentifier];
	return CurrentState->GetArrivalWarningTime();
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaGameMode::InitializeStateMachine()
{
	// Fetch data
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController());
	if (IsValid(PC) && PC->IsLocalController())
	{
		ANovaGameState* NovaGameState = GetGameState<ANovaGameState>();
		NCHECK(IsValid(NovaGameState));
		UNovaOrbitalSimulationComponent* OrbitalSimulationComponent = NovaGameState->GetOrbitalSimulation();
		NCHECK(IsValid(OrbitalSimulationComponent));

		// State initializer
		auto AddState = [&](ENovaGameStateIdentifier Identifier, TSharedPtr<FNovaGameModeState> State, const FString& Name)
		{
			State->Initialize(Name, PC, this, GetGameState<ANovaGameState>(), OrbitalSimulationComponent);
			StateMap.Add(Identifier, State);
		};

		// Create states
		AddState(ENovaGameStateIdentifier::Area, MakeShared<FNovaAreaState>(), TEXT("Area"));
		AddState(ENovaGameStateIdentifier::Orbit, MakeShared<FNovaOrbitState>(), TEXT("Orbit"));
		AddState(ENovaGameStateIdentifier::FastForward, MakeShared<FNovaFastForwardState>(), TEXT("FastForward"));
		AddState(ENovaGameStateIdentifier::DepartureProximity, MakeShared<FNovaDepartureProximityState>(), TEXT("DepartureProximity"));
		AddState(ENovaGameStateIdentifier::ArrivalIntro, MakeShared<FNovaArrivalIntroState>(), TEXT("ArrivalIntro"));
		AddState(ENovaGameStateIdentifier::ArrivalProximity, MakeShared<FNovaArrivalProximityState>(), TEXT("ArrivalProximity"));
	}
}

void ANovaGameMode::ProcessStateMachine()
{
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController());
	NCHECK(IsValid(PC) && PC->IsLocalController());

	if (!PC->IsInSharedTransition())
	{
		// Process the current state
		TSharedPtr<FNovaGameModeState> State = StateMap[CurrentStateIdentifier];
		NCHECK(State.IsValid());
		ENovaGameStateIdentifier NewStateIdentifier = State->UpdateState();

		// If the current state didn't exit, process the desired state instead
		if (NewStateIdentifier == CurrentStateIdentifier)
		{
			NewStateIdentifier = DesiredStateIdentifier;
		}

		// Process state changes : leave the current, enter the new
		if (NewStateIdentifier != CurrentStateIdentifier)
		{
			NLOG("ANovaGameMode::ProcessStateMachine : changing state from %d to %d", CurrentStateIdentifier, NewStateIdentifier);

			// Stop time dilation
			GetGameState<ANovaGameState>()->SetTimeDilation(ENovaTimeDilation::Normal);

			// Leave the current state
			State->LeaveState(CurrentStateIdentifier);

			// Enter the new state
			TSharedPtr<FNovaGameModeState> NewState = StateMap[NewStateIdentifier];
			NCHECK(NewState.IsValid());
			NewState->EnterState(CurrentStateIdentifier);

			CurrentStateIdentifier = NewStateIdentifier;
			DesiredStateIdentifier = NewStateIdentifier;
		}
	}
}

bool ANovaGameMode::LoadStreamingLevel(const UNovaArea* Area, FSimpleDelegate Callback)
{
	NCHECK(Area);

	if (Area->LevelName != NAME_None)
	{
		ANovaGameState* CurrentGameState = GetGameState<ANovaGameState>();
		CurrentGameState->SetCurrentArea(Area);
		CurrentGameState->RotatePrices();

		NLOG("ANovaGameMode::LoadStreamingLevel : loading streaming level '%s'", *Area->LevelName.ToString());

		FLatentActionInfo Info;
		Info.CallbackTarget    = this;
		Info.ExecutionFunction = "OnLevelLoaded";
		Info.UUID              = CurrentStreamingLevelIndex;
		Info.Linkage           = 0;

		UGameplayStatics::LoadStreamLevel(this, Area->LevelName, true, false, Info);
		CurrentStreamingLevelIndex++;
		OnLevelLoadedCallback = Callback;
		return false;
	}
	return true;
}

void ANovaGameMode::UnloadStreamingLevel(const UNovaArea* Area, FSimpleDelegate Callback)
{
	NCHECK(Area);

	if (Area->LevelName != NAME_None)
	{
		NLOG("ANovaGameMode::UnloadStreamingLevel : unloading streaming level '%s'", *Area->LevelName.ToString());

		FLatentActionInfo Info;
		Info.CallbackTarget    = this;
		Info.ExecutionFunction = "OnLevelUnloaded";
		Info.UUID              = CurrentStreamingLevelIndex;
		Info.Linkage           = 0;

		UGameplayStatics::UnloadStreamLevel(this, Area->LevelName, Info, false);
		CurrentStreamingLevelIndex++;
		OnLevelUnloadedCallback = Callback;
	}
}

void ANovaGameMode::OnLevelLoaded()
{
	NLOG("ANovaGameMode::OnLevelLoaded");

	OnLevelLoadedCallback.ExecuteIfBound();
}

void ANovaGameMode::OnLevelUnLoaded()
{
	NLOG("ANovaGameMode::OnLevelUnLoaded");

	OnLevelUnloadedCallback.ExecuteIfBound();
}

#undef LOCTEXT_NAMESPACE
