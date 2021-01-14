// Nova project - Gwennaël Arbona

#include "NovaMeshInterface.h"
#include "Nova/UI/NovaUITypes.h"

#include "Components/PrimitiveComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

FNovaMeshInterfaceBehavior::FNovaMeshInterfaceBehavior()
	: ComponentMaterial(nullptr)
	, CurrentMaterializationState(true)
	, CurrentMaterializationTime(0)
{
	MaterializationDuration = 0.75f;
	ParameterFadeDuration = 0.5f;
}


/*----------------------------------------------------
	Implementation
----------------------------------------------------*/

void FNovaMeshInterfaceBehavior::SetupMaterial(UPrimitiveComponent* Mesh, UMaterialInterface* Material)
{
	ComponentMaterial = UMaterialInstanceDynamic::Create(Material, Mesh);
	Mesh->SetMaterial(0, ComponentMaterial);
}

void FNovaMeshInterfaceBehavior::SetupMaterial(UDecalComponent* Decal, UMaterialInterface* Material)
{
	ComponentMaterial = UMaterialInstanceDynamic::Create(Material, Decal);
}

void FNovaMeshInterfaceBehavior::TickMaterial(float DeltaTime)
{
	// Process materialization
	CurrentMaterializationTime += (DeltaTime / MaterializationDuration) * (CurrentMaterializationState ? 1 : -1);
	CurrentMaterializationTime = FMath::Clamp(CurrentMaterializationTime, 0.0f, MaterializationDuration);
	if (ComponentMaterial)
	{
		float Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentMaterializationTime / MaterializationDuration, ENovaUIConstants::EaseStandard);
		ComponentMaterial->SetScalarParameterValue("MaterializeAlpha", Alpha);
	}

	// Process requests
	TArray<FNovaMaterialParameterRequest> OngoingRequests;
	for (FNovaMaterialParameterRequest& Request : CurrentRequests)
	{
		Request.Time = FMath::Clamp(Request.Time + DeltaTime, 0.0f, ParameterFadeDuration);

		if (Request.IsColor)
		{
			FLinearColor Value = FMath::InterpEaseInOut(PreviousParameterColorValues[Request.Name], Request.ColorValue, Request.Time / ParameterFadeDuration, ENovaUIConstants::EaseStandard);
			ComponentMaterial->SetVectorParameterValue(Request.Name, Value);

			if (Request.Time >= ParameterFadeDuration)
			{
				PreviousParameterColorValues[Request.Name] = Value;
			}
		}
		else
		{
			float Value = FMath::InterpEaseInOut(PreviousParameterFloatValues[Request.Name], Request.FloatValue, Request.Time / ParameterFadeDuration, ENovaUIConstants::EaseStandard);
			ComponentMaterial->SetScalarParameterValue(Request.Name, Value);

			if (Request.Time >= ParameterFadeDuration)
			{
				PreviousParameterFloatValues[Request.Name] = Value;
			}
		}

		if (Request.Time < ParameterFadeDuration)
		{
			OngoingRequests.Add(Request);
		}
	}
	CurrentRequests = OngoingRequests;
}

void FNovaMeshInterfaceBehavior::Materialize(bool Force)
{
	CurrentMaterializationState = true;

	if (Force)
	{
		CurrentMaterializationTime = MaterializationDuration;
	}
}

void FNovaMeshInterfaceBehavior::Dematerialize(bool Force)
{
	CurrentMaterializationState = false;

	if (Force)
	{
		CurrentMaterializationTime = 0;
	}
}

float FNovaMeshInterfaceBehavior::GetMaterializationAlpha() const
{
	return CurrentMaterializationTime / MaterializationDuration;
}

void FNovaMeshInterfaceBehavior::RequestParameter(FName Name, float Value, bool Immediate)
{
	if (Immediate)
	{
		ComponentMaterial->SetScalarParameterValue(Name, Value);

		NLOG("FNovaMeshInterfaceBehavior::RequestParameter : immediate value %.2f for '%s'",
			Value, *Name.ToString());
	}
	else
	{
		FNovaMaterialParameterRequest Request(Name, Value);
		int32 RequestIndex = CurrentRequests.Find(Request);
		float PreviousValue = Value;
		ComponentMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(Name), PreviousValue);

		if (RequestIndex != INDEX_NONE)
		{
			NLOG("FNovaMeshInterfaceBehavior::RequestParameter : ignoring value %.2f for '%s'", Value, *Name.ToString());
			return;
		}
		else if (PreviousParameterFloatValues.Find(Name) == nullptr)
		{
			PreviousParameterFloatValues.Add(Name, PreviousValue);
		}

		NLOG("FNovaMeshInterfaceBehavior::RequestParameter : new value %.2f for '%s' (previously %.2f)",
			Value, *Name.ToString(), PreviousValue);

		CurrentRequests.Add(Request);
	}
}

void FNovaMeshInterfaceBehavior::RequestParameter(FName Name, FLinearColor Value, bool Immediate)
{
	if (Immediate)
	{
		ComponentMaterial->SetVectorParameterValue(Name, Value);

		NLOG("FNovaMeshInterfaceBehavior::RequestParameter : immediate value %.2f, %.2f, %.2f, %.2f for '%s'",
			Value.R, Value.G, Value.B, Value.A, *Name.ToString());
	}
	else
	{
		FNovaMaterialParameterRequest Request(Name, Value);
		int32 RequestIndex = CurrentRequests.Find(Request);
		FLinearColor PreviousValue = Value;
		ComponentMaterial->GetVectorParameterValue(FHashedMaterialParameterInfo(Name), PreviousValue);

		if (RequestIndex != INDEX_NONE)
		{
			NLOG("FNovaMeshInterfaceBehavior::RequestParameter : ignoring value %.2f, %.2f, %.2f, %.2f for '%s'",
				Value.R, Value.G, Value.B, Value.A, *Name.ToString());

			return;
		}
		else if (PreviousParameterColorValues.Find(Name) == nullptr)
		{
			PreviousParameterColorValues.Add(Name, PreviousValue);
		}

		NLOG("FNovaMeshInterfaceBehavior::RequestParameter : new value %.2f, %.2f, %.2f, %.2f for '%s' (previously %.2f, %.2f, %.2f, %.2f)",
			Value.R, Value.G, Value.B, Value.A, *Name.ToString(), PreviousValue.R, PreviousValue.G, PreviousValue.B, PreviousValue.A);

		CurrentRequests.Add(Request);
	}
}

FVector INovaMeshInterface::GetExtent() const
{
	const UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(this);

	if (RootComponent)
	{
		FBox OwnerBounds = RootComponent->Bounds.GetBox();
		return OwnerBounds.GetExtent();
	}

	return FVector::ZeroVector;
}


/*----------------------------------------------------
	Full-collision component movement
----------------------------------------------------*/

static void PullBackHit(FHitResult& Hit, const FVector& Start, const FVector& End, const float Dist)
{
	const float DesiredTimeBack = FMath::Clamp(0.1f, 0.1f / Dist, 1.f / Dist) + 0.001f;
	Hit.Time = FMath::Clamp(Hit.Time - DesiredTimeBack, 0.f, 1.f);
}

static bool IsValidHitResult(const UWorld* InWorld, FHitResult const& TestHit, FVector const& MovementDirDenormalized)
{
	if (TestHit.bBlockingHit && TestHit.bStartPenetrating && (TestHit.ImpactNormal | MovementDirDenormalized.GetSafeNormal()) > 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool INovaMeshInterface::MoveComponentHierarchy(UPrimitiveComponent* RootComponent, const FVector& OriginalLocation,
	const FVector& Delta, const FQuat& NewRotationQuat,
	bool bSweep, FHitResult* OutHit, ETeleportType Teleport,
	FInternalSetWorldLocationAndRotation MovementCallback)
{
	bool Moved = false;

	// This is the actual override going on - needs to be the root component
	if (bSweep && OutHit && !OutHit->IsValidBlockingHit())
	{
		NCHECK(RootComponent);
		NCHECK(RootComponent->GetOwner());
		NCHECK(RootComponent == RootComponent->GetOwner()->GetRootComponent());

		TArray<UPrimitiveComponent*> ChildComponents;
		RootComponent->GetOwner()->GetComponents<UPrimitiveComponent>(ChildComponents);

		// Add attached components 
		TArray<AActor*> AttachedActors;
		RootComponent->GetOwner()->GetAttachedActors(AttachedActors);
		for (AActor* Actor : AttachedActors)
		{
			TArray<UPrimitiveComponent*> PrimitiveComponents;
			Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
			ChildComponents.Append(PrimitiveComponents);
		}

		TArray<FHitResult> ChildComponentHits;
		for (UActorComponent* Component : ChildComponents)
		{
			// Ignore irrelevant actors
			FComponentQueryParams Params(SCENE_QUERY_STAT(MoveComponent), Component->GetOwner());
			Params.AddIgnoredActor(RootComponent->GetOwner());
			Params.AddIgnoredActors(RootComponent->MoveIgnoreActors);
			Params.bTraceComplex = true;
			FCollisionResponseParams ResponseParam;

			// Build trace parameters
			UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
			const FVector ChildTraceStart = PrimitiveComponent->GetComponentLocation();
			const FVector ChildTraceEnd = ChildTraceStart + Delta;

			// Trace
			PrimitiveComponent->InitSweepCollisionParams(Params, ResponseParam);
			RootComponent->GetWorld()->ComponentSweepMulti(ChildComponentHits, PrimitiveComponent,
				ChildTraceStart, ChildTraceEnd, PrimitiveComponent->GetComponentRotation(), Params);

			// Handle hit results
			if (ChildComponentHits.Num())
			{
				for (int32 HitIdx = 0; HitIdx < ChildComponentHits.Num(); HitIdx++)
				{
					PullBackHit(ChildComponentHits[HitIdx], ChildTraceStart, ChildTraceEnd, Delta.Size());
				}

				// Look for the most movement opposing hit normal
				int32 BlockingHitIndex = INDEX_NONE;
				float BlockingHitNormalDotDelta = BIG_NUMBER;
				for (int32 HitIdx = 0; HitIdx < ChildComponentHits.Num(); HitIdx++)
				{
					const FHitResult& TestHit = ChildComponentHits[HitIdx];

					if (TestHit.bBlockingHit)
					{
						if (IsValidHitResult(RootComponent->GetWorld(), TestHit, Delta))
						{
							if (TestHit.Time == 0.f)
							{
								const float NormalDotDelta = (TestHit.ImpactNormal | Delta);
								if (NormalDotDelta < BlockingHitNormalDotDelta)
								{
									BlockingHitNormalDotDelta = NormalDotDelta;
									BlockingHitIndex = HitIdx;
								}
							}
							else if (BlockingHitIndex == INDEX_NONE)
							{
								BlockingHitIndex = HitIdx;
								break;
							}
						}
					}
				}

				if (BlockingHitIndex >= 0)
				{
					*OutHit = ChildComponentHits[BlockingHitIndex];
				}

				// We found a blocking hit : set the location back again
				if (OutHit->bBlockingHit)
				{
					/*VLOG("Actor '%s' with component '%s' collided with actor '%s' on component '%s'",
						*Component->GetOwner()->GetName(),
						*Component->GetName(),
						*OutHit->GetActor()->GetName(),
						*OutHit->GetComponent()->GetName());*/

					FVector NewLocation = OriginalLocation + (OutHit->Time * Delta);
					const float MinMovementDistSq = (bSweep ? FMath::Square(4.f * KINDA_SMALL_NUMBER) : 0.f);

					const FVector ToNewLocation = (NewLocation - OriginalLocation);
					if (ToNewLocation.SizeSquared() <= MinMovementDistSq)
					{
						NewLocation = OriginalLocation;
						OutHit->Time = 0.f;
					}

					Moved = MovementCallback.Execute(NewLocation, NewRotationQuat, Teleport);
				}
			}
		}
	}

	return Moved;
}


#undef LOCTEXT_NAMESPACE
