// Nova project - Gwennaël Arbona

#include "NovaPlayerStart.h"
#include "Nova/Nova.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaPlayerStart::ANovaPlayerStart(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Create waiting point
	WaitingPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WaitingPoint"));
	WaitingPoint->SetupAttachment(RootComponent);
	WaitingPoint->SetRelativeLocation(FVector(10000, 0, 0));

	// Settings
	bAlwaysRelevant = true;
#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
#endif

	// Defaults
	IsInSpace = false;
}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

#if WITH_EDITOR

void ANovaPlayerStart::Tick(float DeltaTime)
{
	constexpr float OrbitCylinderLength = 10000;
	constexpr float OrbitCylinderRadius = 1000;
	const FColor    PathColor           = IsSelectedInEditor() ? FColor::Yellow : FColor::White;

	if (!IsHidden() && (GetWorld()->WorldType == EWorldType::Editor || GetWorld()->WorldType == EWorldType::EditorPreview))
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), WaitingPoint->GetComponentLocation(), PathColor, false);
		DrawDebugSphere(GetWorld(), WaitingPoint->GetComponentLocation(), OrbitCylinderRadius, 16, PathColor, false);
		DrawDebugCylinder(GetWorld(), GetWaitingPointLocation() - FVector(0, OrbitCylinderLength, 0),
			GetWaitingPointLocation() + FVector(0, OrbitCylinderLength, 0), OrbitCylinderRadius, 16, PathColor, false);
	}
}

#endif
