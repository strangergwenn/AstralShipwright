// Nova project - Gwennaël Arbona

#include "NovaCanvas.h"
#include "Nova/Nova.h"

#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

/*----------------------------------------------------
    Texture canvas tools
----------------------------------------------------*/

UCanvasRenderTarget2D* UNovaCanvas::CreateRenderTarget(UObject* Owner, int32 Width, int32 Height)
{
	UCanvasRenderTarget2D* Texture =
		UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(Owner, UCanvasRenderTarget2D::StaticClass(), Width, Height);
	NCHECK(Texture);

	Texture->ClearColor = FLinearColor::Black;
	Texture->UpdateResource();

	return Texture;
}

void UNovaCanvas::DrawTexture(UCanvas* CurrentCanvas, UTexture* Texture, float ScreenX, float ScreenY, float ScreenW, float ScreenH,
	FLinearColor Color, EBlendMode BlendMode, float Scale, bool bScalePosition, float Rotation, FVector2D RotPivot)
{
	// Setup texture
	FCanvasTileItem TileItem(
		FVector2D(ScreenX, ScreenY), Texture->Resource, FVector2D(ScreenW, ScreenH) * Scale, FVector2D(0, 0), FVector2D(1, 1), Color);

	// More setup
	TileItem.Rotation   = FRotator(0, Rotation, 0);
	TileItem.PivotPoint = RotPivot;
	if (bScalePosition)
	{
		TileItem.Position *= Scale;
	}
	TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(BlendMode);

	// Draw texture
	TileItem.SetColor(Color);
	CurrentCanvas->DrawItem(TileItem);
}

void UNovaCanvas::DrawFlipbook(UCanvas* CurrentCanvas, UTexture* Texture, float ScreenX, float ScreenY, float ScreenW, float ScreenH,
	int32 Resolution, float Time, FLinearColor Color, EBlendMode BlendMode)
{
	// Compute flipbook UVs
	FVector2D UVSize       = (1.0f / Resolution) * FVector2D::UnitVector;
	FVector2D UVMultiplier = FVector2D(FMath::Square(Resolution), Resolution);
	FVector2D SourceUV     = UVMultiplier * FMath::Frac(Time);
	SourceUV               = FVector2D(FMath::FloorToInt(SourceUV.X), FMath::FloorToInt(SourceUV.Y)) / Resolution;

	// Setup texture
	FCanvasTileItem TileItem(
		FVector2D(ScreenX, ScreenY), Texture->Resource, FVector2D(ScreenW, ScreenH), SourceUV, SourceUV + UVSize, Color);

	// More setup
	TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(BlendMode);

	// Draw texture
	TileItem.SetColor(Color);
	CurrentCanvas->DrawItem(TileItem);
}

void UNovaCanvas::DrawMaterial(UCanvas* CurrentCanvas, const UMaterialInterface* Material, float ScreenX, float ScreenY, float ScreenW,
	float ScreenH, FLinearColor Color, float Scale, bool bScalePosition, float Rotation, FVector2D RotPivot)
{
	// Setup material
	FCanvasTileItem TileItem(
		FVector2D(ScreenX, ScreenY), Material->GetRenderProxy(), FVector2D(ScreenW, ScreenH) * Scale, FVector2D(0, 0), FVector2D(1, 1));

	// More setup
	TileItem.Rotation   = FRotator(0, Rotation, 0);
	TileItem.PivotPoint = RotPivot;
	if (bScalePosition)
	{
		TileItem.Position *= Scale;
	}

	// Draw material
	TileItem.SetColor(Color);
	CurrentCanvas->DrawItem(TileItem);
}

void UNovaCanvas::DrawText(
	UCanvas* CurrentCanvas, FText Text, class UFont* Font, FVector2D Position, FLinearColor Color, float Scale, bool Center)
{
	float X = Position.X;
	float Y = Position.Y;

	// Optional centering
	if (Center)
	{
		float XL, YL;
		CurrentCanvas->TextSize(Font, Text.ToString(), XL, YL, Scale);
		X = FMath::RoundToInt(CurrentCanvas->ClipX / 2.0f - XL / 2.0f + Position.X);
		Y = FMath::RoundToInt(CurrentCanvas->ClipY / 2.0f - YL / 2.0f + Position.Y);
	}

	// Text
	{
		FCanvasTextItem TextItem(FVector2D(X, Y), Text, Font, Color);
		TextItem.Scale     = Scale * FVector2D(1, 1);
		TextItem.bOutlined = false;
		CurrentCanvas->DrawItem(TextItem);
	}
}

void UNovaCanvas::DrawLine(UCanvas* CurrentCanvas, FVector2D Start, FVector2D End, FLinearColor Color)
{
	FCanvasLineItem LineItem(Start, End);
	LineItem.SetColor(Color);
	LineItem.LineThickness = 1.0f;
	CurrentCanvas->DrawItem(LineItem);
}

void UNovaCanvas::DrawSquare(UCanvas* CurrentCanvas, FVector2D Position, FVector2D Size, FLinearColor Color)
{
	FCanvasTileItem SquareItem(Position, Size, Color);
	SquareItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Opaque);
	CurrentCanvas->DrawItem(SquareItem);
}

void UNovaCanvas::DrawProgressBar(UCanvas* CurrentCanvas, FVector2D Position, float Value, FLinearColor BarColor, FLinearColor BoxColor,
	int32 Width, int32 Height, int32 BoundHeight)
{
	DrawBoxBar(CurrentCanvas, Position, 0, Value, BarColor, BoxColor, Width, Height, BoundHeight);
}

void UNovaCanvas::DrawBoxBar(UCanvas* CurrentCanvas, FVector2D Position, float StartValue, float EndValue, FLinearColor BarColor,
	FLinearColor BoxColor, int32 Width, int32 Height, int32 BoundHeight)
{
	// Draw upper bound
	FCanvasTileItem UpperBarItem(Position, FVector2D(Width, BoundHeight), BoxColor);
	UpperBarItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Opaque);
	CurrentCanvas->DrawItem(UpperBarItem);

	// Draw upper bound
	FCanvasTileItem LowerBarItem(Position + FVector2D(0, Height - BoundHeight), FVector2D(Width, BoundHeight), BoxColor);
	LowerBarItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Opaque);
	CurrentCanvas->DrawItem(LowerBarItem);

	// Get values
	float ClampedDelta            = FMath::Clamp(EndValue - StartValue, 0.0f, 1.0f);
	float ClampedValue            = FMath::Clamp(EndValue, 0.0f, 1.0f);
	int32 ProgressBarHeight       = FMath::CeilToInt(ClampedDelta * (Height - (2 * BoundHeight)));
	int32 ProgressBarHeightOffset = FMath::FloorToInt(BoundHeight + (1 - ClampedValue) * (Height - (2 * BoundHeight)));

	// Draw progress bar
	FCanvasTileItem ProgressBarItem(Position + FVector2D(0, ProgressBarHeightOffset), FVector2D(Width, ProgressBarHeight), BarColor);
	ProgressBarItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Opaque);
	CurrentCanvas->DrawItem(ProgressBarItem);
}
