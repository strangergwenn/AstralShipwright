// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftPowerSystem.h"
#include "NovaSpacecraftProcessingSystem.h"
#include "Spacecraft/NovaSpacecraftPawn.h"

#include "Game/NovaGameState.h"
#include "Game/NovaOrbitalSimulationComponent.h"
#include "Game/NovaOrbitalSimulationTypes.h"

#include "Nova.h"

#include "Neutron/System/NeutronAssetManager.h"

#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftPowerSystem::UNovaSpacecraftPowerSystem()
	: Super()

	, CurrentPower(0)
	, CurrentPowerProduction(0)
	, CurrentExposureRatio(0)
	, CurrentEnergy(0)
	, EnergyCapacity(0)
{
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    System implementation
----------------------------------------------------*/

void UNovaSpacecraftPowerSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	// Get game pointers
	const FNovaSpacecraft*                 Spacecraft        = GetSpacecraft();
	const ANovaGameState*                  GameState         = GetWorld()->GetGameState<ANovaGameState>();
	const UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	const UNovaSpacecraftProcessingSystem* ProcessingSystem =
		GameState->GetSpacecraftSystem<UNovaSpacecraftProcessingSystem>(GetSpacecraft());
	class UNovaSpacecraftMovementComponent* SpacecraftMovement = Cast<ANovaSpacecraftPawn>(GetOwner())->GetSpacecraftMovement();

	// Reset stats
	CurrentPower           = 0;
	CurrentPowerProduction = 0;
	CurrentExposureRatio   = 0;

	// Compute solar exposure
	double CurrentExposure = 0;
	if (Spacecraft)
	{
		// Get the relevant planetary bodies
		const UNovaCelestialBody* SunBody    = nullptr;
		const UNovaCelestialBody* PlanetBody = UNeutronAssetManager::Get()->GetDefaultAsset<UNovaCelestialBody>();
		for (const UNovaCelestialBody* Body : UNeutronAssetManager::Get()->GetAssets<UNovaCelestialBody>())
		{
			if (Body->Body == nullptr)
			{
				SunBody = Body;
				break;
			}
		}
		NCHECK(SunBody);
		NCHECK(PlanetBody);

		// Compute the planet's occlusion
		const FNovaOrbitalLocation* PlayerLocation = OrbitalSimulation->GetSpacecraftLocation(Spacecraft->Identifier);
		if (PlayerLocation)
		{
			FVector2D SpacecraftCartesianLocation = PlayerLocation->GetCartesianLocation();

			// Fetch basic data
			const double SunDistanceFromPlanetKm = PlanetBody->Altitude.GetValue();
			const double PlanetOcclusionHalfAngle =
				FMath::RadiansToDegrees(FMath::Asin(PlanetBody->Radius / SpacecraftCartesianLocation.Size()));
			const double PlayerRotationAngle =
				FMath::RadiansToDegrees(FVector(SpacecraftCartesianLocation.X, SpacecraftCartesianLocation.Y, 0).HeadingAngle());

			// Process angles into an exposure value using an arbitrary smoothing value
			const double AngularDistance     = FMath::Abs(PlayerRotationAngle + 90);
			const double ExposureCurveLength = 0.15 * PlanetOcclusionHalfAngle;
			CurrentExposureRatio = FMath::Clamp(FMath::Abs(AngularDistance - PlanetOcclusionHalfAngle) / ExposureCurveLength, 0.0, 1.0);

			// NLOG("%f - %f (%f) = %f -> %f", AngularDistance, PlanetOcclusionHalfAngle, ExposureCurveLength,
			//	FMath::Abs(AngularDistance - PlanetOcclusionHalfAngle), CurrentExposureRatio);
		}
	}

	if (!IsSpacecraftDocked())
	{
		// Handle power usage from groups
		for (int32 GroupIndex = 0; GroupIndex < ProcessingSystem->GetProcessingGroupCount(); GroupIndex++)
		{
			CurrentPower -= ProcessingSystem->GetPowerUsage(GroupIndex);
		}

		const auto& Compartments = GetSpacecraft()->Compartments;
		for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
		{
			// Iterate over modules for *production* only, never consumption that we already handled per group
			for (const FNovaCompartmentModule& CompartmentModule : Compartments[CompartmentIndex].Modules)
			{
				const UNovaProcessingModuleDescription* Module = Cast<UNovaProcessingModuleDescription>(CompartmentModule.Description);
				if (::IsValid(Module))
				{
					CurrentPower += FMath::Max(-Module->Power, 0);
				}
			}

			// Iterate over equipment
			for (const UNovaEquipmentDescription* Equipment : Compartments[CompartmentIndex].Equipment)
			{
				// Solar panels
				const UNovaPowerEquipmentDescription* PowerEquipment = Cast<UNovaPowerEquipmentDescription>(Equipment);
				if (PowerEquipment)
				{
					CurrentPower += CurrentExposureRatio * PowerEquipment->Power;
					CurrentPowerProduction += CurrentExposureRatio * PowerEquipment->Power;
				}

				// Mining rig
				const UNovaMiningEquipmentDescription* MiningEquipment = Cast<UNovaMiningEquipmentDescription>(Equipment);
				if (MiningEquipment && ProcessingSystem->IsMiningRigActive())
				{
					CurrentPower -= MiningEquipment->Power;
				}

				// Mast
				const UNovaRadioMastDescription* MastEquipment = Cast<UNovaRadioMastDescription>(Equipment);
				if (MastEquipment && IsValid(GameState) && IsValid(GameState->GetCurrentArea()) && GameState->GetCurrentArea()->IsInSpace &&
					IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() >= ENovaMovementState::Orbiting)
				{
					CurrentPower -= MastEquipment->Power;
				}
			}
		}

		// Handle batteries
		CurrentEnergy += CurrentPower * (FinalTime - InitialTime).AsHours();
		CurrentEnergy = FMath::Clamp(CurrentEnergy, 0.0, EnergyCapacity);
		if (CurrentPower > 0 && CurrentEnergy >= EnergyCapacity)
		{
			CurrentPower = 0;
		}
	}
}

void UNovaSpacecraftPowerSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftPowerSystem, CurrentPower);
	DOREPLIFETIME(UNovaSpacecraftPowerSystem, CurrentPowerProduction);
	DOREPLIFETIME(UNovaSpacecraftPowerSystem, CurrentExposureRatio);
	DOREPLIFETIME(UNovaSpacecraftPowerSystem, CurrentEnergy);
	DOREPLIFETIME(UNovaSpacecraftPowerSystem, EnergyCapacity);
}
