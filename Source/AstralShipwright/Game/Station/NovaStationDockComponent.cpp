// Astral Shipwright - Gwennaël Arbona

#include "NovaStationDockComponent.h"
#include "NovaStationRingComponent.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "UI/Menu/NovaMainMenu.h"

#include "Nova.h"

#include "Neutron/Actor/NeutronActorTools.h"
#include "Neutron/System/NeutronMenuManager.h"

#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaStationDockComponent::UNovaStationDockComponent() : Super(), CurrentLinearVelocity(0)
{
	// Defaults
	LinearDeadDistance = 1;
	MaxLinearVelocity  = 1000;
	LinearAcceleration = 2000;
	CameraBoxExtent    = 500;

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
	UNeutronMenuManager*             MenuManager   = UNeutronMenuManager::Get();

	// Process the dock logic
	if (IsValid(RingComponent) && IsValid(MenuManager))
	{
		const ANovaSpacecraftPawn* Spacecraft = RingComponent->GetCurrentSpacecraft();
		const SNovaMainMenu*       MainMenu   = static_cast<SNovaMainMenu*>(MenuManager->GetMenu().Get());

		// Test for collision
		FVector            CameraLocation;
		FRotator           CameraRotation;
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
		FVector LocalCameraLocation = GetComponentTransform().InverseTransformPosition(CameraLocation);
		FBox    CameraCollider =
			FBox(LocalCameraLocation - CameraBoxExtent * FVector(1, 1, 1), LocalCameraLocation + CameraBoxExtent * FVector(1, 1, 1));

		// Compute visibility filters
		bool CameraIsInside       = GetStaticMesh()->GetBoundingBox().Intersect(CameraCollider);
		bool IsOnAssembly         = MainMenu && MainMenu->IsOnAssemblyMenu();
		bool SpacecraftIsFiltered = IsValid(Spacecraft) && Spacecraft->GetCompartmentFilter() != INDEX_NONE;

		// Update materialization
		if (CameraIsInside || IsOnAssembly || SpacecraftIsFiltered)
		{
			Dematerialize();
		}
		else
		{
			Materialize();
		}

		// Process dock target
		FVector CurrentLocation = GetRelativeLocation();
		double  TargetLocation  = CurrentLocation.Z;
		if (RingComponent->IsDockEnabled())
		{
			const USceneComponent* TargetComponent = RingComponent->GetCurrentTarget();
			TargetLocation                         = SocketRelativeLocation.Y +
			                 RingComponent->GetComponentTransform().InverseTransformPosition(TargetComponent->GetSocketLocation("Dock")).Z;
		}
		else
		{
			TargetLocation = RingComponent->GetSocketTransform("Base", RTS_Component).GetTranslation().Z + SocketRelativeLocation.Y - 1000;
		}

		// Solve for velocities
		UNeutronActorTools::SolveVelocity(CurrentLinearVelocity, 0.0, CurrentLocation.Z, TargetLocation, LinearAcceleration,
			MaxLinearVelocity, LinearDeadDistance, DeltaTime);

		// Integrate velocity to derive position
		CurrentLocation.Z += CurrentLinearVelocity * DeltaTime;
		SetRelativeLocation(CurrentLocation);
	}
};