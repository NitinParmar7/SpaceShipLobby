#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SpaceShipHUD.generated.h"

UCLASS()
class SPACESHIP_API ASpaceShipHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
