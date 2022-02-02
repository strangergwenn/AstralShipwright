// Astral Shipwright - Gwennaël Arbona

#include "NovaStationRingComponent.h"

#include "Actor/NovaActorTools.h"
#include "Actor/NovaPlayerStart.h"

#include "Spacecraft/NovaSpacecraftCompartmentComponent.h"
#include "Spacecraft/NovaSpacecraftHatchComponent.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"
#include "Spacecraft/NovaSpacecraftPawn.h"

#include "Nova.h"

#include "DrawDebugHelpers.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaStationRingComponent::UNovaStationRingComponent() : Super(), CurrentLinearVelocity(0), CurrentRollVelocity(0)
{
	// Defaults
	LinearDeadDistance  = 1;
	MaxLinearVelocity   = 5000;
	LinearAcceleration  = 1000;
	AngularDeadDistance = 0.01;
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

	// Initialize state
	TargetComponent        = nullptr;
	TargetComponentIsHatch = false;
	CurrentLinearDistance  = 0;
	CurrentRollDistance    = 0;

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
	if (IsOperating())
	{
		FVector  CurrentLocation = GetComponentLocation();
		FRotator CurrentRotation = GetComponentRotation();
		double   TargetLocation  = CurrentLocation.X;
		double   TargetRoll      = CurrentRotation.Roll;

		if (IsValid(PreviousRing))
		{
			TargetLocation = PreviousRing->GetComponentLocation().X - 1000;
		}

		// Target hatch components, or the central structure
		int32 CompartmentIndex = RingIndex - (ENovaConstants::MaxCompartmentCount - AttachedSpacecraft->GetCompartmentComponentCount()) / 2;
		if (CompartmentIndex >= 0 && CompartmentIndex < AttachedSpacecraft->GetCompartmentCount())
		{
			const UNovaSpacecraftCompartmentComponent* Compartment = AttachedSpacecraft->GetCompartmentComponent(CompartmentIndex);
			NCHECK(Compartment);

			TargetComponent = Compartment->GetMainStructure();

			// Find hatches
			TArray<USceneComponent*> Children;
			Compartment->GetChildrenComponents(true, Children);
			for (const USceneComponent* Child : Children)
			{
				if (Child->IsA<UNovaSpacecraftHatchComponent>())
				{
					TargetComponent        = Child->GetAttachParent();
					TargetComponentIsHatch = true;
					break;
				}
			}

			// Target hatches
			if (IsValid(TargetComponent))
			{
				FVector RelativeTargetLocation =
					AttachedSpacecraft->GetTransform().InverseTransformPosition(TargetComponent->GetSocketLocation("Dock"));
				TargetLocation = AttachedSpacecraft->GetActorLocation().X + RelativeTargetLocation.X;

				if (TargetComponentIsHatch)
				{
					TargetRoll = FMath::RadiansToDegrees(FMath::Atan2(RelativeTargetLocation.Y, RelativeTargetLocation.Z));
					if (TargetRoll >= 180.0)
					{
						TargetRoll -= 360.0;
					}
				}
			}
		}

		// Solve for velocities
		CurrentLinearDistance = UNovaActorTools::SolveVelocity(CurrentLinearVelocity, 0.0, CurrentLocation.X, TargetLocation,
			LinearAcceleration, MaxLinearVelocity, LinearDeadDistance, DeltaTime);
		CurrentRollDistance   = UNovaActorTools::SolveVelocity(CurrentRollVelocity, 0.0, CurrentRotation.Roll, TargetRoll,
            AngularAcceleration, MaxAngularVelocity, AngularDeadDistance, DeltaTime);

		// Integrate velocity to derive position
		CurrentLocation.X += CurrentLinearVelocity * DeltaTime;
		CurrentRotation.Roll += CurrentRollVelocity * DeltaTime;
		SetWorldLocation(CurrentLocation);
		SetWorldRotation(CurrentRotation);
	}
}

bool UNovaStationRingComponent::IsOperating() const
{
	if (IsValid(AttachedSpacecraft) && AttachedSpacecraft->IsIdle())
	{
		const UNovaSpacecraftMovementComponent* MovementComponent = Cast<ANovaSpacecraftPawn>(AttachedSpacecraft)->GetSpacecraftMovement();
		NCHECK(MovementComponent);

		return MovementComponent->IsDocked();
	}

	return false;
}

bool UNovaStationRingComponent::IsDockEnabled() const
{
	return IsOperating() && IsValid(TargetComponent) && IsCurrentTargetHatch() && CurrentLinearDistance < LinearDeadDistance &&
		   CurrentRollDistance < AngularDeadDistance;
}
