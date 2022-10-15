// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftMiningRigComponent.h"
#include "Spacecraft/System/NovaSpacecraftProcessingSystem.h"
#include "Nova.h"

#include "NiagaraComponent.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftMiningRigComponent::UNovaSpacecraftMiningRigComponent() : Super()
{
	SetAbsolute(false, false, true);
	PrimaryComponentTick.bCanEverTick          = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaSpacecraftMiningRigComponent::SetAdditionalAsset(TSoftObjectPtr<UObject> AdditionalAsset)
{
	DrillingEffect = Cast<UNiagaraSystem>(AdditionalAsset.Get());
}

void UNovaSpacecraftMiningRigComponent::BeginPlay()
{
	Super::BeginPlay();

	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, false);

	// Create effect
	DrillingEffectComponent = NewObject<UNiagaraComponent>(this, UNiagaraComponent::StaticClass());
	NCHECK(DrillingEffectComponent);
	DrillingEffectComponent->RegisterComponent();
	DrillingEffectComponent->SetAsset(DrillingEffect);
	DrillingEffectComponent->SetAutoActivate(false);

	// Attach effect
	FVector  SocketLocation;
	FRotator SocketRotation;
	Cast<UPrimitiveComponent>(GetAttachParent())->GetSocketWorldLocationAndRotation("Dock", SocketLocation, SocketRotation);
	DrillingEffectComponent->SetWorldLocation(SocketLocation);
	DrillingEffectComponent->SetWorldRotation(SocketRotation);
	DrillingEffectComponent->AttachToComponent(this, AttachRules);
}

void UNovaSpacecraftMiningRigComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(GetOwner()) && IsValid(GetAttachParent()))
	{
		const UNovaSpacecraftProcessingSystem* ProcessingSystem = GetOwner()->FindComponentByClass<UNovaSpacecraftProcessingSystem>();
		NCHECK(ProcessingSystem);

		if (ProcessingSystem->IsMiningRigActive() && ProcessingSystem->CanMiningRigBeActive())
		{
			DrillingEffectComponent->Activate();
		}
		else
		{
			DrillingEffectComponent->Deactivate();
		}
	}
}
