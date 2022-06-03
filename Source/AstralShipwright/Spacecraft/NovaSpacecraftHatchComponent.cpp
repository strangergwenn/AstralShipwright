// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftHatchComponent.h"
#include "Actor/NovaMeshInterface.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Nova.h"

#include "Animation/AnimSingleNodeInstance.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftHatchComponent::UNovaSpacecraftHatchComponent() : Super(), CurrentDockingState(false)
{
	// Settings
	PrimaryComponentTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaSpacecraftHatchComponent::SetAdditionalAsset(TSoftObjectPtr<UObject> AdditionalAsset)
{}

void UNovaSpacecraftHatchComponent::BeginPlay()
{
	Super::BeginPlay();

	HatchMesh = Cast<UNovaSkeletalMeshComponent>(GetAttachParent());
	NCHECK(HatchMesh);
}

void UNovaSpacecraftHatchComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(GetOwner());
	if (IsValid(SpacecraftPawn))
	{
		bool DesiredDockingState = SpacecraftPawn->IsDocking() || SpacecraftPawn->IsDocked();

		if (DesiredDockingState != CurrentDockingState)
		{
			NLOG("UNovaSpacecraftHatchComponent::TickComponent : new dock state is %d", DesiredDockingState);

			HatchMesh->GetSingleNodeInstance()->SetReverse(!DesiredDockingState);
			HatchMesh->GetSingleNodeInstance()->SetPlaying(true);
			CurrentDockingState = DesiredDockingState;
		}
	}
}
