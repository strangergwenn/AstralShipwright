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
	MaxLinearVelocity  = 10;
	LinearAcceleration = 20;

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

		if (RingComponent->IsOperating())
		{
			FVector CurrentLocation = GetRelativeLocation();
			double  TargetLocation  = CurrentLocation.Y;

			// Process target
			const USceneComponent* TargetComponent = RingComponent->GetCurrentTarget();
			if (IsValid(TargetComponent) && RingComponent->IsCurrentTargetHatch())
			{
				DrawDebugLine(GetWorld(), TargetComponent->GetComponentLocation(), GetSocketTransform("Dock", RTS_World).GetTranslation(),
					FColor::Green);

				FVector Distance = TargetComponent->GetComponentLocation() - GetSocketTransform("Dock", RTS_World).GetTranslation();
				NLOG("%f %f %f / target at %f %f %f / arm at %f %f %f / ring at %f %f %f", Distance.X, Distance.Y, Distance.Z,
					TargetComponent->GetComponentLocation().X, TargetComponent->GetComponentLocation().Y,
					TargetComponent->GetComponentLocation().Z, GetComponentLocation().X, GetComponentLocation().Y, GetComponentLocation().Z,
					RingComponent->GetComponentLocation().X, RingComponent->GetComponentLocation().Y,
					RingComponent->GetComponentLocation().Z);

				TargetLocation =
					RingComponent->GetComponentTransform().InverseTransformPosition(TargetComponent->GetComponentLocation()).Y -
					SocketRelativeLocation.Y;
			}

			// Solve for velocities
			UNovaActorTools::SolveVelocity(CurrentLinearVelocity, 0.0, CurrentLocation.Y / 100.0, TargetLocation / 100.0,
				LinearAcceleration, MaxLinearVelocity, LinearDeadDistance, DeltaTime);

			// Integrate velocity to derive position
			CurrentLocation.Y += CurrentLinearVelocity * 100.0 * DeltaTime;
			SetRelativeLocation(CurrentLocation);
		}
	}
}
