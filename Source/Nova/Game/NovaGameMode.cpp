// Nova project - Gwennaël Arbona

#include "NovaGameMode.h"
#include "NovaGameInstance.h"

#include "Nova/Game/NovaWorldSettings.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Nova.h"

#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/PlayerStartPIE.h"
#include "Engine/World.h"
#include "EngineUtils.h"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

ANovaGameMode::ANovaGameMode()
	: Super()
	, IsLoadingStreamingLevel(false)
	, IsUnloadingStreamingLevel(false)
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
	LoadStreamingLevel("Station");
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
	TArray<APlayerStart*> Candidates;

	// Iterate over player starts
	for (APlayerStart* PlayerStart : TActorRange<APlayerStart>(GetWorld()))
	{
		if (PlayerStart->IsA<APlayerStartPIE>())
		{
			return PlayerStart;
		}
		else
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
		SpacecraftPawn->GetSpacecraftMovement()->Reset();
	}
}

void ANovaGameMode::LeaveArea()
{
	NLOG("ANovaGameMode::LeaveArea");
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController());
	NCHECK(PC);
	ANovaSpacecraftPawn* PlayerPawn = PC->GetSpacecraftPawn();

	// Cutscene is completed during shared transition : unload level and stop
	FSimpleDelegate EndCutscene = FSimpleDelegate::CreateLambda([=]()
		{
			for (ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
			{
				NLOG("ANovaGameMode::LeaveArea : ending cutscene");
				SpacecraftPawn->GetSpacecraftMovement()->Stop();
				UnloadStreamingLevel("Station");
				LoadStreamingLevel("Orbit");
			}
		});

	// Cutscene is ending : start a share transition
	FSimpleDelegate StopCutscene = FSimpleDelegate::CreateLambda([=]()
		{
			NLOG("ANovaGameMode::LeaveArea : stopping cutscene");
			PC->SharedTransition(EndCutscene, false);
		});

	// Cutscene is starting : start leaving the area
	FSimpleDelegate StartCutscene = FSimpleDelegate::CreateLambda([=]()
		{
			NLOG("ANovaGameMode::LeaveArea : starting cutscene");
			for (ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
			{
				SpacecraftPawn->GetSpacecraftMovement()->LeaveArea(SpacecraftPawn == PlayerPawn ? StopCutscene : FSimpleDelegate());
			}
		});

	PC->SharedTransition(StartCutscene, true);
}


/*----------------------------------------------------
	Level loading
----------------------------------------------------*/

bool ANovaGameMode::LoadStreamingLevel(FName SectorLevel)
{
	if (SectorLevel != NAME_None)
	{
		NLOG("ANovaGameMode::LoadStreamingLevel : Loading streaming level '%s'", *SectorLevel.ToString());

		FLatentActionInfo Info;
		Info.CallbackTarget = this;
		Info.ExecutionFunction = "OnLevelLoaded";
		Info.UUID = CurrentStreamingLevelIndex;
		Info.Linkage = 0;

		UGameplayStatics::LoadStreamLevel(this, SectorLevel, true, false, Info);
		CurrentStreamingLevelIndex++;
		IsLoadingStreamingLevel = true;
		return false;
	}
	return true;
}

void ANovaGameMode::UnloadStreamingLevel(FName SectorLevel)
{
	if (SectorLevel != NAME_None)
	{
		NLOG("ANovaGameMode::UnloadStreamingLevel : Unloading streaming level '%s'", *SectorLevel.ToString());

		FLatentActionInfo Info;
		Info.CallbackTarget = this;
		Info.ExecutionFunction = "OnLevelUnloaded";
		Info.UUID = CurrentStreamingLevelIndex;
		Info.Linkage = 0;

		UGameplayStatics::UnloadStreamLevel(this, SectorLevel, Info, false);
		CurrentStreamingLevelIndex++;
		IsUnloadingStreamingLevel = true;
	}
}

void ANovaGameMode::OnLevelLoaded()
{
	NLOG("ANovaGameMode::OnLevelLoaded");

	IsLoadingStreamingLevel = false;

	ResetArea();
}

void ANovaGameMode::OnLevelUnLoaded()
{
	NLOG("ANovaGameMode::OnLevelUnLoaded");

	IsUnloadingStreamingLevel = false;

	ResetArea();
}
