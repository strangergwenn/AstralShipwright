// Nova project - Gwennaël Arbona

#include "NovaGameMode.h"

#include "NovaAssetCatalog.h"
#include "NovaArea.h"
#include "NovaGameInstance.h"
#include "NovaGameModeStates.h"
#include "NovaGameState.h"
#include "NovaGameWorld.h"
#include "NovaOrbitalSimulationComponent.h"
#include "NovaWorldSettings.h"

#include "Nova/Actor/NovaPlayerStart.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Player/NovaPlayerState.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Nova.h"

#include "GameFramework/SpectatorPawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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

	// Spawn the game world
	UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
	if (GameInstance)
	{
		ANovaGameWorld* GameWorld = GetWorld()->SpawnActor<ANovaGameWorld>();
		NCHECK(IsValid(GameInstance));
		GetGameState<ANovaGameState>()->SetGameWorld(GameWorld);
	}
}

void ANovaGameMode::StartPlay()
{
	NLOG("ANovaGameMode::StartPlay");
	Super::StartPlay();

	// Load the game world
	UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
	NCHECK(GameInstance);
	if (GameInstance->HasSave())
	{
		ANovaGameWorld* GameWorld = GetGameState<ANovaGameState>()->GetGameWorld();
		NCHECK(IsValid(GameInstance));
		GameWorld->Load(GameInstance->GetWorldSave());
	}

	// TODO : this should be dependent on save data
	const UNovaArea* Station = GameInstance->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
	LoadStreamingLevel(Station, true, FSimpleDelegate());

	// Startup the state machine
	InitializeStateMachine();
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
		FVector        ActorLocation = PlayerStart->GetActorLocation();
		const FRotator ActorRotation = PlayerStart->GetActorRotation();

		// Calculate the horizontal distance with other pawns to avoid colliding
		// This is required because EncroachingBlockingGeometry() does not work without a full movement component
		bool Collided = false;
		for (APawn* Pawn : TActorRange<APawn>(GetWorld()))
		{
			FVector DistanceVector = Pawn->GetActorLocation() - ActorLocation;
			DistanceVector.Z       = 0;

			if (DistanceVector.Size() < 500)
			{
				NLOG("ANovaGameMode::ChoosePlayerStart_Implementation : encroaching '%s' invalidated '%s'", *Pawn->GetName(),
					*PlayerStart->GetName());

				Collided = true;
				break;
			}
		}

		if (!Collided)
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

	ProcessStateMachine();
}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaGameMode::FastForward()
{
	DesiredStateIdentifier = ENovaGameStateIdentifier::FastForward;
}

bool ANovaGameMode::CanFastForward() const
{
	const ANovaGameWorld* GameWorld = GetGameState<ANovaGameState>()->GetGameWorld();
	NCHECK(IsValid(GameWorld));

	return (CurrentStateIdentifier == ENovaGameStateIdentifier::Area || CurrentStateIdentifier == ENovaGameStateIdentifier::ArrivalCoast ||
			   CurrentStateIdentifier == ENovaGameStateIdentifier::Orbit ||
			   CurrentStateIdentifier == ENovaGameStateIdentifier::DepartureCoast) &&
		   GameWorld->CanFastForward();
}

void ANovaGameMode::ChangeArea(const UNovaArea* Area)
{
	NCHECK(IsValid(Area));
	NCHECK(Area->IsValidLowLevelFast());

	// Compare with the current area and exit
	const UNovaArea* CurrentArea = GetGameState<ANovaGameState>()->GetCurrentArea();
	if (Area != CurrentArea)
	{
		NLOG("ANovaGameMode::ChangeArea : '%s'", *Area->LevelName.ToString());

		for (ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
		{
			SpacecraftPawn->GetSpacecraftMovement()->Stop();
		}

		UnloadStreamingLevel(CurrentArea);
		LoadStreamingLevel(Area);
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

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaGameMode::InitializeStateMachine()
{
	// Fetch data
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController());
	NCHECK(IsValid(PC) && PC->IsLocalController());
	ANovaGameWorld* GameWorld = GetGameState<ANovaGameState>()->GetGameWorld();
	NCHECK(IsValid(GameWorld));
	UNovaOrbitalSimulationComponent* OrbitalSimulationComponent = GameWorld->GetOrbitalSimulation();
	NCHECK(IsValid(OrbitalSimulationComponent));

	// State initializer
	auto AddState = [&](ENovaGameStateIdentifier Identifier, TSharedPtr<FNovaGameState> State, const FString& Name)
	{
		State->Initialize(Name, PC, this, GameWorld, OrbitalSimulationComponent);
		StateMap.Add(Identifier, State);
	};

	// Create states
	AddState(ENovaGameStateIdentifier::Area, MakeShared<FNovaAreaState>(), TEXT("Area"));
	AddState(ENovaGameStateIdentifier::Orbit, MakeShared<FNovaOrbitState>(), TEXT("Orbit"));
	AddState(ENovaGameStateIdentifier::FastForward, MakeShared<FNovaFastForwardState>(), TEXT("FastForward"));
	AddState(ENovaGameStateIdentifier::DepartureProximity, MakeShared<FNovaDepartureProximityState>(), TEXT("DepartureProximity"));
	AddState(ENovaGameStateIdentifier::DepartureCoast, MakeShared<FNovaDepartureCoastState>(), TEXT("DepartureCoast"));
	AddState(ENovaGameStateIdentifier::ArrivalIntro, MakeShared<FNovaArrivalIntroState>(), TEXT("ArrivalIntro"));
	AddState(ENovaGameStateIdentifier::ArrivalCoast, MakeShared<FNovaArrivalCoastState>(), TEXT("ArrivalCoast"));
	AddState(ENovaGameStateIdentifier::ArrivalProximity, MakeShared<FNovaArrivalProximityState>(), TEXT("ArrivalProximity"));
}

void ANovaGameMode::ProcessStateMachine()
{
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController());
	NCHECK(IsValid(PC) && PC->IsLocalController());

	if (!PC->IsInSharedTransition())
	{
		// Process the current state
		TSharedPtr<FNovaGameState> State = StateMap[CurrentStateIdentifier];
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

			// Stop tile dilation
			ANovaGameWorld* GameWorld = GetGameState<ANovaGameState>()->GetGameWorld();
			NCHECK(IsValid(GameWorld));
			GameWorld->SetTimeDilation(ENovaTimeDilation::Normal);

			// Leave the current state
			State->LeaveState(CurrentStateIdentifier);

			// Enter the new state
			TSharedPtr<FNovaGameState> NewState = StateMap[NewStateIdentifier];
			NCHECK(NewState.IsValid());
			NewState->EnterState(CurrentStateIdentifier);

			CurrentStateIdentifier = NewStateIdentifier;
			DesiredStateIdentifier = NewStateIdentifier;
		}
	}
}

bool ANovaGameMode::LoadStreamingLevel(const UNovaArea* Area, bool StartDocked, FSimpleDelegate Callback)
{
	NCHECK(Area);

	if (Area->LevelName != NAME_None)
	{
		GetGameState<ANovaGameState>()->SetCurrentArea(Area, StartDocked);

		NLOG("ANovaGameMode::LoadStreamingLevel : loading streaming level '%s' (%d)", *Area->LevelName.ToString(), StartDocked);

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
