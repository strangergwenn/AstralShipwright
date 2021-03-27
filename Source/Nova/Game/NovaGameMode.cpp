// Nova project - Gwennaël Arbona

#include "NovaGameMode.h"
#include "NovaGameInstance.h"

#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameWorld.h"
#include "Nova/Game/NovaWorldSettings.h"

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

ANovaGameMode::ANovaGameMode() : Super(), CurrentStreamingLevelIndex(0)
{
	// Defaults
	GameStateClass        = ANovaGameState::StaticClass();
	PlayerStateClass      = ANovaPlayerState::StaticClass();
	PlayerControllerClass = ANovaPlayerController::StaticClass();
	DefaultPawnClass      = ANovaSpacecraftPawn::StaticClass();

	// Settings
	bUseSeamlessTravel = true;
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
	LoadStreamingLevel(Station, true,
		FSimpleDelegate::CreateLambda(
			[=]()
			{
				ResetArea();
			}));
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

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaGameMode::ResetArea()
{
	NLOG("ANovaGameMode::ResetArea");

	for (ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		ANovaPlayerController* PC    = SpacecraftPawn->GetController<ANovaPlayerController>();
		AActor*                Start = ChoosePlayerStart(PC);
		NCHECK(Start);

		SpacecraftPawn->GetSpacecraftMovement()->Reset();
	}
}

void ANovaGameMode::ChangeAreaToOrbit()
{
	ChangeArea(OrbitArea);
}

void ANovaGameMode::ChangeArea(const UNovaArea* Area)
{
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController());
	NCHECK(PC);
	NCHECK(Area);

	NLOG("ANovaGameMode::ChangeArea : '%s'", *Area->LevelName.ToString());
	ANovaSpacecraftPawn* PlayerPawn = PC->GetSpacecraftPawn();

	// 5 : Cutscene is finished, level has loaded, reinitialize ships
	FSimpleDelegate ResetSpacecraft = FSimpleDelegate::CreateLambda(
		[=]()
		{
			ResetArea();
		});

	// 4 : Cutscene is completed during shared transition : switch the levels
	FSimpleDelegate SwitchLevels = FSimpleDelegate::CreateLambda(
		[=]()
		{
			for (ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
			{
				NLOG("ANovaGameMode::ChangeArea : ending cutscene");
				SpacecraftPawn->GetSpacecraftMovement()->Stop();
			}

			UnloadStreamingLevel(GetGameState<ANovaGameState>()->GetCurrentArea());
			LoadStreamingLevel(Area);
		});

	// 3: Cutscene is ending : start a shared transition
	FSimpleDelegate StopCutscene = FSimpleDelegate::CreateLambda(
		[=]()
		{
			NLOG("ANovaGameMode::ChangeArea : stopping cutscene");
			PC->SharedTransition(false, SwitchLevels, ResetSpacecraft);
		});

	// 2 : Cutscene is starting : start leaving the area
	FSimpleDelegate StartCutscene = FSimpleDelegate::CreateLambda(
		[=]()
		{
			NLOG("ANovaGameMode::ChangeArea : starting cutscene");
			for (ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
			{
				SpacecraftPawn->GetSpacecraftMovement()->LeaveArea(SpacecraftPawn == PlayerPawn ? StopCutscene : FSimpleDelegate());
			}
		});

	// 1 : Start a shared transition for the cutscene
	PC->SharedTransition(true, StartCutscene);
}

bool ANovaGameMode::IsInOrbit() const
{
	return GetGameState<ANovaGameState>()->GetCurrentArea() == OrbitArea;
}

/*----------------------------------------------------
    Level loading
----------------------------------------------------*/

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
