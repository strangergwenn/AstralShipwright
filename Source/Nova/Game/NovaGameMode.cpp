// Nova project - Gwennaël Arbona

#include "NovaGameMode.h"
#include "NovaGameInstance.h"

#include "Nova/Actor/NovaPlayerStart.h"
#include "Nova/Game/NovaWorldSettings.h"
#include "Nova/Player/NovaPlayerController.h"
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
	, CurrentStreamingLevelIndex(0)
{
	// Defaults
	PlayerControllerClass = ANovaPlayerController::StaticClass();
	DefaultPawnClass = ANovaSpacecraftPawn::StaticClass();

	// Settings
	bUseSeamlessTravel = true;
}


/*----------------------------------------------------
	Inherited
----------------------------------------------------*/

void ANovaGameMode::StartPlay()
{
	NLOG("ANovaGameMode::StartPlay");
	Super::StartPlay();

	// TODO : this should be dependent on save data
	LoadStreamingLevel("Station", FSimpleDelegate::CreateLambda([=]()
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
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<ANovaPlayerStart*> Candidates;

	// Iterate over player starts
	for (ANovaPlayerStart* PlayerStart : TActorRange<ANovaPlayerStart>(GetWorld()))
	{
		FVector ActorLocation = PlayerStart->GetActorLocation();
		const FRotator ActorRotation = PlayerStart->GetActorRotation();

		// Calculate the horizontal distance with other pawns to avoid colliding
		// This is required because EncroachingBlockingGeometry() does not work without a full movement component
		bool Collided = false;
		for (APawn* Pawn : TActorRange<APawn>(GetWorld()))
		{
			FVector DistanceVector = Pawn->GetActorLocation() - ActorLocation;
			DistanceVector.Z = 0;

			if (DistanceVector.Size() < 500)
			{
				NLOG("AVestaANovaGameModeGameMode::ChoosePlayerStart_Implementation : encroaching '%s' invalidated '%s'",
					*Pawn->GetName(), *PlayerStart->GetName());

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
		ANovaPlayerController* PC = SpacecraftPawn->GetController<ANovaPlayerController>();
		AActor* Start = ChoosePlayerStart(PC);
		NCHECK(Start);

		SpacecraftPawn->GetSpacecraftMovement()->Reset();
	}
}

void ANovaGameMode::ChangeArea(FName LevelName)
{
	NLOG("ANovaGameMode::ChangeArea : '%s'", *LevelName.ToString());
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController());
	NCHECK(PC);
	ANovaSpacecraftPawn* PlayerPawn = PC->GetSpacecraftPawn();

	// 5 : Cutscene is finished, level has loaded, reinitialize ships
	FSimpleDelegate ResetSpacecraft = FSimpleDelegate::CreateLambda([=]()
		{
			ResetArea();
		});

	// 4 : Cutscene is completed during shared transition : switch the levels
	FSimpleDelegate SwitchLevels = FSimpleDelegate::CreateLambda([=]()
		{
			for (ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
			{
				NLOG("ANovaGameMode::ChangeArea : ending cutscene");
				SpacecraftPawn->GetSpacecraftMovement()->Stop();
				UnloadStreamingLevel(CurrentLevelName);
				LoadStreamingLevel(LevelName);
			}
		});

	// 3: Cutscene is ending : start a shared transition
	FSimpleDelegate StopCutscene = FSimpleDelegate::CreateLambda([=]()
		{
			NLOG("ANovaGameMode::ChangeArea : stopping cutscene");
			PC->SharedTransition(false, SwitchLevels, ResetSpacecraft);
		});

	// 2 : Cutscene is starting : start leaving the area
	FSimpleDelegate StartCutscene = FSimpleDelegate::CreateLambda([=]()
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


/*----------------------------------------------------
	Level loading
----------------------------------------------------*/

bool ANovaGameMode::LoadStreamingLevel(FName LevelName, FSimpleDelegate Callback)
{
	if (LevelName != NAME_None)
	{
		CurrentLevelName = LevelName;

		NLOG("ANovaGameMode::LoadStreamingLevel : loading streaming level '%s'", *LevelName.ToString());

		FLatentActionInfo Info;
		Info.CallbackTarget = this;
		Info.ExecutionFunction = "OnLevelLoaded";
		Info.UUID = CurrentStreamingLevelIndex;
		Info.Linkage = 0;

		UGameplayStatics::LoadStreamLevel(this, LevelName, true, false, Info);
		CurrentStreamingLevelIndex++;
		OnLevelLoadedCallback = Callback;
		return false;
	}
	return true;
}

void ANovaGameMode::UnloadStreamingLevel(FName LevelName, FSimpleDelegate Callback)
{
	if (LevelName != NAME_None)
	{
		NLOG("ANovaGameMode::UnloadStreamingLevel : unloading streaming level '%s'", *LevelName.ToString());

		FLatentActionInfo Info;
		Info.CallbackTarget = this;
		Info.ExecutionFunction = "OnLevelUnloaded";
		Info.UUID = CurrentStreamingLevelIndex;
		Info.Linkage = 0;

		UGameplayStatics::UnloadStreamLevel(this, LevelName, Info, false);
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
