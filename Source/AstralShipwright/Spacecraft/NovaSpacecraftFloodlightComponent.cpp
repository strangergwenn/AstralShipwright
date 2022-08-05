// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftFloodlightComponent.h"
#include "Nova.h"

#include "Neutron/Actor/NeutronMeshInterface.h"

#include "DrawDebugHelpers.h"
#include "Components/SpotLightComponent.h"

#define SHOW_TRACES 0

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftFloodlightComponent::UNovaSpacecraftFloodlightComponent() : Super()
{
	// Settings
	SetAbsolute(false, false, true);
	PrimaryComponentTick.bCanEverTick = true;
}

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
			Light->SetIntensity(1000000.0f);
			Light->SetAttenuationRadius(5000.0f);
			Light->SetOuterConeAngle(90.0f);
			Light->SetInnerConeAngle(10.0f);
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetCastShadows(true);
			Light->SetActive(true);

			// Move on
			Lights.Add(Light);
		}

		CurrentSocketIndex++;

	} while (HasSocket);

	// Configure the main light too
	ParentMesh->RequestParameter("LightColor", LightColor);
	ParentMesh->RequestParameter("LightIntensity", 20.0f);
}

void UNovaSpacecraftFloodlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(GetAttachParent()))
	{
		// Update all exhaust effects
		for (USpotLightComponent* Light : Lights)
		{
			// Light->SetActive(true);
		}
	}
}
