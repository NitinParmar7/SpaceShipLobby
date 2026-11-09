// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SpaceShipGameInstance.generated.h"

class SLobbyMenuWidget;
class FOnlineSessionSearch;

UCLASS()
class SPACESHIP_API USpaceShipGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	USpaceShipGameInstance();

	virtual void Init() override;
	virtual void OnStart() override;
	virtual void OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld) override;
	virtual void Shutdown() override;

	UFUNCTION(Exec)
	void HostLobby();

	UFUNCTION(Exec)
	void JoinLobby();

	UFUNCTION(Exec)
	void LeaveLobby();

	void FindAndJoinLobby();
	void QuitGame();
	FText GetLobbyStatus() const;

private:
	IOnlineSessionPtr GetSessionInterface() const;
	void CreateLobbySession();
	void ShowLobbyMenu();
	void HideLobbyMenu();
	void SetLobbyStatus(const FText& Status);
	void ReturnToStandaloneLobby();
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void RecoverFromTravelFailure(const FString& ErrorString);

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	TSharedPtr<SLobbyMenuWidget> LobbyMenuWidget;
	FText LobbyStatus;
	bool bCreateSessionAfterDestroy = false;
	bool bReturnToMenuAfterDestroy = false;
	bool bMenuShouldBeVisible = false;
	bool bLobbyMenuAddedToViewport = false;
	bool bOperationInProgress = false;
};
