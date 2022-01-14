// Astral Shipwright - Gwennaël Arbona

#include "NovaAISpacecraft.h"

#include "Player/NovaPlayerController.h"
#include "Spacecraft/NovaSpacecraftPawn.h"

#include "Nova.h"

/*----------------------------------------------------
    AI spacecraft description
----------------------------------------------------*/

void UNovaAISpacecraftDescription::LoadInGame()
{
#if WITH_EDITOR

	NLOG("UNovaAISpacecraftDescription::LoadInGame : '%s'", *GetName());

	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	if (SpacecraftPawn)
	{
		FNovaSpacecraft NewSpacecraft = Spacecraft;
		NewSpacecraft.Identifier      = SpacecraftPawn->GetSpacecraftIdentifier();

		SpacecraftPawn->SetSpacecraft(&NewSpacecraft);
	}

#endif    // WITH_EDITOR
}

void UNovaAISpacecraftDescription::SaveFromGame()
{
#if WITH_EDITOR

	NLOG("UNovaAISpacecraftDescription::SaveFromGame : '%s'", *GetName());

	const ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();

	if (SpacecraftPawn)
	{
		Spacecraft            = SpacecraftPawn->GetSpacecraftCopy();
		Spacecraft.Identifier = Identifier;

		MarkPackageDirty();
	}

#endif    // WITH_EDITOR
}

ANovaSpacecraftPawn* UNovaAISpacecraftDescription::GetSpacecraftPawn() const
{
	// Find a player controller
	TArray<AActor*> PlayerControllers;
	for (const FWorldContext& World : GEngine->GetWorldContexts())
	{
		UGameplayStatics::GetAllActorsOfClass(World.World(), ANovaPlayerController::StaticClass(), PlayerControllers);
		if (PlayerControllers.Num())
		{
			break;
		}
	}

	// Proceed
	if (PlayerControllers.Num() > 0)
	{
		ANovaPlayerController* PlayerController = Cast<ANovaPlayerController>(PlayerControllers[0]);
		NCHECK(PlayerController);

		ANovaSpacecraftPawn* SpacecraftPawn = PlayerController->GetSpacecraftPawn();
		if (IsValid(SpacecraftPawn))
		{
			return SpacecraftPawn;
		}
	}

	return nullptr;
}
