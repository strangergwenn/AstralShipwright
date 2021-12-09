// Astral Shipwright - Gwennaël Arbona

#include "NovaStationRingComponent.h"
#include "NovaPlayerStart.h"

#include "Actor/NovaActorTools.h"

#include "Spacecraft/NovaSpacecraftCompartmentComponent.h"
#include "Spacecraft/NovaSpacecraftHatchComponent.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"
#include "Spacecraft/NovaSpacecraftPawn.h"

#include "Nova.h"

#include "DrawDebugHelpers.h"

#define SHOW_TRACES 0

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaStationRingComponent::UNovaStationRingComponent() : Super(), CurrentLinearVelocity(0), CurrentRollVelocity(0)
{
	// Defaults
	LinearDeadDistance  = 1;
	MaxLinearVelocity   = 50;
	LinearAcceleration  = 10;
	AngularDeadDistance = 1;
	MaxAngularVelocity  = 60;
	AngularAcceleration = 10;

	// Settings
	PrimaryComponentTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaStationRingComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find our where the socket is
	FTransform SocketTransform = GetSocketTransform("Base", RTS_Component);
	SocketRelativeLocation     = SocketTransform.GetTranslation();
	SocketRelativeRotation     = SocketTransform.GetRotation().Rotator();

	// Find the previous ring
	TArray<UNovaStationRingComponent*> Rings;
	GetOwner()->GetComponents<UNovaStationRingComponent>(Rings);
	for (const UNovaStationRingComponent* Ring : Rings)
	{
		if (Ring->RingIndex == RingIndex - 1)
		{
			PreviousRing = Ring;
			break;
		}
	}
}

void UNovaStationRingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Find a player start to work with
	if (!IsValid(AttachedPlayerStart))
	{
		TArray<AActor*> AttachedActors;
		GetOwner()->GetAttachedActors(AttachedActors);
		for (const AActor* Actor : AttachedActors)
		{
			const ANovaPlayerStart* PlayerStart = Cast<ANovaPlayerStart>(Actor);
			if (PlayerStart)
			{
				AttachedPlayerStart = PlayerStart;
				break;
			}
		}
	}

	// Find a spacecraft to work with
	if (IsValid(AttachedPlayerStart) && !IsValid(AttachedSpacecraft))
	{
		TArray<AActor*> SpacecraftPawns;
		UGameplayStatics::GetAllActorsOfClass(this, ANovaSpacecraftPawn::StaticClass(), SpacecraftPawns);

		for (const AActor* SpacecraftPawn : SpacecraftPawns)
		{
			const UNovaSpacecraftMovementComponent* MovementComponent = Cast<ANovaSpacecraftPawn>(SpacecraftPawn)->GetSpacecraftMovement();
			NCHECK(MovementComponent);

			if (AttachedPlayerStart == MovementComponent->GetPlayerStart())
			{
				AttachedSpacecraft = Cast<ANovaSpacecraftPawn>(SpacecraftPawn);
			}
		}
	}

	// Process the ring logic
	if (IsValid(AttachedSpacecraft) && AttachedSpacecraft->IsIdle())
	{
		const UNovaSpacecraftMovementComponent* MovementComponent = Cast<ANovaSpacecraftPawn>(AttachedSpacecraft)->GetSpacecraftMovement();
		NCHECK(MovementComponent);
		if (MovementComponent->IsDockedOrDocking())
		{
			const USceneComponent* TargetComponent = nullptr;

			FVector  CurrentLocation = GetComponentLocation();
			FRotator CurrentRotation = GetComponentRotation();
			double   TargetLocation  = CurrentLocation.X;
			double   TargetRoll      = CurrentRotation.Roll;

			if (IsValid(PreviousRing))
			{
				TargetLocation = PreviousRing->GetComponentLocation().X - 1000;
			}

			// Target hatch components, or the central structure
			if (RingIndex < AttachedSpacecraft->GetCompartmentComponentCount())
			{
				const UNovaSpacecraftCompartmentComponent* Compartment = AttachedSpacecraft->GetCompartmentComponent(RingIndex);
				NCHECK(Compartment);

				TargetComponent = Compartment->GetMainStructure();

				// Find hatches
				TArray<USceneComponent*> Children;
				Compartment->GetChildrenComponents(true, Children);
				for (const USceneComponent* Child : Children)
				{
					if (Child->IsA<UNovaSpacecraftHatchComponent>())
					{
						TargetComponent = Child->GetAttachParent();
						break;
					}
				}

				// Target hatches
				if (IsValid(TargetComponent))
				{
					// DrawDebugLine(GetWorld(), TargetComponent->GetComponentLocation(),
					//	GetSocketTransform("Base", RTS_World).GetTranslation(), FColor::Red);

					FVector RelativeTargetLocation =
						AttachedSpacecraft->GetTransform().InverseTransformPosition(TargetComponent->GetComponentLocation());
					double Roll = FMath::RadiansToDegrees(FMath::Atan2(RelativeTargetLocation.Y, RelativeTargetLocation.Z));

					TargetLocation = AttachedSpacecraft->GetActorLocation().X + RelativeTargetLocation.X;
					TargetRoll     = Roll;
				}
			}

			// Solve for velocities
			UNovaActorTools::SolveVelocity(CurrentLinearVelocity, 0.0, CurrentLocation.X / 100.0, TargetLocation / 100.0,
				LinearAcceleration, MaxLinearVelocity, LinearDeadDistance, DeltaTime);
			UNovaActorTools::SolveVelocity(CurrentRollVelocity, 0.0, CurrentRotation.Roll, TargetRoll, AngularAcceleration,
				MaxAngularVelocity, AngularDeadDistance, DeltaTime);

			// Integrate velocity to derive position
			CurrentLocation.X += CurrentLinearVelocity * 100.0 * DeltaTime;
			CurrentRotation.Roll += CurrentRollVelocity * DeltaTime;
			SetWorldLocation(CurrentLocation);
			SetWorldRotation(CurrentRotation);
		}
	}
}
