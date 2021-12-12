// Astral Shipwright - Gwennaël Arbona

#include "NovaStationDockComponent.h"
#include "NovaStationRingComponent.h"

#include "Actor/NovaActorTools.h"
#include "Spacecraft/NovaSpacecraftPawn.h"

#include "Nova.h"

#include "DrawDebugHelpers.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaStationDockComponent::UNovaStationDockComponent() : Super(), CurrentLinearVelocity(0)
{
	// Defaults
	LinearDeadDistance = 1;
	MaxLinearVelocity  = 1000;
	LinearAcceleration = 1000;

	// Settings
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionProfileName("NoCollision");
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaStationDockComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find our where the socket is
	FTransform SocketTransform = GetSocketTransform("Dock", RTS_Component);
	SocketRelativeLocation     = SocketTransform.GetTranslation();
}

void UNovaStationDockComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UNovaStationRingComponent* RingComponent = Cast<UNovaStationRingComponent>(GetAttachParent());

	// Process the dock logic
	if (IsValid(RingComponent))
	{
		// Show up only when outside assembly
		const ANovaSpacecraftPawn* Spacecraft = RingComponent->GetCurrentSpacecraft();
		if (!IsValid(Spacecraft) || Spacecraft->GetCompartmentFilter() == INDEX_NONE)
		{
			Materialize();
		}
		else
		{
			Dematerialize();
		}

		FVector CurrentLocation = GetRelativeLocation();
		double  TargetLocation  = CurrentLocation.Z;

		// Process target
		if (RingComponent->IsDockEnabled())
		{
			const USceneComponent* TargetComponent = RingComponent->GetCurrentTarget();
			TargetLocation = SocketRelativeLocation.Y + RingComponent->GetComponentTransform().InverseTransformPosition(TargetComponent->GetSocketLocation("Dock")).Z;
		}
		else
		{
			TargetLocation = RingComponent->GetSocketTransform("Base", RTS_Component).GetTranslation().Z + SocketRelativeLocation.Y;
		}

		// Solve for velocities
		UNovaActorTools::SolveVelocity(CurrentLinearVelocity, 0.0, CurrentLocation.Z, TargetLocation, LinearAcceleration, MaxLinearVelocity,
			LinearDeadDistance, DeltaTime);

		// Integrate velocity to derive position
		CurrentLocation.Z += CurrentLinearVelocity * DeltaTime;
		SetRelativeLocation(CurrentLocation);
	}
}
