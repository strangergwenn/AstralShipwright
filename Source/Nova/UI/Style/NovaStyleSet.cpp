// Nova project - Gwennaël Arbona

#include "NovaStyleSet.h"
#include "Nova/Nova.h"

#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

#define GAIA_STYLE_INSTANCE_NAME "NovaStyle"
#define GAIA_STYLE_PATH "/Game/UI"

/*----------------------------------------------------
    Singleton
----------------------------------------------------*/

TSharedPtr<FSlateStyleSet> FNovaStyleSet::Instance = NULL;

void FNovaStyleSet::Initialize()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(GAIA_STYLE_INSTANCE_NAME);

	if (!Instance.IsValid())
	{
		Instance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*Instance);
	}
}

void FNovaStyleSet::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*Instance);

	NCHECK(Instance.IsUnique());
	Instance.Reset();
}

TPair<FText, const FSlateBrush*> FNovaStyleSet::GetKeyDisplay(const FKey& Key)
{
	if (Key.IsValid())
	{
		FString DisplayBrushName;
		FText   DisplayName;

		// Special keyboard keys
		if (Key == EKeys::LeftShift)
		{
			DisplayName = FText::FromString("SHFT");
		}
		else if (Key == EKeys::RightShift)
		{
			DisplayName = FText::FromString("RSHF");
		}
		else if (Key == EKeys::LeftControl)
		{
			DisplayName = FText::FromString("CTL");
		}
		else if (Key == EKeys::RightControl)
		{
			DisplayName = FText::FromString("RCTL");
		}
		else if (Key == EKeys::LeftCommand)
		{
			DisplayName = FText::FromString("CMD");
		}
		else if (Key == EKeys::RightCommand)
		{
			DisplayName = FText::FromString("RCMD");
		}
		else if (Key == EKeys::LeftAlt)
		{
			DisplayName = FText::FromString("ALT");
		}
		else if (Key == EKeys::RightAlt)
		{
			DisplayName = FText::FromString("RALT");
		}
		else if (Key == EKeys::Backslash)
		{
			DisplayName = FText::FromString("RET");
		}
		else if (Key == EKeys::NumLock)
		{
			DisplayName = FText::FromString("NUM");
		}
		else if (Key == EKeys::ScrollLock)
		{
			DisplayName = FText::FromString("SCR");
		}
		else if (Key == EKeys::PageDown)
		{
			DisplayName = FText::FromString("PDN");
		}
		else if (Key == EKeys::PageUp)
		{
			DisplayName = FText::FromString("PUP");
		}
		else if (Key == EKeys::Down)
		{
			DisplayBrushName = "Down";
		}
		else if (Key == EKeys::Left)
		{
			DisplayBrushName = "Left";
		}
		else if (Key == EKeys::Right)
		{
			DisplayBrushName = "Right";
		}
		else if (Key == EKeys::Up)
		{
			DisplayBrushName = "Up";
		}

		// Mouse keys
		else if (Key == EKeys::LeftMouseButton)
		{
			// TODO
			DisplayName = FText::FromString("LMB");
		}
		else if (Key == EKeys::RightMouseButton)
		{
			// TODO
			DisplayName = FText::FromString("RMB");
		}
		else if (Key == EKeys::MiddleMouseButton)
		{
			// TODO
			DisplayName = FText::FromString("MMB");
		}
		else if (Key == EKeys::MouseScrollDown)
		{
			DisplayBrushName = "MouseDown";
		}
		else if (Key == EKeys::MouseScrollUp)
		{
			DisplayBrushName = "MouseUp";
		}
		else if (Key == EKeys::ThumbMouseButton)
		{
			DisplayName = FText::FromString("M4");
		}
		else if (Key == EKeys::ThumbMouseButton2)
		{
			DisplayName = FText::FromString("M5");
		}

		// Gamepad
		else if (Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			DisplayBrushName = "A";
		}
		else if (Key == EKeys::Gamepad_FaceButton_Left)
		{
			DisplayBrushName = "X";
		}
		else if (Key == EKeys::Gamepad_FaceButton_Right)
		{
			DisplayBrushName = "B";
		}
		else if (Key == EKeys::Gamepad_FaceButton_Top)
		{
			DisplayBrushName = "Y";
		}
		else if (Key == EKeys::Gamepad_DPad_Down)
		{
			DisplayBrushName = "Down";
		}
		else if (Key == EKeys::Gamepad_DPad_Left)
		{
			DisplayBrushName = "Left";
		}
		else if (Key == EKeys::Gamepad_DPad_Right)
		{
			DisplayBrushName = "Right";
		}
		else if (Key == EKeys::Gamepad_DPad_Up)
		{
			DisplayBrushName = "Up";
		}
		else if (Key == EKeys::Gamepad_LeftShoulder)
		{
			// TODO
			DisplayName = FText::FromString("LB");
		}
		else if (Key == EKeys::Gamepad_RightShoulder)
		{
			// TODO
			DisplayName = FText::FromString("RB");
		}
		else if (Key == EKeys::Gamepad_Special_Left)
		{
			DisplayBrushName = "View";
		}
		else if (Key == EKeys::Gamepad_Special_Right)
		{
			DisplayBrushName = "Menu";
		}
		else if (Key == EKeys::Gamepad_LeftThumbstick)
		{
			// TODO
			DisplayName = FText::FromString("LTHMB");
		}
		else if (Key == EKeys::Gamepad_RightThumbstick)
		{
			// TODO
			DisplayName = FText::FromString("RTHMB");
		}

		// Defaults : default brush if no brush set, uppercase name if no text set
		if (DisplayBrushName.Len() == 0)
		{
			DisplayBrushName = "None";
			if (DisplayName.ToString().Len() == 0)
			{
				DisplayName = FText::FromString(Key.GetDisplayName(false).ToString().ToUpper());
			}
		}

		// Return details
		const FSlateBrush* DisplayBrush = FNovaStyleSet::GetBrush(*(FString(TEXT("Key/SB_")) + DisplayBrushName));
		NCHECK(DisplayBrush);
		return TPair<FText, const FSlateBrush*>(DisplayName, DisplayBrush);
	}

	return TPair<FText, const FSlateBrush*>(FText(), nullptr);
}

TPair<TSharedPtr<FSlateBrush>, UMaterialInstanceDynamic*> FNovaStyleSet::GetDynamicBrush(const FString& BrushName)
{
	TSharedPtr<FSlateBrush> ResultBrush = MakeShared<FSlateBrush>();
	const FSlateBrush*      BaseBrush   = FNovaStyleSet::GetBrush(BrushName);

	// Create material
	UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(BaseBrush->GetResourceObject());
	NCHECK(BaseMaterial);
	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
	NCHECK(DynamicMaterial);

	// Update icon brush
	FVector2D ImageSize = BaseBrush->ImageSize;
	ResultBrush->SetResourceObject(DynamicMaterial);
	ResultBrush->ImageSize = ImageSize;

	return TPair<TSharedPtr<FSlateBrush>, UMaterialInstanceDynamic*>(ResultBrush, DynamicMaterial);
}

FLinearColor FNovaStyleSet::GetPlasmaColor(float Alpha)
{
	constexpr FLinearColor Palette[] = {

		FLinearColor(0.050383f, 0.029803f, 0.527975f), FLinearColor(0.063536f, 0.028426f, 0.533124f),
		FLinearColor(0.075353f, 0.027206f, 0.538007f), FLinearColor(0.086222f, 0.026125f, 0.542658f),
		FLinearColor(0.096379f, 0.025165f, 0.547103f), FLinearColor(0.105980f, 0.024309f, 0.551368f),
		FLinearColor(0.115124f, 0.023556f, 0.555468f), FLinearColor(0.123903f, 0.022878f, 0.559423f),
		FLinearColor(0.132381f, 0.022258f, 0.563250f), FLinearColor(0.140603f, 0.021687f, 0.566959f),
		FLinearColor(0.148607f, 0.021154f, 0.570562f), FLinearColor(0.156421f, 0.020651f, 0.574065f),
		FLinearColor(0.164070f, 0.020171f, 0.577478f), FLinearColor(0.171574f, 0.019706f, 0.580806f),
		FLinearColor(0.178950f, 0.019252f, 0.584054f), FLinearColor(0.186213f, 0.018803f, 0.587228f),
		FLinearColor(0.193374f, 0.018354f, 0.590330f), FLinearColor(0.200445f, 0.017902f, 0.593364f),
		FLinearColor(0.207435f, 0.017442f, 0.596333f), FLinearColor(0.214350f, 0.016973f, 0.599239f),
		FLinearColor(0.221197f, 0.016497f, 0.602083f), FLinearColor(0.227983f, 0.016007f, 0.604867f),
		FLinearColor(0.234715f, 0.015502f, 0.607592f), FLinearColor(0.241396f, 0.014979f, 0.610259f),
		FLinearColor(0.248032f, 0.014439f, 0.612868f), FLinearColor(0.254627f, 0.013882f, 0.615419f),
		FLinearColor(0.261183f, 0.013308f, 0.617911f), FLinearColor(0.267703f, 0.012716f, 0.620346f),
		FLinearColor(0.274191f, 0.012109f, 0.622722f), FLinearColor(0.280648f, 0.011488f, 0.625038f),
		FLinearColor(0.287076f, 0.010855f, 0.627295f), FLinearColor(0.293478f, 0.010213f, 0.629490f),
		FLinearColor(0.299855f, 0.009561f, 0.631624f), FLinearColor(0.306210f, 0.008902f, 0.633694f),
		FLinearColor(0.312543f, 0.008239f, 0.635700f), FLinearColor(0.318856f, 0.007576f, 0.637640f),
		FLinearColor(0.325150f, 0.006915f, 0.639512f), FLinearColor(0.331426f, 0.006261f, 0.641316f),
		FLinearColor(0.337683f, 0.005618f, 0.643049f), FLinearColor(0.343925f, 0.004991f, 0.644710f),
		FLinearColor(0.350150f, 0.004382f, 0.646298f), FLinearColor(0.356359f, 0.003798f, 0.647810f),
		FLinearColor(0.362553f, 0.003243f, 0.649245f), FLinearColor(0.368733f, 0.002724f, 0.650601f),
		FLinearColor(0.374897f, 0.002245f, 0.651876f), FLinearColor(0.381047f, 0.001814f, 0.653068f),
		FLinearColor(0.387183f, 0.001434f, 0.654177f), FLinearColor(0.393304f, 0.001114f, 0.655199f),
		FLinearColor(0.399411f, 0.000859f, 0.656133f), FLinearColor(0.405503f, 0.000678f, 0.656977f),
		FLinearColor(0.411580f, 0.000577f, 0.657730f), FLinearColor(0.417642f, 0.000564f, 0.658390f),
		FLinearColor(0.423689f, 0.000646f, 0.658956f), FLinearColor(0.429719f, 0.000831f, 0.659425f),
		FLinearColor(0.435734f, 0.001127f, 0.659797f), FLinearColor(0.441732f, 0.001540f, 0.660069f),
		FLinearColor(0.447714f, 0.002080f, 0.660240f), FLinearColor(0.453677f, 0.002755f, 0.660310f),
		FLinearColor(0.459623f, 0.003574f, 0.660277f), FLinearColor(0.465550f, 0.004545f, 0.660139f),
		FLinearColor(0.471457f, 0.005678f, 0.659897f), FLinearColor(0.477344f, 0.006980f, 0.659549f),
		FLinearColor(0.483210f, 0.008460f, 0.659095f), FLinearColor(0.489055f, 0.010127f, 0.658534f),
		FLinearColor(0.494877f, 0.011990f, 0.657865f), FLinearColor(0.500678f, 0.014055f, 0.657088f),
		FLinearColor(0.506454f, 0.016333f, 0.656202f), FLinearColor(0.512206f, 0.018833f, 0.655209f),
		FLinearColor(0.517933f, 0.021563f, 0.654109f), FLinearColor(0.523633f, 0.024532f, 0.652901f),
		FLinearColor(0.529306f, 0.027747f, 0.651586f), FLinearColor(0.534952f, 0.031217f, 0.650165f),
		FLinearColor(0.540570f, 0.034950f, 0.648640f), FLinearColor(0.546157f, 0.038954f, 0.647010f),
		FLinearColor(0.551715f, 0.043136f, 0.645277f), FLinearColor(0.557243f, 0.047331f, 0.643443f),
		FLinearColor(0.562738f, 0.051545f, 0.641509f), FLinearColor(0.568201f, 0.055778f, 0.639477f),
		FLinearColor(0.573632f, 0.060028f, 0.637349f), FLinearColor(0.579029f, 0.064296f, 0.635126f),
		FLinearColor(0.584391f, 0.068579f, 0.632812f), FLinearColor(0.589719f, 0.072878f, 0.630408f),
		FLinearColor(0.595011f, 0.077190f, 0.627917f), FLinearColor(0.600266f, 0.081516f, 0.625342f),
		FLinearColor(0.605485f, 0.085854f, 0.622686f), FLinearColor(0.610667f, 0.090204f, 0.619951f),
		FLinearColor(0.615812f, 0.094564f, 0.617140f), FLinearColor(0.620919f, 0.098934f, 0.614257f),
		FLinearColor(0.625987f, 0.103312f, 0.611305f), FLinearColor(0.631017f, 0.107699f, 0.608287f),
		FLinearColor(0.636008f, 0.112092f, 0.605205f), FLinearColor(0.640959f, 0.116492f, 0.602065f),
		FLinearColor(0.645872f, 0.120898f, 0.598867f), FLinearColor(0.650746f, 0.125309f, 0.595617f),
		FLinearColor(0.655580f, 0.129725f, 0.592317f), FLinearColor(0.660374f, 0.134144f, 0.588971f),
		FLinearColor(0.665129f, 0.138566f, 0.585582f), FLinearColor(0.669845f, 0.142992f, 0.582154f),
		FLinearColor(0.674522f, 0.147419f, 0.578688f), FLinearColor(0.679160f, 0.151848f, 0.575189f),
		FLinearColor(0.683758f, 0.156278f, 0.571660f), FLinearColor(0.688318f, 0.160709f, 0.568103f),
		FLinearColor(0.692840f, 0.165141f, 0.564522f), FLinearColor(0.697324f, 0.169573f, 0.560919f),
		FLinearColor(0.701769f, 0.174005f, 0.557296f), FLinearColor(0.706178f, 0.178437f, 0.553657f),
		FLinearColor(0.710549f, 0.182868f, 0.550004f), FLinearColor(0.714883f, 0.187299f, 0.546338f),
		FLinearColor(0.719181f, 0.191729f, 0.542663f), FLinearColor(0.723444f, 0.196158f, 0.538981f),
		FLinearColor(0.727670f, 0.200586f, 0.535293f), FLinearColor(0.731862f, 0.205013f, 0.531601f),
		FLinearColor(0.736019f, 0.209439f, 0.527908f), FLinearColor(0.740143f, 0.213864f, 0.524216f),
		FLinearColor(0.744232f, 0.218288f, 0.520524f), FLinearColor(0.748289f, 0.222711f, 0.516834f),
		FLinearColor(0.752312f, 0.227133f, 0.513149f), FLinearColor(0.756304f, 0.231555f, 0.509468f),
		FLinearColor(0.760264f, 0.235976f, 0.505794f), FLinearColor(0.764193f, 0.240396f, 0.502126f),
		FLinearColor(0.768090f, 0.244817f, 0.498465f), FLinearColor(0.771958f, 0.249237f, 0.494813f),
		FLinearColor(0.775796f, 0.253658f, 0.491171f), FLinearColor(0.779604f, 0.258078f, 0.487539f),
		FLinearColor(0.783383f, 0.262500f, 0.483918f), FLinearColor(0.787133f, 0.266922f, 0.480307f),
		FLinearColor(0.790855f, 0.271345f, 0.476706f), FLinearColor(0.794549f, 0.275770f, 0.473117f),
		FLinearColor(0.798216f, 0.280197f, 0.469538f), FLinearColor(0.801855f, 0.284626f, 0.465971f),
		FLinearColor(0.805467f, 0.289057f, 0.462415f), FLinearColor(0.809052f, 0.293491f, 0.458870f),
		FLinearColor(0.812612f, 0.297928f, 0.455338f), FLinearColor(0.816144f, 0.302368f, 0.451816f),
		FLinearColor(0.819651f, 0.306812f, 0.448306f), FLinearColor(0.823132f, 0.311261f, 0.444806f),
		FLinearColor(0.826588f, 0.315714f, 0.441316f), FLinearColor(0.830018f, 0.320172f, 0.437836f),
		FLinearColor(0.833422f, 0.324635f, 0.434366f), FLinearColor(0.836801f, 0.329105f, 0.430905f),
		FLinearColor(0.840155f, 0.333580f, 0.427455f), FLinearColor(0.843484f, 0.338062f, 0.424013f),
		FLinearColor(0.846788f, 0.342551f, 0.420579f), FLinearColor(0.850066f, 0.347048f, 0.417153f),
		FLinearColor(0.853319f, 0.351553f, 0.413734f), FLinearColor(0.856547f, 0.356066f, 0.410322f),
		FLinearColor(0.859750f, 0.360588f, 0.406917f), FLinearColor(0.862927f, 0.365119f, 0.403519f),
		FLinearColor(0.866078f, 0.369660f, 0.400126f), FLinearColor(0.869203f, 0.374212f, 0.396738f),
		FLinearColor(0.872303f, 0.378774f, 0.393355f), FLinearColor(0.875376f, 0.383347f, 0.389976f),
		FLinearColor(0.878423f, 0.387932f, 0.386600f), FLinearColor(0.881443f, 0.392529f, 0.383229f),
		FLinearColor(0.884436f, 0.397139f, 0.379860f), FLinearColor(0.887402f, 0.401762f, 0.376494f),
		FLinearColor(0.890340f, 0.406398f, 0.373130f), FLinearColor(0.893250f, 0.411048f, 0.369768f),
		FLinearColor(0.896131f, 0.415712f, 0.366407f), FLinearColor(0.898984f, 0.420392f, 0.363047f),
		FLinearColor(0.901807f, 0.425087f, 0.359688f), FLinearColor(0.904601f, 0.429797f, 0.356329f),
		FLinearColor(0.907365f, 0.434524f, 0.352970f), FLinearColor(0.910098f, 0.439268f, 0.349610f),
		FLinearColor(0.912800f, 0.444029f, 0.346251f), FLinearColor(0.915471f, 0.448807f, 0.342890f),
		FLinearColor(0.918109f, 0.453603f, 0.339529f), FLinearColor(0.920714f, 0.458417f, 0.336166f),
		FLinearColor(0.923287f, 0.463251f, 0.332801f), FLinearColor(0.925825f, 0.468103f, 0.329435f),
		FLinearColor(0.928329f, 0.472975f, 0.326067f), FLinearColor(0.930798f, 0.477867f, 0.322697f),
		FLinearColor(0.933232f, 0.482780f, 0.319325f), FLinearColor(0.935630f, 0.487712f, 0.315952f),
		FLinearColor(0.937990f, 0.492667f, 0.312575f), FLinearColor(0.940313f, 0.497642f, 0.309197f),
		FLinearColor(0.942598f, 0.502639f, 0.305816f), FLinearColor(0.944844f, 0.507658f, 0.302433f),
		FLinearColor(0.947051f, 0.512699f, 0.299049f), FLinearColor(0.949217f, 0.517763f, 0.295662f),
		FLinearColor(0.951344f, 0.522850f, 0.292275f), FLinearColor(0.953428f, 0.527960f, 0.288883f),
		FLinearColor(0.955470f, 0.533093f, 0.285490f), FLinearColor(0.957469f, 0.538250f, 0.282096f),
		FLinearColor(0.959424f, 0.543431f, 0.278701f), FLinearColor(0.961336f, 0.548636f, 0.275305f),
		FLinearColor(0.963203f, 0.553865f, 0.271909f), FLinearColor(0.965024f, 0.559118f, 0.268513f),
		FLinearColor(0.966798f, 0.564396f, 0.265118f), FLinearColor(0.968526f, 0.569700f, 0.261721f),
		FLinearColor(0.970205f, 0.575028f, 0.258325f), FLinearColor(0.971835f, 0.580382f, 0.254931f),
		FLinearColor(0.973416f, 0.585761f, 0.251540f), FLinearColor(0.974947f, 0.591165f, 0.248151f),
		FLinearColor(0.976428f, 0.596595f, 0.244767f), FLinearColor(0.977856f, 0.602051f, 0.241387f),
		FLinearColor(0.979233f, 0.607532f, 0.238013f), FLinearColor(0.980556f, 0.613039f, 0.234646f),
		FLinearColor(0.981826f, 0.618572f, 0.231287f), FLinearColor(0.983041f, 0.624131f, 0.227937f),
		FLinearColor(0.984199f, 0.629718f, 0.224595f), FLinearColor(0.985301f, 0.635330f, 0.221265f),
		FLinearColor(0.986345f, 0.640969f, 0.217948f), FLinearColor(0.987332f, 0.646633f, 0.214648f),
		FLinearColor(0.988260f, 0.652325f, 0.211364f), FLinearColor(0.989128f, 0.658043f, 0.208100f),
		FLinearColor(0.989935f, 0.663787f, 0.204859f), FLinearColor(0.990681f, 0.669558f, 0.201642f),
		FLinearColor(0.991365f, 0.675355f, 0.198453f), FLinearColor(0.991985f, 0.681179f, 0.195295f),
		FLinearColor(0.992541f, 0.687030f, 0.192170f), FLinearColor(0.993032f, 0.692907f, 0.189084f),
		FLinearColor(0.993456f, 0.698810f, 0.186041f), FLinearColor(0.993814f, 0.704741f, 0.183043f),
		FLinearColor(0.994103f, 0.710698f, 0.180097f), FLinearColor(0.994324f, 0.716681f, 0.177208f),
		FLinearColor(0.994474f, 0.722691f, 0.174381f), FLinearColor(0.994553f, 0.728728f, 0.171622f),
		FLinearColor(0.994561f, 0.734791f, 0.168938f), FLinearColor(0.994495f, 0.740880f, 0.166335f),
		FLinearColor(0.994355f, 0.746995f, 0.163821f), FLinearColor(0.994141f, 0.753137f, 0.161404f),
		FLinearColor(0.993851f, 0.759304f, 0.159092f), FLinearColor(0.993482f, 0.765499f, 0.156891f),
		FLinearColor(0.993033f, 0.771720f, 0.154808f), FLinearColor(0.992505f, 0.777967f, 0.152855f),
		FLinearColor(0.991897f, 0.784239f, 0.151042f), FLinearColor(0.991209f, 0.790537f, 0.149377f),
		FLinearColor(0.990439f, 0.796859f, 0.147870f), FLinearColor(0.989587f, 0.803205f, 0.146529f),
		FLinearColor(0.988648f, 0.809579f, 0.145357f), FLinearColor(0.987621f, 0.815978f, 0.144363f),
		FLinearColor(0.986509f, 0.822401f, 0.143557f), FLinearColor(0.985314f, 0.828846f, 0.142945f),
		FLinearColor(0.984031f, 0.835315f, 0.142528f), FLinearColor(0.982653f, 0.841812f, 0.142303f),
		FLinearColor(0.981190f, 0.848329f, 0.142279f), FLinearColor(0.979644f, 0.854866f, 0.142453f),
		FLinearColor(0.977995f, 0.861432f, 0.142808f), FLinearColor(0.976265f, 0.868016f, 0.143351f),
		FLinearColor(0.974443f, 0.874622f, 0.144061f), FLinearColor(0.972530f, 0.881250f, 0.144923f),
		FLinearColor(0.970533f, 0.887896f, 0.145919f), FLinearColor(0.968443f, 0.894564f, 0.147014f),
		FLinearColor(0.966271f, 0.901249f, 0.148180f), FLinearColor(0.964021f, 0.907950f, 0.149370f),
		FLinearColor(0.961681f, 0.914672f, 0.150520f), FLinearColor(0.959276f, 0.921407f, 0.151566f),
		FLinearColor(0.956808f, 0.928152f, 0.152409f), FLinearColor(0.954287f, 0.934908f, 0.152921f),
		FLinearColor(0.951726f, 0.941671f, 0.152925f), FLinearColor(0.949151f, 0.948435f, 0.152178f),
		FLinearColor(0.946602f, 0.955190f, 0.150328f), FLinearColor(0.944152f, 0.961916f, 0.146861f),
		FLinearColor(0.941896f, 0.968590f, 0.140956f), FLinearColor(0.940015f, 0.975158f, 0.131326f)};

	int32 PaletteIndex = FMath::Clamp(FMath::RoundToInt(Alpha * 255.0f), 0, 255);

	return Palette[PaletteIndex];
}

FLinearColor FNovaStyleSet::GetViridisColor(float Alpha)
{
	constexpr FLinearColor Palette[] = {FLinearColor(0.267004f, 0.004874f, 0.329415f), FLinearColor(0.268510f, 0.009605f, 0.335427f),
		FLinearColor(0.269944f, 0.014625f, 0.341379f), FLinearColor(0.271305f, 0.019942f, 0.347269f),
		FLinearColor(0.272594f, 0.025563f, 0.353093f), FLinearColor(0.273809f, 0.031497f, 0.358853f),
		FLinearColor(0.274952f, 0.037752f, 0.364543f), FLinearColor(0.276022f, 0.044167f, 0.370164f),
		FLinearColor(0.277018f, 0.050344f, 0.375715f), FLinearColor(0.277941f, 0.056324f, 0.381191f),
		FLinearColor(0.278791f, 0.062145f, 0.386592f), FLinearColor(0.279566f, 0.067836f, 0.391917f),
		FLinearColor(0.280267f, 0.073417f, 0.397163f), FLinearColor(0.280894f, 0.078907f, 0.402329f),
		FLinearColor(0.281446f, 0.084320f, 0.407414f), FLinearColor(0.281924f, 0.089666f, 0.412415f),
		FLinearColor(0.282327f, 0.094955f, 0.417331f), FLinearColor(0.282656f, 0.100196f, 0.422160f),
		FLinearColor(0.282910f, 0.105393f, 0.426902f), FLinearColor(0.283091f, 0.110553f, 0.431554f),
		FLinearColor(0.283197f, 0.115680f, 0.436115f), FLinearColor(0.283229f, 0.120777f, 0.440584f),
		FLinearColor(0.283187f, 0.125848f, 0.444960f), FLinearColor(0.283072f, 0.130895f, 0.449241f),
		FLinearColor(0.282884f, 0.135920f, 0.453427f), FLinearColor(0.282623f, 0.140926f, 0.457517f),
		FLinearColor(0.282290f, 0.145912f, 0.461510f), FLinearColor(0.281887f, 0.150881f, 0.465405f),
		FLinearColor(0.281412f, 0.155834f, 0.469201f), FLinearColor(0.280868f, 0.160771f, 0.472899f),
		FLinearColor(0.280255f, 0.165693f, 0.476498f), FLinearColor(0.279574f, 0.170599f, 0.479997f),
		FLinearColor(0.278826f, 0.175490f, 0.483397f), FLinearColor(0.278012f, 0.180367f, 0.486697f),
		FLinearColor(0.277134f, 0.185228f, 0.489898f), FLinearColor(0.276194f, 0.190074f, 0.493001f),
		FLinearColor(0.275191f, 0.194905f, 0.496005f), FLinearColor(0.274128f, 0.199721f, 0.498911f),
		FLinearColor(0.273006f, 0.204520f, 0.501721f), FLinearColor(0.271828f, 0.209303f, 0.504434f),
		FLinearColor(0.270595f, 0.214069f, 0.507052f), FLinearColor(0.269308f, 0.218818f, 0.509577f),
		FLinearColor(0.267968f, 0.223549f, 0.512008f), FLinearColor(0.266580f, 0.228262f, 0.514349f),
		FLinearColor(0.265145f, 0.232956f, 0.516599f), FLinearColor(0.263663f, 0.237631f, 0.518762f),
		FLinearColor(0.262138f, 0.242286f, 0.520837f), FLinearColor(0.260571f, 0.246922f, 0.522828f),
		FLinearColor(0.258965f, 0.251537f, 0.524736f), FLinearColor(0.257322f, 0.256130f, 0.526563f),
		FLinearColor(0.255645f, 0.260703f, 0.528312f), FLinearColor(0.253935f, 0.265254f, 0.529983f),
		FLinearColor(0.252194f, 0.269783f, 0.531579f), FLinearColor(0.250425f, 0.274290f, 0.533103f),
		FLinearColor(0.248629f, 0.278775f, 0.534556f), FLinearColor(0.246811f, 0.283237f, 0.535941f),
		FLinearColor(0.244972f, 0.287675f, 0.537260f), FLinearColor(0.243113f, 0.292092f, 0.538516f),
		FLinearColor(0.241237f, 0.296485f, 0.539709f), FLinearColor(0.239346f, 0.300855f, 0.540844f),
		FLinearColor(0.237441f, 0.305202f, 0.541921f), FLinearColor(0.235526f, 0.309527f, 0.542944f),
		FLinearColor(0.233603f, 0.313828f, 0.543914f), FLinearColor(0.231674f, 0.318106f, 0.544834f),
		FLinearColor(0.229739f, 0.322361f, 0.545706f), FLinearColor(0.227802f, 0.326594f, 0.546532f),
		FLinearColor(0.225863f, 0.330805f, 0.547314f), FLinearColor(0.223925f, 0.334994f, 0.548053f),
		FLinearColor(0.221989f, 0.339161f, 0.548752f), FLinearColor(0.220057f, 0.343307f, 0.549413f),
		FLinearColor(0.218130f, 0.347432f, 0.550038f), FLinearColor(0.216210f, 0.351535f, 0.550627f),
		FLinearColor(0.214298f, 0.355619f, 0.551184f), FLinearColor(0.212395f, 0.359683f, 0.551710f),
		FLinearColor(0.210503f, 0.363727f, 0.552206f), FLinearColor(0.208623f, 0.367752f, 0.552675f),
		FLinearColor(0.206756f, 0.371758f, 0.553117f), FLinearColor(0.204903f, 0.375746f, 0.553533f),
		FLinearColor(0.203063f, 0.379716f, 0.553925f), FLinearColor(0.201239f, 0.383670f, 0.554294f),
		FLinearColor(0.199430f, 0.387607f, 0.554642f), FLinearColor(0.197636f, 0.391528f, 0.554969f),
		FLinearColor(0.195860f, 0.395433f, 0.555276f), FLinearColor(0.194100f, 0.399323f, 0.555565f),
		FLinearColor(0.192357f, 0.403199f, 0.555836f), FLinearColor(0.190631f, 0.407061f, 0.556089f),
		FLinearColor(0.188923f, 0.410910f, 0.556326f), FLinearColor(0.187231f, 0.414746f, 0.556547f),
		FLinearColor(0.185556f, 0.418570f, 0.556753f), FLinearColor(0.183898f, 0.422383f, 0.556944f),
		FLinearColor(0.182256f, 0.426184f, 0.557120f), FLinearColor(0.180629f, 0.429975f, 0.557282f),
		FLinearColor(0.179019f, 0.433756f, 0.557430f), FLinearColor(0.177423f, 0.437527f, 0.557565f),
		FLinearColor(0.175841f, 0.441290f, 0.557685f), FLinearColor(0.174274f, 0.445044f, 0.557792f),
		FLinearColor(0.172719f, 0.448791f, 0.557885f), FLinearColor(0.171176f, 0.452530f, 0.557965f),
		FLinearColor(0.169646f, 0.456262f, 0.558030f), FLinearColor(0.168126f, 0.459988f, 0.558082f),
		FLinearColor(0.166617f, 0.463708f, 0.558119f), FLinearColor(0.165117f, 0.467423f, 0.558141f),
		FLinearColor(0.163625f, 0.471133f, 0.558148f), FLinearColor(0.162142f, 0.474838f, 0.558140f),
		FLinearColor(0.160665f, 0.478540f, 0.558115f), FLinearColor(0.159194f, 0.482237f, 0.558073f),
		FLinearColor(0.157729f, 0.485932f, 0.558013f), FLinearColor(0.156270f, 0.489624f, 0.557936f),
		FLinearColor(0.154815f, 0.493313f, 0.557840f), FLinearColor(0.153364f, 0.497000f, 0.557724f),
		FLinearColor(0.151918f, 0.500685f, 0.557587f), FLinearColor(0.150476f, 0.504369f, 0.557430f),
		FLinearColor(0.149039f, 0.508051f, 0.557250f), FLinearColor(0.147607f, 0.511733f, 0.557049f),
		FLinearColor(0.146180f, 0.515413f, 0.556823f), FLinearColor(0.144759f, 0.519093f, 0.556572f),
		FLinearColor(0.143343f, 0.522773f, 0.556295f), FLinearColor(0.141935f, 0.526453f, 0.555991f),
		FLinearColor(0.140536f, 0.530132f, 0.555659f), FLinearColor(0.139147f, 0.533812f, 0.555298f),
		FLinearColor(0.137770f, 0.537492f, 0.554906f), FLinearColor(0.136408f, 0.541173f, 0.554483f),
		FLinearColor(0.135066f, 0.544853f, 0.554029f), FLinearColor(0.133743f, 0.548535f, 0.553541f),
		FLinearColor(0.132444f, 0.552216f, 0.553018f), FLinearColor(0.131172f, 0.555899f, 0.552459f),
		FLinearColor(0.129933f, 0.559582f, 0.551864f), FLinearColor(0.128729f, 0.563265f, 0.551229f),
		FLinearColor(0.127568f, 0.566949f, 0.550556f), FLinearColor(0.126453f, 0.570633f, 0.549841f),
		FLinearColor(0.125394f, 0.574318f, 0.549086f), FLinearColor(0.124395f, 0.578002f, 0.548287f),
		FLinearColor(0.123463f, 0.581687f, 0.547445f), FLinearColor(0.122606f, 0.585371f, 0.546557f),
		FLinearColor(0.121831f, 0.589055f, 0.545623f), FLinearColor(0.121148f, 0.592739f, 0.544641f),
		FLinearColor(0.120565f, 0.596422f, 0.543611f), FLinearColor(0.120092f, 0.600104f, 0.542530f),
		FLinearColor(0.119738f, 0.603785f, 0.541400f), FLinearColor(0.119512f, 0.607464f, 0.540218f),
		FLinearColor(0.119423f, 0.611141f, 0.538982f), FLinearColor(0.119483f, 0.614817f, 0.537692f),
		FLinearColor(0.119699f, 0.618490f, 0.536347f), FLinearColor(0.120081f, 0.622161f, 0.534946f),
		FLinearColor(0.120638f, 0.625828f, 0.533488f), FLinearColor(0.121380f, 0.629492f, 0.531973f),
		FLinearColor(0.122312f, 0.633153f, 0.530398f), FLinearColor(0.123444f, 0.636809f, 0.528763f),
		FLinearColor(0.124780f, 0.640461f, 0.527068f), FLinearColor(0.126326f, 0.644107f, 0.525311f),
		FLinearColor(0.128087f, 0.647749f, 0.523491f), FLinearColor(0.130067f, 0.651384f, 0.521608f),
		FLinearColor(0.132268f, 0.655014f, 0.519661f), FLinearColor(0.134692f, 0.658636f, 0.517649f),
		FLinearColor(0.137339f, 0.662252f, 0.515571f), FLinearColor(0.140210f, 0.665859f, 0.513427f),
		FLinearColor(0.143303f, 0.669459f, 0.511215f), FLinearColor(0.146616f, 0.673050f, 0.508936f),
		FLinearColor(0.150148f, 0.676631f, 0.506589f), FLinearColor(0.153894f, 0.680203f, 0.504172f),
		FLinearColor(0.157851f, 0.683765f, 0.501686f), FLinearColor(0.162016f, 0.687316f, 0.499129f),
		FLinearColor(0.166383f, 0.690856f, 0.496502f), FLinearColor(0.170948f, 0.694384f, 0.493803f),
		FLinearColor(0.175707f, 0.697900f, 0.491033f), FLinearColor(0.180653f, 0.701402f, 0.488189f),
		FLinearColor(0.185783f, 0.704891f, 0.485273f), FLinearColor(0.191090f, 0.708366f, 0.482284f),
		FLinearColor(0.196571f, 0.711827f, 0.479221f), FLinearColor(0.202219f, 0.715272f, 0.476084f),
		FLinearColor(0.208030f, 0.718701f, 0.472873f), FLinearColor(0.214000f, 0.722114f, 0.469588f),
		FLinearColor(0.220124f, 0.725509f, 0.466226f), FLinearColor(0.226397f, 0.728888f, 0.462789f),
		FLinearColor(0.232815f, 0.732247f, 0.459277f), FLinearColor(0.239374f, 0.735588f, 0.455688f),
		FLinearColor(0.246070f, 0.738910f, 0.452024f), FLinearColor(0.252899f, 0.742211f, 0.448284f),
		FLinearColor(0.259857f, 0.745492f, 0.444467f), FLinearColor(0.266941f, 0.748751f, 0.440573f),
		FLinearColor(0.274149f, 0.751988f, 0.436601f), FLinearColor(0.281477f, 0.755203f, 0.432552f),
		FLinearColor(0.288921f, 0.758394f, 0.428426f), FLinearColor(0.296479f, 0.761561f, 0.424223f),
		FLinearColor(0.304148f, 0.764704f, 0.419943f), FLinearColor(0.311925f, 0.767822f, 0.415586f),
		FLinearColor(0.319809f, 0.770914f, 0.411152f), FLinearColor(0.327796f, 0.773980f, 0.406640f),
		FLinearColor(0.335885f, 0.777018f, 0.402049f), FLinearColor(0.344074f, 0.780029f, 0.397381f),
		FLinearColor(0.352360f, 0.783011f, 0.392636f), FLinearColor(0.360741f, 0.785964f, 0.387814f),
		FLinearColor(0.369214f, 0.788888f, 0.382914f), FLinearColor(0.377779f, 0.791781f, 0.377939f),
		FLinearColor(0.386433f, 0.794644f, 0.372886f), FLinearColor(0.395174f, 0.797475f, 0.367757f),
		FLinearColor(0.404001f, 0.800275f, 0.362552f), FLinearColor(0.412913f, 0.803041f, 0.357269f),
		FLinearColor(0.421908f, 0.805774f, 0.351910f), FLinearColor(0.430983f, 0.808473f, 0.346476f),
		FLinearColor(0.440137f, 0.811138f, 0.340967f), FLinearColor(0.449368f, 0.813768f, 0.335384f),
		FLinearColor(0.458674f, 0.816363f, 0.329727f), FLinearColor(0.468053f, 0.818921f, 0.323998f),
		FLinearColor(0.477504f, 0.821444f, 0.318195f), FLinearColor(0.487026f, 0.823929f, 0.312321f),
		FLinearColor(0.496615f, 0.826376f, 0.306377f), FLinearColor(0.506271f, 0.828786f, 0.300362f),
		FLinearColor(0.515992f, 0.831158f, 0.294279f), FLinearColor(0.525776f, 0.833491f, 0.288127f),
		FLinearColor(0.535621f, 0.835785f, 0.281908f), FLinearColor(0.545524f, 0.838039f, 0.275626f),
		FLinearColor(0.555484f, 0.840254f, 0.269281f), FLinearColor(0.565498f, 0.842430f, 0.262877f),
		FLinearColor(0.575563f, 0.844566f, 0.256415f), FLinearColor(0.585678f, 0.846661f, 0.249897f),
		FLinearColor(0.595839f, 0.848717f, 0.243329f), FLinearColor(0.606045f, 0.850733f, 0.236712f),
		FLinearColor(0.616293f, 0.852709f, 0.230052f), FLinearColor(0.626579f, 0.854645f, 0.223353f),
		FLinearColor(0.636902f, 0.856542f, 0.216620f), FLinearColor(0.647257f, 0.858400f, 0.209861f),
		FLinearColor(0.657642f, 0.860219f, 0.203082f), FLinearColor(0.668054f, 0.861999f, 0.196293f),
		FLinearColor(0.678489f, 0.863742f, 0.189503f), FLinearColor(0.688944f, 0.865448f, 0.182725f),
		FLinearColor(0.699415f, 0.867117f, 0.175971f), FLinearColor(0.709898f, 0.868751f, 0.169257f),
		FLinearColor(0.720391f, 0.870350f, 0.162603f), FLinearColor(0.730889f, 0.871916f, 0.156029f),
		FLinearColor(0.741388f, 0.873449f, 0.149561f), FLinearColor(0.751884f, 0.874951f, 0.143228f),
		FLinearColor(0.762373f, 0.876424f, 0.137064f), FLinearColor(0.772852f, 0.877868f, 0.131109f),
		FLinearColor(0.783315f, 0.879285f, 0.125405f), FLinearColor(0.793760f, 0.880678f, 0.120005f),
		FLinearColor(0.804182f, 0.882046f, 0.114965f), FLinearColor(0.814576f, 0.883393f, 0.110347f),
		FLinearColor(0.824940f, 0.884720f, 0.106217f), FLinearColor(0.835270f, 0.886029f, 0.102646f),
		FLinearColor(0.845561f, 0.887322f, 0.099702f), FLinearColor(0.855810f, 0.888601f, 0.097452f),
		FLinearColor(0.866013f, 0.889868f, 0.095953f), FLinearColor(0.876168f, 0.891125f, 0.095250f),
		FLinearColor(0.886271f, 0.892374f, 0.095374f), FLinearColor(0.896320f, 0.893616f, 0.096335f),
		FLinearColor(0.906311f, 0.894855f, 0.098125f), FLinearColor(0.916242f, 0.896091f, 0.100717f),
		FLinearColor(0.926106f, 0.897330f, 0.104071f), FLinearColor(0.935904f, 0.898570f, 0.108131f),
		FLinearColor(0.945636f, 0.899815f, 0.112838f), FLinearColor(0.955300f, 0.901065f, 0.118128f),
		FLinearColor(0.964894f, 0.902323f, 0.123941f), FLinearColor(0.974417f, 0.903590f, 0.130215f),
		FLinearColor(0.983868f, 0.904867f, 0.136897f), FLinearColor(0.993248f, 0.906157f, 0.143936f)};

	int32 PaletteIndex = FMath::Clamp(FMath::RoundToInt(Alpha * 255.0f), 0, 255);

	return Palette[PaletteIndex];
}

/*----------------------------------------------------
    Internal
----------------------------------------------------*/

TSharedRef<FSlateStyleSet> FNovaStyleSet::Create()
{
	TSharedRef<FSlateStyleSet> StyleRef = FSlateGameResources::New(FName(GAIA_STYLE_INSTANCE_NAME), GAIA_STYLE_PATH, GAIA_STYLE_PATH);
	FSlateStyleSet&            Style    = StyleRef.Get();

	Style.Set("Nova.Button", FButtonStyle()
								 .SetNormal(FSlateNoResource())
								 .SetHovered(FSlateNoResource())
								 .SetPressed(FSlateNoResource())
								 .SetDisabled(FSlateNoResource())
								 .SetNormalPadding(0)
								 .SetPressedPadding(0));

	return StyleRef;
}
