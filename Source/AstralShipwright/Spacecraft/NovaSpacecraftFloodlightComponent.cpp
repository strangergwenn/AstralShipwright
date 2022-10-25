// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftFloodlightComponent.h"
#include "Spacecraft/System/NovaSpacecraftPowerSystem.h"
#include "Nova.h"

#include "Neutron/Actor/NeutronMeshInterface.h"

#include "DrawDebugHelpers.h"
#include "Components/SpotLightComponent.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftFloodlightComponent::UNovaSpacecraftFloodlightComponent() : Super()
{}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaSpacecraftFloodlightComponent::BeginPlay()
{
	Super::BeginPlay();

	bool                      HasSocket;
	int32                     CurrentSocketIndex = 0;
	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, false);
	INeutronMeshInterface*    ParentMesh = Cast<INeutronMeshInterface>(GetAttachParent());
	NCHECK(ParentMesh);

	FLinearColor LightColor = FLinearColor(1.0f, 0.7f, 0.50f);

	// Find all light sockets
	do
	{
		// Check for the socket's existence
		FString SocketName = FString("Light_") + FString::FromInt(CurrentSocketIndex);
		HasSocket          = ParentMesh->HasSocket(*SocketName);

		if (HasSocket)
		{
			// Create mesh
			USpotLightComponent* Light = NewObject<USpotLightComponent>(this, USpotLightComponent::StaticClass());
			NCHECK(Light);
			Light->RegisterComponent();

			// Attach
			FVector  SocketLocation;
			FRotator SocketRotation;
			Cast<UPrimitiveComponent>(ParentMesh)->GetSocketWorldLocationAndRotation(*SocketName, SocketLocation, SocketRotation);
			Light->SetWorldLocation(SocketLocation);
			Light->SetWorldRotation(SocketRotation);
			Light->AttachToComponent(this, AttachRules);

			// Configure
			Light->SetLightColor(LightColor);
			Light->SetAttenuationRadius(10000.0f);
			Light->SetOuterConeAngle(90.0f);
			Light->SetInnerConeAngle(45.0f);
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetCastShadows(false);
			Light->SetActive(true);

			// Move on
			Lights.Add(Light);
		}

		CurrentSocketIndex++;

	} while (HasSocket);

	// Configure the main light too
	ParentMesh->RequestParameter("LightColor", LightColor);
	LightIntensity.SetPeriod(0.1f);
}

void UNovaSpacecraftFloodlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Get data
	UNovaSpacecraftPowerSystem* PowerSystem = GetOwner()->FindComponentByClass<UNovaSpacecraftPowerSystem>();
	NCHECK(PowerSystem);
	INeutronMeshInterface* ParentMesh = Cast<INeutronMeshInterface>(GetAttachParent());
	NCHECK(ParentMesh);

	// Update lights
	if (PowerSystem && ParentMesh)
	{
		LightIntensity.Set(PowerSystem->GetRemainingEnergy() > 0 ? 1.0f : 0.0f, DeltaTime);

		ParentMesh->RequestParameter("LightIntensity", 50.0f * LightIntensity.Get());

		for (USpotLightComponent* Light : Lights)
		{
			Light->SetIntensity(10000000.0f * LightIntensity.Get());
		}
	}
}
