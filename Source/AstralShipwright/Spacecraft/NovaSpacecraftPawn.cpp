// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftPawn.h"

#include "NovaSpacecraftCompartmentComponent.h"
#include "NovaSpacecraftHatchComponent.h"
#include "NovaSpacecraftMiningRigComponent.h"
#include "NovaSpacecraftMovementComponent.h"

#include "System/NovaSpacecraftCrewSystem.h"
#include "System/NovaSpacecraftPowerSystem.h"
#include "System/NovaSpacecraftProcessingSystem.h"
#include "System/NovaSpacecraftPropellantSystem.h"

#include "Game/NovaGameState.h"
#include "Player/NovaPlayerController.h"

#include "Nova.h"

#include "Neutron/Actor/NeutronActorTools.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronAssetManager.h"

#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "ANovaAssembly"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaSpacecraftPawn::ANovaSpacecraftPawn()
	: Super()
	, AssemblyState(ENovaAssemblyState::Idle)
	, SelfDestruct(false)
	, EditingSpacecraft(false)

	, WaitingAssetLoading(false)

	, HoveredCompartment(ENeutronUIConstants::FadeDurationMinimal)
	, SelectedCompartment(ENeutronUIConstants::FadeDurationMinimal)

	, DisplayFilterType(ENovaAssemblyDisplayFilter::All)
	, DisplayFilterIndex(INDEX_NONE)
	, ImmediateMode(false)
{
	// Setup movement component
	MovementComponent = CreateDefaultSubobject<UNovaSpacecraftMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->SetUpdatedComponent(RootComponent);

	// Setup systems
	CrewSystem = CreateDefaultSubobject<UNovaSpacecraftCrewSystem>("CrewSystem");
	CrewSystem->SetupAttachment(RootComponent);
	PowerSystem = CreateDefaultSubobject<UNovaSpacecraftPowerSystem>("PowerSystem");
	PowerSystem->SetupAttachment(RootComponent);
	ProcessingSystem = CreateDefaultSubobject<UNovaSpacecraftProcessingSystem>("ProcessingSystem");
	ProcessingSystem->SetupAttachment(RootComponent);
	PropellantSystem = CreateDefaultSubobject<UNovaSpacecraftPropellantSystem>("PropellantSystem");
	PropellantSystem->SetupAttachment(RootComponent);

	// Settings
	bReplicates = true;
	SetReplicatingMovement(false);
	bAlwaysRelevant               = true;
	PrimaryActorTick.bCanEverTick = true;

	// Defaults
	DefaultDistanceFactor = 2.5f;
}

/*----------------------------------------------------
    General gameplay
----------------------------------------------------*/

void ANovaSpacecraftPawn::BeginPlay()
{
	Super::BeginPlay();
}

void ANovaSpacecraftPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update visual effects
	HoveredCompartment.Update(DeltaTime);
	SelectedCompartment.Update(DeltaTime);

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

		// While not updating the spacecraft, always update to the game state
		else if (!IsEditing())
		{
			const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();

			if (IsValid(GameState))
			{
				// Creating or updating spacecraft
				if (RequestedSpacecraftIdentifier.IsValid())
				{
					const FNovaSpacecraft* NewSpacecraft = GameState->GetSpacecraft(RequestedSpacecraftIdentifier);
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
						if (Element.Mesh)
						{
							int32 HoveredValue  = CompartmentIndex == HoveredCompartment.GetCurrent() ? 1 : 0;
							int32 SelectedValue = CompartmentIndex == SelectedCompartment.GetCurrent() ? 1 : 0;

							Element.Mesh->RequestParameter("OutlineAlpha", HoveredValue * HoveredCompartment.GetAlpha(), true);
							Element.Mesh->RequestParameter("HighlightAlpha", SelectedValue * SelectedCompartment.GetAlpha(), true);
							Element.Mesh->RequestParameter("HighlightColor", UNeutronMenuManager::Get()->GetHighlightColor(), true);
						}
					}));
		}

		UpdateBounds();
	}
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

void ANovaSpacecraftPawn::RevertModifications()
{
	SetEditing(false);

	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	if (IsValid(GameState) && RequestedSpacecraftIdentifier.IsValid())
	{
		const FNovaSpacecraft* NewSpacecraft = GameState->GetSpacecraft(RequestedSpacecraftIdentifier);
		if (NewSpacecraft)
		{
			SetSpacecraft(NewSpacecraft);
		}
		else
		{
			Spacecraft.Reset();
		}
	}
}

float ANovaSpacecraftPawn::GetCurrentMass() const
{
	if (Spacecraft.IsValid())
	{
		return Spacecraft->GetPropulsionMetrics().DryMass + Spacecraft->GetCurrentCargoMass() +
		       FindComponentByClass<UNovaSpacecraftPropellantSystem>()->GetCurrentPropellantMass();
	}

	return 0;
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

bool ANovaSpacecraftPawn::IsDocking() const
{
	return IsValid(MovementComponent) && MovementComponent->GetState() == ENovaMovementState::DockingPhase1;
}

const UPrimitiveComponent* ANovaSpacecraftPawn::GetAnchorComponent() const
{
	UNovaSpacecraftMiningRigComponent* AnchorComponent = FindComponentByClass<UNovaSpacecraftMiningRigComponent>();

	return IsValid(AnchorComponent) ? Cast<UPrimitiveComponent>(AnchorComponent->GetAttachParent()) : nullptr;
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
			GameState->UpdatePlayerSpacecraft(UpdatedSpacecraft, false);
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
    Asset getters
----------------------------------------------------*/

TArray<const class UNovaCompartmentDescription*> ANovaSpacecraftPawn::GetCompatibleCompartments(
	const ANovaPlayerController* PC, int32 CompartmentIndex) const
{
	NCHECK(Spacecraft.IsValid());
	if (Spacecraft.IsValid())
	{
		TArray<const class UNovaCompartmentDescription*> Compartments = Spacecraft->GetCompatibleCompartments(CompartmentIndex);

		Compartments = Compartments.FilterByPredicate(
			[PC](const UNovaCompartmentDescription* Module)
			{
				return PC->IsComponentUnlocked(Module);
			});
		return Compartments;
	}
	else
	{
		return TArray<const class UNovaCompartmentDescription*>();
	}
}

TArray<const class UNovaModuleDescription*> ANovaSpacecraftPawn::GetCompatibleModules(
	const ANovaPlayerController* PC, int32 CompartmentIndex, int32 SlotIndex) const
{
	NCHECK(Spacecraft.IsValid());
	if (Spacecraft.IsValid())
	{
		TArray<const class UNovaModuleDescription*> Modules = Spacecraft->GetCompatibleModules(CompartmentIndex, SlotIndex);

		Modules = Modules.FilterByPredicate(
			[PC](const UNovaModuleDescription* Module)
			{
				return Module == nullptr || PC->IsComponentUnlocked(Module);
			});
		return Modules;
	}
	else
	{
		return TArray<const class UNovaModuleDescription*>();
	}
}

TArray<const class UNovaEquipmentDescription*> ANovaSpacecraftPawn::GetCompatibleEquipment(
	const ANovaPlayerController* PC, int32 CompartmentIndex, int32 SlotIndex) const
{
	NCHECK(Spacecraft.IsValid());
	if (Spacecraft.IsValid())
	{
		TArray<const class UNovaEquipmentDescription*> Equipment = Spacecraft->GetCompatibleEquipment(CompartmentIndex, SlotIndex);

		Equipment = Equipment.FilterByPredicate(
			[PC](const UNovaEquipmentDescription* Equipment)
			{
				return Equipment == nullptr || PC->IsComponentUnlocked(Equipment);
			});
		return Equipment;
	}
	else
	{
		return TArray<const class UNovaEquipmentDescription*>();
	}
}

/*----------------------------------------------------
    Assembly interface
----------------------------------------------------*/

void ANovaSpacecraftPawn::ApplyAssembly()
{
	NLOG("ANovaAssembly::ApplyAssembly");

	SetEditing(false);

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

bool ANovaSpacecraftPawn::SwapCompartments(int32 IndexA, int32 IndexB)
{
	NLOG("ANovaAssembly::SwapCompartments %d <-> %d", IndexA, IndexB);

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(Spacecraft.IsValid());
		NCHECK(IndexA >= 0 && IndexA < Spacecraft->Compartments.Num());
		NCHECK(IndexB >= 0 && IndexB < Spacecraft->Compartments.Num());

		Swap(Spacecraft->Compartments[IndexA], Spacecraft->Compartments[IndexB]);

		return true;
	}

	return false;
}

bool ANovaSpacecraftPawn::SwapModules(int32 CompartmentIndex, int32 IndexA, int32 IndexB)
{
	NLOG("ANovaAssembly::SwapModules %d: %d <-> %d", CompartmentIndex, IndexA, IndexB);

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(Spacecraft.IsValid());
		NCHECK(IndexA >= 0 && IndexA < ENovaConstants::MaxModuleCount);
		NCHECK(IndexB >= 0 && IndexB < ENovaConstants::MaxModuleCount);
		NCHECK(CompartmentIndex >= 0 && CompartmentIndex < Spacecraft->Compartments.Num());

		FNovaCompartment& EditedCompartment = Spacecraft->Compartments[CompartmentIndex];
		Swap(EditedCompartment.Modules[IndexA], EditedCompartment.Modules[IndexB]);

		return true;
	}

	return false;
}

bool ANovaSpacecraftPawn::SwapEquipment(int32 CompartmentIndex, int32 IndexA, int32 IndexB)
{
	NLOG("ANovaAssembly::SwapEquipment %d: %d <-> %d", CompartmentIndex, IndexA, IndexB);

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(Spacecraft.IsValid());
		NCHECK(IndexA >= 0 && IndexA < ENovaConstants::MaxEquipmentCount);
		NCHECK(IndexB >= 0 && IndexB < ENovaConstants::MaxEquipmentCount);
		NCHECK(CompartmentIndex >= 0 && CompartmentIndex < Spacecraft->Compartments.Num());

		FNovaCompartment& EditedCompartment = Spacecraft->Compartments[CompartmentIndex];
		Swap(EditedCompartment.Equipment[IndexA], EditedCompartment.Equipment[IndexB]);

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

const UNovaSpacecraftCompartmentComponent* ANovaSpacecraftPawn::GetCompartmentComponent(int32 Index) const
{
	NCHECK(Index >= 0 && Index < CompartmentComponents.Num());
	return CompartmentComponents[Index];
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
		for (const FNovaCompartment& Compartment : Spacecraft->Compartments)
		{
			if (Compartment.IsValid())
			{
				Count++;
			}
		}
	}

	return Count;
}

int32 ANovaSpacecraftPawn::GetCompartmentComponentCount() const
{
	return CompartmentComponents.Num();
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

void ANovaSpacecraftPawn::UpdateCustomization(const struct FNovaSpacecraftCustomization& Customization)
{
	if (Customization != Spacecraft->Customization)
	{
		Spacecraft->Customization = Customization;

		for (UNovaSpacecraftCompartmentComponent* Compartment : CompartmentComponents)
		{
			Compartment->UpdateCustomization();
		}
	}
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

		// Initialize missing physical compartments
		for (int32 CompartmentIndex = CompartmentComponents.Num(); CompartmentIndex < NewSpacecraft->Compartments.Num(); CompartmentIndex++)
		{
			const FNovaCompartment& Assembly = NewSpacecraft->Compartments[CompartmentIndex];
			NCHECK(Assembly.Description);
			CompartmentComponents.Add(CreateCompartment(Assembly));
		}

		// Start assembling, using a copy of the target assembly data
		Spacecraft = NewSpacecraft->GetSharedCopy();
		NCHECK(Spacecraft->Compartments.Num() <= CompartmentComponents.Num());
		StartAssemblyUpdate();
	}
}

void ANovaSpacecraftPawn::StartAssemblyUpdate()
{
	NCHECK(AssemblyState == ENovaAssemblyState::Idle);

	// Update the spacecraft
	Spacecraft->UpdatePropulsionMetrics();
	Spacecraft->UpdatePowerMetrics();
	Spacecraft->UpdateProceduralElements();
	Spacecraft->UpdateModuleGroups();

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
			for (const UNovaEquipmentDescription* Equipment : Compartment.Equipment)
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
			UNeutronAssetManager::Get()->LoadAssets(RequestedAssets);

			MoveCompartments();
			BuildCompartments();

			AssemblyState = ENovaAssemblyState::Idle;
		}
		else
		{
			AssemblyState       = ENovaAssemblyState::LoadingDematerializing;
			WaitingAssetLoading = true;

			UNeutronAssetManager::Get()->LoadAssets(RequestedAssets, FStreamableDelegate::CreateLambda(
																		 [&]()
																		 {
																			 WaitingAssetLoading = false;
																		 }));
		}
	}
	else
	{
		AssemblyState = ENovaAssemblyState::Moving;
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
						{}
						else if (Element.Mesh->IsDematerialized())
						{
							UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Element.Mesh);
							NCHECK(PrimitiveComponent);

							TArray<USceneComponent*> ChildComponents;
							Cast<UPrimitiveComponent>(Element.Mesh)->GetChildrenComponents(true, ChildComponents);
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
			AssemblyState = ENovaAssemblyState::Moving;
			MoveCompartments();
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
		BuildCompartments();
		AssemblyState = ENovaAssemblyState::Idle;
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
							case ENovaAssemblyElementType::Structure:
								DisplayState = true;
								break;

							case ENovaAssemblyElementType::Module:
								DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::StructureModules;
								break;

							case ENovaAssemblyElementType::Equipment:
								DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::StructureModulesEquipment;
								break;

							case ENovaAssemblyElementType::Wiring:
								DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::StructureModulesEquipmentWiring;
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
	if (DisplayFilterIndex != INDEX_NONE || IsDocked())
	{
		FBox Bounds(ForceInit);
		ForEachComponent<UPrimitiveComponent>(false,
			[&](const UPrimitiveComponent* Prim)
			{
				if (Prim->IsRegistered() && (IsDocked() || Prim->IsAttachedTo(CompartmentComponents[DisplayFilterIndex])) &&
					Prim->Implements<UNeutronMeshInterface>() && Cast<INeutronMeshInterface>(Prim)->IsMaterialized())
				{
					Bounds += Prim->Bounds.GetBox();
				}
			});
		Bounds.GetCenterAndExtents(CurrentOrigin, CurrentExtent);
	}

	// In other cases, use a point cloud from component origins because we rotate, and the size doesn't change much
	else
	{
		CurrentOrigin = FVector::ZeroVector;

		TArray<FVector> Origins;
		ForEachComponent<UPrimitiveComponent>(false,
			[&](const UPrimitiveComponent* Prim)
			{
				if (Prim->IsRegistered() && Prim->Implements<UNeutronMeshInterface>() &&
					Cast<INeutronMeshInterface>(Prim)->IsMaterialized())
				{
					Origins.Add(Prim->Bounds.Origin);
				}
			});

		FVector Origin = FVector::ZeroVector;
		for (const FVector& Point : Origins)
		{
			Origin += Point / Origins.Num();
		}

		float Radius = 0;
		for (const FVector& Point : Origins)
		{
			float Distance = (Point - Origin).Size();
			Radius         = FMath::Max(Distance, Radius);
		}

		CurrentExtent = Radius * FVector(1, 1, 1);
	}
}

void ANovaSpacecraftPawn::MoveCompartments()
{
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

void ANovaSpacecraftPawn::BuildCompartments()
{
	// Build the assembly
	int32         ValidIndex = 0;
	TArray<int32> CompartmentIndicesToRemove;
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
			CompartmentIndicesToRemove.Add(CompartmentIndex);
		}
	}

	// Destroy removed compartments
	int32 RemovedComponentCount = 0;
	for (int32 Index : CompartmentIndicesToRemove)
	{
		int32 UpdatedIndex = Index - RemovedComponentCount;
		RemovedComponentCount++;

		Spacecraft->Compartments.RemoveAt(UpdatedIndex);
		CompartmentComponents[UpdatedIndex]->DestroyComponent();
		CompartmentComponents.RemoveAt(UpdatedIndex);
	}

	for (int32 CompartmentIndex = 0; CompartmentIndex < Spacecraft->Compartments.Num(); CompartmentIndex++)
	{
		CompartmentComponents[CompartmentIndex]->Description = Spacecraft->Compartments[CompartmentIndex].Description;
	}

	// Complete the process
	DisplayFilterIndex = FMath::Min(DisplayFilterIndex, Spacecraft->Compartments.Num() - 1);
	UpdateDisplayFilter();

	// Unload unneeded leftover assets
	for (const FSoftObjectPath& Asset : CurrentAssets)
	{
		if (!RequestedAssets.Contains(Asset))
		{
			UNeutronAssetManager::Get()->UnloadAsset(Asset);
		}
	}
	CurrentAssets = RequestedAssets;
	RequestedAssets.Empty();
}

void ANovaSpacecraftPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaSpacecraftPawn, RequestedSpacecraftIdentifier);
}

#undef LOCTEXT_NAMESPACE
