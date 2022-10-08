
#include "NovaSpacecraftSolarPanelComponent.h"

#include "Game/NovaGameState.h"
#include "Game/NovaPlanetarium.h"
#include "Nova.h"

#include "Neutron/Actor/NeutronMeshInterface.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftSolarPanelComponent::UNovaSpacecraftSolarPanelComponent() : Super()
{
	PrimaryComponentTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void UNovaSpacecraftSolarPanelComponent::BeginPlay()
{
	Super::BeginPlay();

	LastUpdateGameTime = 0;
}

void UNovaSpacecraftSolarPanelComponent::TickComponent(
	float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	enum class ERotationMode
	{
		Pitch,
		Yaw,
		Roll
	};

	const FVector                  X(1, 0, 0);
	const FVector                  Y(0, 1, 0);
	const FVector                  Z(0, 0, 1);
	static constexpr double        RotationSpeed = 20.0;
	static constexpr ERotationMode RotationMode  = ERotationMode::Yaw;

	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	// Get the mesh
	USceneComponent* ParentMesh = GetAttachParent();
	NCHECK(ParentMesh);

	// Get the planetarium
	TArray<AActor*> PlanetariumCandidates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPlanetarium::StaticClass(), PlanetariumCandidates);
	NCHECK(PlanetariumCandidates.Num() > 0);
	const ANovaPlanetarium* Planetarium = Cast<ANovaPlanetarium>(PlanetariumCandidates[0]);

	// Determine if the time has changed a lot since last tick
	bool            RequiresInitialOrientation = false;
	const FNovaTime CurrentGameTime            = Cast<ANovaGameState>(GetWorld()->GetGameState())->GetCurrentTime();
	if ((CurrentGameTime - LastUpdateGameTime).AsMinutes() > 1)
	{
		RequiresInitialOrientation = true;
	}
	LastUpdateGameTime = CurrentGameTime;

	// Update
	if (ParentMesh && Planetarium)
	{
		// Rotation axis
		FRotator Axis;
		if (RotationMode == ERotationMode::Pitch)
		{
			Axis = FRotator(1, 0, 0);
		}
		else if (RotationMode == ERotationMode::Yaw)
		{
			Axis = FRotator(0, 1, 0);
		}
		else
		{
			Axis = FRotator(0, 0, 1);
		}

		// Get the local sun direction
		const FVector SunDirection            = Planetarium->GetSunDirection();
		const FVector LocalSunDirection       = ParentMesh->GetComponentToWorld().GetRotation().Inverse().RotateVector(SunDirection);
		FVector       LocalPlanarSunDirection = LocalSunDirection;
		if (RotationMode == ERotationMode::Pitch)
		{
			LocalPlanarSunDirection.Y = 0;
		}
		else if (RotationMode == ERotationMode::Yaw)
		{
			LocalPlanarSunDirection.Z = 0;
		}
		else
		{
			LocalPlanarSunDirection.X = 0;
		}

		// Move to align
		if (!LocalPlanarSunDirection.IsNearlyZero())
		{
			float   Angle        = 0;
			FVector RotationAxis = X;
			LocalPlanarSunDirection.Normalize();

			if (RotationMode == ERotationMode::Pitch)
			{
				Angle        = FMath::RadiansToDegrees(FMath::Atan2(LocalPlanarSunDirection.Z, LocalPlanarSunDirection.X));
				RotationAxis = Y;
			}
			else if (RotationMode == ERotationMode::Yaw)
			{
				Angle        = -FMath::RadiansToDegrees(FMath::Atan2(LocalPlanarSunDirection.X, LocalPlanarSunDirection.Y));
				RotationAxis = Z;
			}
			else
			{
				Angle        = -FMath::RadiansToDegrees(FMath::Atan2(LocalPlanarSunDirection.Y, LocalPlanarSunDirection.Z));
				RotationAxis = X;
			}

			Angle            = FMath::UnwindDegrees(Angle);
			float DeltaAngle = FMath::Sign(Angle) * FMath::Min(RotationSpeed * DeltaSeconds, FMath::Abs(Angle));

			if (RequiresInitialOrientation)
			{
				DeltaAngle                 = Angle;
				RequiresInitialOrientation = false;
			}

			if (FMath::Abs(Angle) > 0.01)
			{
				FRotator Delta = FQuat(RotationAxis, FMath::DegreesToRadians(DeltaAngle)).Rotator();

				ParentMesh->AddLocalRotation(Delta);
			}
		}
	}
}
