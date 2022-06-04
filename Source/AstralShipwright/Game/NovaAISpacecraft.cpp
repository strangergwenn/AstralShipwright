// Astral Shipwright - Gwennaël Arbona

#include "NovaAISpacecraft.h"

#include "Player/NovaPlayerController.h"
#include "Spacecraft/NovaSpacecraftPawn.h"

#include "Nova.h"

#include "Neutron/Actor/NeutronCaptureActor.h"

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

		SpacecraftPawn->SetEditing(true);
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

FNeutronAssetPreviewSettings UNovaAISpacecraftDescription::GetPreviewSettings() const
{
	FNeutronAssetPreviewSettings Settings;

	Settings.Class                   = ANovaSpacecraftPawn::StaticClass();
	Settings.RequireCustomPrimitives = true;
	Settings.Rotation                = FRotator(0, 180, 0);
	Settings.RelativeXOffset         = -0.5f / Spacecraft.Compartments.Num();
	Settings.Scale *= 1.25f;

	return Settings;
}

void UNovaAISpacecraftDescription::ConfigurePreviewActor(AActor* Actor) const
{
	NCHECK(Actor->GetClass() == ANovaSpacecraftPawn::StaticClass());
	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(Actor);

	SpacecraftPawn->SetImmediateMode(true);
	SpacecraftPawn->SetSpacecraft(&Spacecraft);
}
