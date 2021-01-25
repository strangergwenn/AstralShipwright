// Nova project - Gwennaël Arbona

#include "NovaGameMode.h"
#include "NovaGameInstance.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Game/NovaWorldSettings.h"
#include "Nova/Player/NovaPlayerController.h"

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
{
	PlayerControllerClass = ANovaPlayerController::StaticClass();
	DefaultPawnClass = ANovaSpacecraftPawn::StaticClass();

	bUseSeamlessTravel = true;
}


/*----------------------------------------------------
	Inherited
----------------------------------------------------*/

void ANovaGameMode::StartPlay()
{
	NLOG("ANovaGameMode::StartPlay");
	Super::StartPlay();
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
