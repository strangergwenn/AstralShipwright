// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NovaGameMode.generated.h"


/** Default game mode class */
UCLASS(ClassGroup = (Nova))
class ANovaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ANovaGameMode();


	/*----------------------------------------------------
		Inherited
	----------------------------------------------------*/

	virtual void StartPlay() override;

	virtual void PostLogin(class APlayerController* Player) override;

	virtual void Logout(class AController* Player) override;

	virtual class UClass* GetDefaultPawnClassForController_Implementation(class AController* InController) override;

	virtual AActor* ChoosePlayerStart_Implementation(class AController* Player) override;


};
