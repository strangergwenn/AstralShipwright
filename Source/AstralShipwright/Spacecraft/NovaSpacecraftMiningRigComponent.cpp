// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftMiningRigComponent.h"
#include "Spacecraft/System/NovaSpacecraftProcessingSystem.h"
#include "Nova.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftMiningRigComponent::UNovaSpacecraftMiningRigComponent() : Super()
{
	SetAbsolute(false, false, true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrimaryComponentTick.bCanEverTick = true;
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

	SetAsset(DrillingEffect);
}

void UNovaSpacecraftMiningRigComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(GetOwner()) && IsValid(GetAttachParent()))
	{
		const UNovaSpacecraftProcessingSystem* ProcessingSystem = GetOwner()->FindComponentByClass<UNovaSpacecraftProcessingSystem>();
		NCHECK(ProcessingSystem);

		if (ProcessingSystem->IsMiningRigActive())
		{
			Activate();
		}
		else
		{
			Deactivate();
		}
	}
}
