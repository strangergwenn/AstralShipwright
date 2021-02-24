// Nova project - Gwennaël Arbona

#include "NovaSpacecraftThrusterComponent.h"
#include "NovaSpacecraftMovementComponent.h"
#include "Nova/Actor/NovaMeshInterface.h"
#include "Nova/Nova.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftThrusterComponent::UNovaSpacecraftThrusterComponent() : Super()
{
	// Settings
	SetAbsolute(false, false, true);
	PrimaryComponentTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaSpacecraftThrusterComponent::SetAdditionalAsset(TSoftObjectPtr<UObject> AdditionalAsset)
{
	ExhaustMesh = Cast<UStaticMesh>(AdditionalAsset.Get());
}

void UNovaSpacecraftThrusterComponent::BeginPlay()
{
	Super::BeginPlay();

	bool                      HasSocket;
	int32                     CurrentSocketIndex = 0;
	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, false);
	INovaMeshInterface*       ParentMesh = Cast<INovaMeshInterface>(GetAttachParent());
	NCHECK(ParentMesh);

	// Find all exhaust sockets
	do
	{
		// Check for the socket's existence
		FString SocketName = FString("Exhaust_") + FString::FromInt(CurrentSocketIndex);
		HasSocket          = ParentMesh->HasSocket(*SocketName);

		if (HasSocket)
		{
			// Create exhaust structure
			FNovaThrusterExhaust Exhaust;
			Exhaust.Name = FName(*SocketName);
			Exhaust.Power.SetPeriod(0.15f);

			// Create mesh
			Exhaust.Mesh = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			NCHECK(Exhaust.Mesh);
			Exhaust.Mesh->RegisterComponent();
			Exhaust.Mesh->SetStaticMesh(ExhaustMesh);
			Exhaust.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			// Attach
			FVector  SocketLocation;
			FRotator SocketRotation;
			Cast<UPrimitiveComponent>(ParentMesh)->GetSocketWorldLocationAndRotation(Exhaust.Name, SocketLocation, SocketRotation);
			Exhaust.Mesh->SetWorldLocation(SocketLocation);
			Exhaust.Mesh->SetWorldRotation(SocketRotation);
			Exhaust.Mesh->AttachToComponent(this, AttachRules);

			// Create material
			Exhaust.Material = UMaterialInstanceDynamic::Create(Exhaust.Mesh->GetMaterial(0), GetWorld());
			NCHECK(Exhaust.Material);
			Exhaust.Mesh->SetMaterial(0.0f, Exhaust.Material);

			// Move on
			ThrusterExhausts.Add(Exhaust);
		}

		CurrentSocketIndex++;

	} while (HasSocket);
}

void UNovaSpacecraftThrusterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Get data
	UNovaSpacecraftMovementComponent* MovementComponent = GetOwner()->FindComponentByClass<UNovaSpacecraftMovementComponent>();
	NCHECK(MovementComponent);

	if (MovementComponent && IsValid(GetAttachParent()))
	{
		INovaMeshInterface* ParentMesh = Cast<INovaMeshInterface>(GetAttachParent());
		NCHECK(ParentMesh);

		// Initialize thrust data
		FVector LinearAcceleration  = MovementComponent->GetThrusterAcceleration();
		FVector AngularAcceleration = MovementComponent->GetThrusterAngularAcceleration();

		// Update all exhaust effects
		for (FNovaThrusterExhaust& Exhaust : ThrusterExhausts)
		{
			float EngineIntensity = 0.0f;

			if (ParentMesh->IsDematerializing())
			{
				EngineIntensity = 0.0f;
			}
			else
			{
				// Get location
				FVector  SocketLocation;
				FRotator SocketRotation;
				Cast<UPrimitiveComponent>(ParentMesh)->GetSocketWorldLocationAndRotation(Exhaust.Name, SocketLocation, SocketRotation);
				FVector EngineDirection = SocketRotation.RotateVector(FVector(1.0f, 0, 0));
				FVector EngineOffset    = (SocketLocation - GetOwner()->GetActorLocation()) / 100;

				// Get linear alpha
				float LinearAlpha = -FVector::DotProduct(EngineDirection, LinearAcceleration.GetSafeNormal());

				// Get Angular alpha
				FVector TorqueDirection = FVector::CrossProduct(EngineOffset, EngineDirection).GetSafeNormal();
				float   AngularAlpha    = 0;
				if (!AngularAcceleration.IsNearlyZero())
				{
					AngularAlpha = FVector::DotProduct(TorqueDirection, AngularAcceleration.GetSafeNormal());
				}

				EngineIntensity = FMath::Max(LinearAlpha + AngularAlpha, 0.0f);
			}

			// Apply power
			Exhaust.Power.Set(EngineIntensity, DeltaTime);
			if (IsValid(Exhaust.Material))
			{
				Exhaust.Material->SetScalarParameterValue("EngineIntensity", Exhaust.Power.Get());
			}
		}
	}
}
