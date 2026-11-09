// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpaceShipGameMode.generated.h"

UCLASS(minimalapi)
class ASpaceShipGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpaceShipGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby")
	int32 MaxLobbyPlayers = 16;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby")
	FVector LobbySpawnOrigin = FVector(0.0f, 0.0f, 100.0f);

private:
	TWeakObjectPtr<APlayerStart> BasePlayerStart;
	TMap<AController*, int32> PlayerSpawnSlots;
	TMap<int32, APlayerStart*> SpawnStartsBySlot;
	bool bCheckedForBasePlayerStart = false;
};


