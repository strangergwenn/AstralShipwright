// Nova project - Gwennaël Arbona

#include "NovaSpacecraftPawn.h"
#include "NovaSpacecraftMovementComponent.h"
#include "NovaSpacecraftCompartmentComponent.h"

#include "System/NovaSpacecraftPropellantSystem.h"

#include "Nova/Actor/NovaMeshInterface.h"

#include "Nova/Game/NovaGameState.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Player/NovaPlayerState.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaAssetManager.h"

#include "Nova/Nova.h"

#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ANovaAssembly"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaSpacecraftPawn::ANovaSpacecraftPawn()
	: Super()
	, AssemblyState(ENovaAssemblyState::Idle)
	, SelfDestruct(false)
	, WaitingAssetLoading(false)
	, HighlightedCompartment(ENovaUIConstants::FadeDurationMinimal)
	, OutlinedCompartment(ENovaUIConstants::FadeDurationMinimal)
	, DisplayFilterType(ENovaAssemblyDisplayFilter::All)
	, DisplayFilterIndex(INDEX_NONE)
	, ImmediateMode(false)
{
	// Setup movement component
	MovementComponent = CreateDefaultSubobject<UNovaSpacecraftMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->SetUpdatedComponent(RootComponent);

	// Setup systems
	PropellantSystem = CreateDefaultSubobject<UNovaSpacecraftPropellantSystem>("PropellantSystem");
	PropellantSystem->SetupAttachment(RootComponent);

	// Defaults
	static ConstructorHelpers::FObjectFinder<UNovaCompartmentDescription> EmptyCompartmentObject(
		TEXT("/Game/Gameplay/Compartments/Empty/Compartment_Empty.Compartment_Empty"));
	EmptyCompartmentDescription = EmptyCompartmentObject.Object;

	// Settings
	bReplicates = true;
	SetReplicatingMovement(false);
	bAlwaysRelevant               = true;
	PrimaryActorTick.bCanEverTick = true;
}

/*----------------------------------------------------
    General gameplay
----------------------------------------------------*/

void ANovaSpacecraftPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Assembly sequence
	if (AssemblyState != ENovaAssemblyState::Idle)
	{
		UpdateAssembly();
	}

	// Idle processing
	else
	{
		if (SelfDestruct)
		{
			NLOG("ANovaSpacecraftPawn::Tick : self-destructing spacecraft");
			Destroy();
		}

		// Fetch the spacecraft from the player state when we are remotely controlled OR no spacecraft was ever set
		else if (!Spacecraft.IsValid() || !IsLocallyControlled())
		{
			const ANovaGameState*   GameState         = GetWorld()->GetGameState<ANovaGameState>();
			const ANovaPlayerState* OwningPlayerState = GetPlayerState<ANovaPlayerState>();

			if (IsValid(GameState))
			{
				// Creating or updating spacecraft
				if (IsValid(OwningPlayerState) && OwningPlayerState->GetSpacecraftIdentifier().IsValid())
				{
					const FNovaSpacecraft* NewSpacecraft = GameState->GetSpacecraft(OwningPlayerState->GetSpacecraftIdentifier());
					if (NewSpacecraft && (!Spacecraft.IsValid() || *NewSpacecraft != *Spacecraft.Get()))
					{
						NLOG("ANovaSpacecraftPawn::Tick : updating spacecraft");
						SetSpacecraft(NewSpacecraft);
						SelfDestruct = false;
					}
				}

				// Destroying spacecraft
				else if (Spacecraft.IsValid() && !IsValid(GetOwner()))
				{
					NLOG("ANovaSpacecraftPawn::Tick : removing spacecraft");
					FNovaSpacecraft EmptySpacecraft;
					SetSpacecraft(&EmptySpacecraft);
					SelfDestruct = true;
				}
			}
		}

		// Update selection
		for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentComponents.Num(); CompartmentIndex++)
		{
			ProcessCompartment(CompartmentComponents[CompartmentIndex], FNovaCompartment(),
				FNovaAssemblyCallback::CreateLambda(
					[&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
					{
						UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Element.Mesh);
						if (PrimitiveComponent)
						{
							int32 StencilValue = 0;

							// Outline only, no highlight
							if (CompartmentIndex == OutlinedCompartment.GetCurrent() &&
								CompartmentIndex != HighlightedCompartment.GetCurrent())
							{
								StencilValue = 1;
							}

							// Outline and highlight
							else if (CompartmentIndex == OutlinedCompartment.GetCurrent() &&
									 CompartmentIndex == HighlightedCompartment.GetCurrent())
							{
								StencilValue = 2;
							}

							// Highlight only
							if (CompartmentIndex != OutlinedCompartment.GetCurrent() &&
								CompartmentIndex == HighlightedCompartment.GetCurrent())
							{
								StencilValue = 3;
							}

							PrimitiveComponent->SetCustomDepthStencilValue(StencilValue);
						}
					}));
		}

		UpdateBounds();
	}

	// Update visual effects
	HighlightedCompartment.Update(DeltaTime);
	OutlinedCompartment.Update(DeltaTime);
}

void ANovaSpacecraftPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Set the correct network role
	if (GetNetMode() != NM_Standalone && Cast<APlayerController>(Controller))
	{
		if (Cast<APlayerController>(Controller)->GetRemoteRole() == ROLE_AutonomousProxy)
		{
			SetAutonomousProxy(true);
		}
		else
		{
			SetAutonomousProxy(false);
		}
	}
}

bool ANovaSpacecraftPawn::HasModifications() const
{
	ANovaPlayerController* PC = GetController<ANovaPlayerController>();
	NCHECK(IsValid(PC) && PC->IsLocalController());

	const FNovaSpacecraft* CurrentSpacecraft = PC->GetSpacecraft();
	return CurrentSpacecraft && Spacecraft.IsValid() && *Spacecraft != *CurrentSpacecraft;
}

void ANovaSpacecraftPawn::Dock(FSimpleDelegate Callback)
{
	SaveSystems();
	MovementComponent->Dock(Callback);
}

void ANovaSpacecraftPawn::Undock(FSimpleDelegate Callback)
{
	LoadSystems();
	MovementComponent->Undock(Callback);
}

bool ANovaSpacecraftPawn::IsDocked() const
{
	return IsValid(MovementComponent) && MovementComponent->GetState() == ENovaMovementState::Docked;
}

void ANovaSpacecraftPawn::LoadSystems()
{
	NLOG("ANovaSpacecraftPawn::LoadSystems ('%s')", *GetRoleString(this));

	if (GetLocalRole() == ROLE_Authority)
	{
		TArray<UActorComponent*> Components = GetComponentsByInterface(UNovaSpacecraftSystemInterface::StaticClass());
		for (UActorComponent* Component : Components)
		{
			INovaSpacecraftSystemInterface* System = Cast<INovaSpacecraftSystemInterface>(Component);
			NCHECK(System);

			const FNovaSpacecraft* SystemSpacecraft = System->GetSpacecraft();
			NCHECK(SystemSpacecraft);

			System->Load(*SystemSpacecraft);
		}
	}
	else
	{
		ServerLoadSystems();
	}
}

void ANovaSpacecraftPawn::ServerLoadSystems_Implementation()
{
	LoadSystems();
}

void ANovaSpacecraftPawn::SaveSystems()
{
	NLOG("ANovaSpacecraftPawn::SaveSystems ('%s')", *GetRoleString(this));

	if (GetLocalRole() == ROLE_Authority)
	{
		TArray<UActorComponent*> Components = GetComponentsByInterface(UNovaSpacecraftSystemInterface::StaticClass());
		for (UActorComponent* Component : Components)
		{
			INovaSpacecraftSystemInterface* System = Cast<INovaSpacecraftSystemInterface>(Component);
			NCHECK(System);

			const FNovaSpacecraft* SystemSpacecraft = System->GetSpacecraft();
			NCHECK(SystemSpacecraft);

			FNovaSpacecraft UpdatedSpacecraft = *SystemSpacecraft;
			System->Save(UpdatedSpacecraft);

			ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
			NCHECK(GameState);
			GameState->UpdateSpacecraft(UpdatedSpacecraft, false);
		}
	}
	else
	{
		ServerSaveSystems();
	}
}

void ANovaSpacecraftPawn::ServerSaveSystems_Implementation()
{
	SaveSystems();
}

/*----------------------------------------------------
    Assembly interface
----------------------------------------------------*/

void ANovaSpacecraftPawn::ApplyAssembly()
{
	NLOG("ANovaAssembly::ApplyAssembly");

	ANovaPlayerController* PC = GetController<ANovaPlayerController>();
	NCHECK(IsValid(PC) && PC->IsLocalController());
	NCHECK(Spacecraft.IsValid());

	PC->UpdateSpacecraft(*Spacecraft.Get());
}

bool ANovaSpacecraftPawn::InsertCompartment(FNovaCompartment Compartment, int32 Index)
{
	NLOG("ANovaAssembly::InsertCompartment %d", Index);

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(Spacecraft.IsValid());
		NCHECK(Compartment.Description);

		Spacecraft->Compartments.Insert(Compartment, Index);
		CompartmentComponents.Insert(CreateCompartment(Compartment), Index);

		return true;
	}

	return false;
}

bool ANovaSpacecraftPawn::RemoveCompartment(int32 Index)
{
	NLOG("ANovaAssembly::RemoveCompartment %d", Index);

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(Spacecraft.IsValid());
		NCHECK(Index >= 0 && Index < Spacecraft->Compartments.Num());
		Spacecraft->Compartments[Index] = FNovaCompartment();

		return true;
	}

	return false;
}

FNovaCompartment& ANovaSpacecraftPawn::GetCompartment(int32 Index)
{
	NCHECK(Spacecraft.IsValid());
	NCHECK(Index >= 0 && Index < Spacecraft->Compartments.Num());
	return Spacecraft->Compartments[Index];
}

const FNovaCompartment& ANovaSpacecraftPawn::GetCompartment(int32 Index) const
{
	NCHECK(Spacecraft.IsValid());
	NCHECK(Index >= 0 && Index < Spacecraft->Compartments.Num());
	return Spacecraft->Compartments[Index];
}

FNovaSpacecraftCompartmentMetrics ANovaSpacecraftPawn::GetCompartmentMetrics(int32 Index) const
{
	if (Spacecraft.IsValid())
	{
		return FNovaSpacecraftCompartmentMetrics(*Spacecraft, Index);
	}

	return FNovaSpacecraftCompartmentMetrics();
}

int32 ANovaSpacecraftPawn::GetCompartmentCount() const
{
	int32 Count = 0;

	if (Spacecraft)
	{
		for (const FNovaCompartment& Assembly : Spacecraft->Compartments)
		{
			if (Assembly.IsValid())
			{
				Count++;
			}
		}
	}

	return Count;
}

int32 ANovaSpacecraftPawn::GetCompartmentIndexByPrimitive(const class UPrimitiveComponent* Component)
{
	int32 Result = 0;
	for (UNovaSpacecraftCompartmentComponent* CompartmentComponent : CompartmentComponents)
	{
		if (Component->IsAttachedTo(CompartmentComponent))
		{
			return Result;
		}
		Result++;
	}

	return INDEX_NONE;
}

void ANovaSpacecraftPawn::SetDisplayFilter(ENovaAssemblyDisplayFilter Filter, int32 CompartmentIndex)
{
	DisplayFilterType  = Filter;
	DisplayFilterIndex = CompartmentIndex;

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		UpdateDisplayFilter();
	}
}

/*----------------------------------------------------
    Compartment assembly internals
----------------------------------------------------*/

void ANovaSpacecraftPawn::SetSpacecraft(const FNovaSpacecraft* NewSpacecraft)
{
	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(NewSpacecraft != nullptr);
		NLOG("ANovaAssembly::SetSpacecraft (%d compartments)", NewSpacecraft->Compartments.Num());

		// Clean up first
		if (Spacecraft)
		{
			for (int32 Index = 0; Index < Spacecraft->Compartments.Num(); Index++)
			{
				RemoveCompartment(Index);
			}
		}

		// Initialize physical compartments
		for (const FNovaCompartment& Assembly : NewSpacecraft->Compartments)
		{
			NCHECK(Assembly.Description);
			CompartmentComponents.Add(CreateCompartment(Assembly));
		}

		// Start assembling, using a copy of the target assembly data
		Spacecraft = NewSpacecraft->GetSharedCopy();
		StartAssemblyUpdate();
	}
}

void ANovaSpacecraftPawn::StartAssemblyUpdate()
{
	NCHECK(AssemblyState == ENovaAssemblyState::Idle);

	// In case we have nothing to de-materialize or load, we will move there
	AssemblyState = ENovaAssemblyState::Moving;

	// Update the spacecraft
	Spacecraft->UpdatePropulsionMetrics();
	Spacecraft->UpdateProceduralElements();

	// De-materialize unwanted compartments
	for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentComponents.Num(); CompartmentIndex++)
	{
		ProcessCompartmentIfDifferent(CompartmentComponents[CompartmentIndex],
			CompartmentIndex < Spacecraft->Compartments.Num() ? Spacecraft->Compartments[CompartmentIndex] : FNovaCompartment(),
			FNovaAssemblyCallback::CreateLambda(
				[&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
				{
					if (Element.Mesh)
					{
						Element.Mesh->Dematerialize(ImmediateMode);
						AssemblyState = ENovaAssemblyState::LoadingDematerializing;
					}
				}));
	}

	// Find new assets to load
	NCHECK(RequestedAssets.Num() == 0);
	for (const FNovaCompartment& Compartment : Spacecraft->Compartments)
	{
		if (Compartment.Description)
		{
			for (FSoftObjectPath Asset : Compartment.Description->GetAsyncAssets())
			{
				RequestedAssets.AddUnique(Asset);
			}
			for (const FNovaCompartmentModule& Module : Compartment.Modules)
			{
				if (Module.Description)
				{
					for (FSoftObjectPath Asset : Module.Description->GetAsyncAssets())
					{
						RequestedAssets.AddUnique(Asset);
					}
				}
			}
			for (const UNovaEquipmentDescription* Equipment : Compartment.Equipments)
			{
				if (Equipment)
				{
					for (FSoftObjectPath Asset : Equipment->GetAsyncAssets())
					{
						RequestedAssets.AddUnique(Asset);
					}
				}
			}
		}
	}

	// Request loading of new assets
	if (RequestedAssets.Num())
	{
		if (ImmediateMode)
		{
			UNovaAssetManager::Get()->LoadAssets(RequestedAssets);
		}
		else
		{
			AssemblyState       = ENovaAssemblyState::LoadingDematerializing;
			WaitingAssetLoading = true;

			UNovaAssetManager::Get()->LoadAssets(RequestedAssets, FStreamableDelegate::CreateLambda(
																	  [&]()
																	  {
																		  WaitingAssetLoading = false;
																	  }));
		}
	}
}

void ANovaSpacecraftPawn::UpdateAssembly()
{
	// Process de-materialization and destruction of previous components, wait asset loading
	if (AssemblyState == ENovaAssemblyState::LoadingDematerializing)
	{
		bool StillWaiting = false;

		// Check completion
		for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentComponents.Num(); CompartmentIndex++)
		{
			ProcessCompartmentIfDifferent(CompartmentComponents[CompartmentIndex],
				CompartmentIndex < Spacecraft->Compartments.Num() ? Spacecraft->Compartments[CompartmentIndex] : FNovaCompartment(),
				FNovaAssemblyCallback::CreateLambda(
					[&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
					{
						if (Element.Mesh == nullptr)
						{
						}
						else if (Element.Mesh->IsDematerialized())
						{
							UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Element.Mesh);
							NCHECK(PrimitiveComponent);

							TArray<USceneComponent*> ChildComponents;
							Cast<UPrimitiveComponent>(Element.Mesh)->GetChildrenComponents(false, ChildComponents);
							for (USceneComponent* ChildComponent : ChildComponents)
							{
								ChildComponent->DestroyComponent();
							}
							PrimitiveComponent->DestroyComponent();

							Element.Mesh = nullptr;
						}
						else
						{
							StillWaiting = true;
						}
					}));
		}

		// Start the animation
		if (!StillWaiting && !WaitingAssetLoading)
		{
			AssemblyState       = ENovaAssemblyState::Moving;
			FVector BuildOffset = FVector::ZeroVector;

			// Compute the initial build offset
			for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentComponents.Num(); CompartmentIndex++)
			{
				const FNovaCompartment& Compartment =
					CompartmentIndex < Spacecraft->Compartments.Num() ? Spacecraft->Compartments[CompartmentIndex] : FNovaCompartment();
				BuildOffset += CompartmentComponents[CompartmentIndex]->GetCompartmentLength(Compartment);
			}
			BuildOffset /= 2;

			// Apply individual offsets to compartments
			for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentComponents.Num(); CompartmentIndex++)
			{
				CompartmentComponents[CompartmentIndex]->SetRequestedLocation(BuildOffset);
				const FNovaCompartment& Compartment =
					CompartmentIndex < Spacecraft->Compartments.Num() ? Spacecraft->Compartments[CompartmentIndex] : FNovaCompartment();
				BuildOffset -= CompartmentComponents[CompartmentIndex]->GetCompartmentLength(Compartment);
			}
		}
	}

	// Wait for the end of the compartment animation
	if (AssemblyState == ENovaAssemblyState::Moving)
	{
		bool StillWaiting = false;
		for (UNovaSpacecraftCompartmentComponent* CompartmentComponent : CompartmentComponents)
		{
			if (!CompartmentComponent->IsAtRequestedLocation())
			{
				StillWaiting = true;
				break;
			}
		}

		if (!StillWaiting || ImmediateMode)
		{
			AssemblyState = ENovaAssemblyState::Building;
		}
	}

	// Run the actual building process
	if (AssemblyState == ENovaAssemblyState::Building)
	{
		// Unload unneeded leftover assets
		for (const FSoftObjectPath& Asset : CurrentAssets)
		{
			if (RequestedAssets.Find(Asset) == INDEX_NONE)
			{
				UNovaAssetManager::Get()->UnloadAsset(Asset);
			}
		}
		CurrentAssets = RequestedAssets;
		RequestedAssets.Empty();

		// Build the assembly
		int32         ValidIndex = 0;
		TArray<int32> RemovedCompartments;
		for (int32 CompartmentIndex = 0; CompartmentIndex < Spacecraft->Compartments.Num(); CompartmentIndex++)
		{
			UNovaSpacecraftCompartmentComponent* PreviousCompartmentComponent =
				CompartmentIndex > 0 ? CompartmentComponents[CompartmentIndex - 1] : nullptr;
			const FNovaCompartment* PreviousCompartment = CompartmentIndex > 0 ? &Spacecraft->Compartments[CompartmentIndex - 1] : nullptr;
			const FNovaCompartment& CurrentCompartment  = Spacecraft->Compartments[CompartmentIndex];

			if (CurrentCompartment.IsValid())
			{
				CompartmentComponents[CompartmentIndex]->BuildCompartment(CurrentCompartment, ValidIndex);
				ValidIndex++;
			}
			else
			{
				RemovedCompartments.Add(CompartmentIndex);
			}
		}

		// Destroy removed compartments
		for (int32 Index : RemovedCompartments)
		{
			Spacecraft->Compartments.RemoveAt(Index);
			CompartmentComponents[Index]->DestroyComponent();
			CompartmentComponents.RemoveAt(Index);
		}

		// Complete the process
		DisplayFilterIndex = FMath::Min(DisplayFilterIndex, Spacecraft->Compartments.Num() - 1);
		AssemblyState      = ENovaAssemblyState::Idle;
		UpdateDisplayFilter();
	}
}

void ANovaSpacecraftPawn::UpdateDisplayFilter()
{
	for (int32 CompartmentIndex = 0; CompartmentIndex < Spacecraft->Compartments.Num(); CompartmentIndex++)
	{
		ProcessCompartment(CompartmentComponents[CompartmentIndex], FNovaCompartment(),
			FNovaAssemblyCallback::CreateLambda(
				[&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
				{
					if (Element.Mesh)
					{
						bool DisplayState          = false;
						bool IsFocusingCompartment = DisplayFilterIndex != INDEX_NONE;

						// Process the filter type
						switch (Element.Type)
						{
							case ENovaAssemblyElementType::Module:
								DisplayState = true;
								break;

							case ENovaAssemblyElementType::Structure:
								DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::ModulesStructure;
								break;

							case ENovaAssemblyElementType::Equipment:
								DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::ModulesStructureEquipments;
								break;

							case ENovaAssemblyElementType::Wiring:
								DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::ModulesStructureEquipmentsWiring;
								break;

							case ENovaAssemblyElementType::Hull:
								DisplayState = DisplayFilterType == ENovaAssemblyDisplayFilter::All;
								break;

							default:
								NCHECK(false);
						}

						// Process the filter index : include the current compartment
						if (IsFocusingCompartment && CompartmentIndex != DisplayFilterIndex)
						{
							DisplayState = false;
						}

						if (DisplayState)
						{
							Element.Mesh->Materialize(ImmediateMode);
						}
						else
						{
							Element.Mesh->Dematerialize(ImmediateMode);
						}
					}
				}));
	}
}

UNovaSpacecraftCompartmentComponent* ANovaSpacecraftPawn::CreateCompartment(const FNovaCompartment& Assembly)
{
	UNovaSpacecraftCompartmentComponent* CompartmentComponent = NewObject<UNovaSpacecraftCompartmentComponent>(this);
	NCHECK(CompartmentComponent);

	// Setup the compartment
	CompartmentComponent->Description = Assembly.Description;
	CompartmentComponent->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
	CompartmentComponent->RegisterComponent();
	CompartmentComponent->SetImmediateMode(ImmediateMode);

	return CompartmentComponent;
}

void ANovaSpacecraftPawn::ProcessCompartmentIfDifferent(
	UNovaSpacecraftCompartmentComponent* CompartmentComponent, const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback)
{
	ProcessCompartment(CompartmentComponent, Compartment,
		FNovaAssemblyCallback::CreateLambda(
			[&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
			{
				if (Element.Asset != Asset.ToSoftObjectPath() || CompartmentComponent->Description != Compartment.Description)
				{
					Callback.Execute(Element, Asset, AdditionalComponent);
				}
			}));
}

void ANovaSpacecraftPawn::ProcessCompartment(
	UNovaSpacecraftCompartmentComponent* CompartmentComponent, const FNovaCompartment& Compartment, FNovaAssemblyCallback Callback)
{
	if (CompartmentComponent->Description)
	{
		CompartmentComponent->ProcessCompartment(Compartment, Callback);
	}
}

void ANovaSpacecraftPawn::UpdateBounds()
{
	CurrentOrigin = FVector::ZeroVector;
	CurrentExtent = FVector::ZeroVector;

	// While focusing a compartment, only account for it
	if (DisplayFilterIndex != INDEX_NONE)
	{
		NCHECK(DisplayFilterIndex >= 0 && DisplayFilterIndex < CompartmentComponents.Num());

		FBox Bounds(ForceInit);
		ForEachComponent<UPrimitiveComponent>(false,
			[&](const UPrimitiveComponent* Prim)
			{
				if (Prim->IsRegistered() && Prim->IsAttachedTo(CompartmentComponents[DisplayFilterIndex]) &&
					Prim->Implements<UNovaMeshInterface>() && !Cast<INovaMeshInterface>(Prim)->IsDematerializing())
				{
					Bounds += Prim->Bounds.GetBox();
				}
			});
		Bounds.GetCenterAndExtents(CurrentOrigin, CurrentExtent);
	}

	// In other case, use an estimate to avoid animation bounces
	else
	{
		FVector Unused;
		GetActorBounds(true, Unused, CurrentExtent);
	}
}

#undef LOCTEXT_NAMESPACE
