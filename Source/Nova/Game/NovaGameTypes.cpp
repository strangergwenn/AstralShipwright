// Nova project - Gwennaël Arbona

#include "NovaGameTypes.h"
#include "Nova/Spacecraft/NovaCaptureActor.h"
#include "Nova/Nova.h"

#include "Engine/Engine.h"

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNovaAssetDescription::UpdateAssetRender()
{
#if WITH_EDITOR

	// Find a capture actor
	TArray<AActor*> CaptureActors;
	for (const FWorldContext& World : GEngine->GetWorldContexts())
	{
		UGameplayStatics::GetAllActorsOfClass(World.World(), ANovaCaptureActor::StaticClass(), CaptureActors);
		if (CaptureActors.Num())
		{
			break;
		}
	}
	NCHECK(CaptureActors.Num());
	ANovaCaptureActor* CaptureActor = Cast<ANovaCaptureActor>(CaptureActors[0]);
	NCHECK(CaptureActor);

	// Capture it
	CaptureActor->RenderAsset(this, AssetRender);

#endif
}
