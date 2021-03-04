// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "NovaCanvas.generated.h"

/** Canvas helper class */
UCLASS(ClassGroup = (Nova))
class UNovaCanvas : public UObject
{
	GENERATED_BODY()

public:
	/*----------------------------------------------------
	    Texture canvas tools
	----------------------------------------------------*/

	/** Create a texture canvas */
	static UCanvasRenderTarget2D* CreateRenderTarget(UObject* Owner, int32 Width, int32 Height);

	/** Draw a texture on the canvas */
	static void DrawTexture(UCanvas* CurrentCanvas, UTexture* Texture, float ScreenX, float ScreenY, float ScreenW, float ScreenH,
		FLinearColor Color = FLinearColor::White, EBlendMode BlendMode = BLEND_Translucent, float Scale = 1.f, bool bScalePosition = false,
		float Rotation = 0.f, FVector2D RotPivot = FVector2D::ZeroVector);

	/** Draw a flipbook texture on the canvas */
	static void DrawFlipbook(UCanvas* CurrentCanvas, UTexture* Texture, float ScreenX, float ScreenY, float ScreenW, float ScreenH,
		int32 Resolution, float Time, FLinearColor Color = FLinearColor::White, EBlendMode BlendMode = BLEND_Translucent);

	/** Draw a material on the canvas */
	static void DrawMaterial(UCanvas* CurrentCanvas, const UMaterialInterface* Material, float ScreenX, float ScreenY, float ScreenW,
		float ScreenH, FLinearColor Color = FLinearColor::White, float Scale = 1.f, bool bScalePosition = false, float Rotation = 0.f,
		FVector2D RotPivot = FVector2D::ZeroVector);

	/** Draw a string on the canvas */
	static void DrawText(UCanvas* CurrentCanvas, FText Text, class UFont* Font, FVector2D Position,
		FLinearColor Color = FLinearColor::White, float Scale = 1.0f, bool Center = false);

	/** Draw a line on the canvas */
	static void DrawLine(UCanvas* CurrentCanvas, FVector2D Start, FVector2D End, FLinearColor Color = FLinearColor::White);

	/** Draw a square on the canvas */
	static void DrawSquare(UCanvas* CurrentCanvas, FVector2D Position, FVector2D Size, FLinearColor Color = FLinearColor::White);

	/** Draw a progress bar on the canvas */
	static void DrawProgressBar(UCanvas* CurrentCanvas, FVector2D Position, float Value, FLinearColor BarColor = FLinearColor::White,
		FLinearColor BoxColor = FLinearColor::White, int32 Width = 4, int32 Height = 20, int32 BoundHeight = 1);

	/** Draw a box bar graph on the canvas */
	static void DrawBoxBar(UCanvas* CurrentCanvas, FVector2D Position, float StartValue, float EndValue,
		FLinearColor BarColor = FLinearColor::White, FLinearColor BoxColor = FLinearColor::White, int32 Width = 4, int32 Height = 20,
		int32 BoundHeight = 1);
};
