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
		double  TargetLocation  = CurrentLocation.Y;

		// Process target
		if (RingComponent->IsDockEnabled())
		{
			const USceneComponent* TargetComponent = RingComponent->GetCurrentTarget();
			TargetLocation = RingComponent->GetComponentTransform().InverseTransformPosition(TargetComponent->GetSocketLocation("Dock")).Y -
							 SocketRelativeLocation.Y;
		}
		else
		{
			TargetLocation = -SocketRelativeLocation.Y - RingComponent->GetSocketTransform("Base", RTS_Component).GetTranslation().Z;
		}

		// Solve for velocities
		UNovaActorTools::SolveVelocity(CurrentLinearVelocity, 0.0, CurrentLocation.Y, TargetLocation, LinearAcceleration, MaxLinearVelocity,
			LinearDeadDistance, DeltaTime);

		// Integrate velocity to derive position
		CurrentLocation.Y += CurrentLinearVelocity * DeltaTime;
		SetRelativeLocation(CurrentLocation);
	}
}
