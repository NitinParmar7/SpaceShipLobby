// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceShipGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "SLobbyMenuWidget.h"

namespace
{
	const FName LobbySessionName = NAME_GameSession;
	const FName LobbyMapName(TEXT("/Game/SpaceshipInterior/Maps/Demonstration"));
}

USpaceShipGameInstance::USpaceShipGameInstance()
	: CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &USpaceShipGameInstance::HandleCreateSessionComplete))
	, FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &USpaceShipGameInstance::HandleFindSessionsComplete))
	, JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &USpaceShipGameInstance::HandleJoinSessionComplete))
	, DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &USpaceShipGameInstance::HandleDestroySessionComplete))
	, LobbyStatus(FText::FromString(TEXT("Host a lobby or find one on your LAN.")))
{
}

void USpaceShipGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &USpaceShipGameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &USpaceShipGameInstance::HandleTravelFailure);
	}
}

void USpaceShipGameInstance::OnStart()
{
	Super::OnStart();
	ShowLobbyMenu();
}

void USpaceShipGameInstance::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	Super::OnWorldChanged(OldWorld, NewWorld);

	if (bMenuShouldBeVisible && NewWorld && NewWorld->GetNetMode() == NM_Standalone)
	{
		ShowLobbyMenu();
	}
}

void USpaceShipGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	HideLobbyMenu();
	Super::Shutdown();
}

void USpaceShipGameInstance::HostLobby()
{
	if (bOperationInProgress)
	{
		return;
	}

	bOperationInProgress = true;
	HideLobbyMenu();
	SetLobbyStatus(FText::FromString(TEXT("Creating a 16-player LAN lobby...")));

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("Online sessions are unavailable.")));
		ShowLobbyMenu();
		return;
	}

	if (SessionInterface->GetNamedSession(LobbySessionName))
	{
		bCreateSessionAfterDestroy = true;
		DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
		if (!SessionInterface->DestroySession(LobbySessionName))
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			bCreateSessionAfterDestroy = false;
			bOperationInProgress = false;
			SetLobbyStatus(FText::FromString(TEXT("Could not close the previous lobby.")));
			ShowLobbyMenu();
		}
		return;
	}

	CreateLobbySession();
}

void USpaceShipGameInstance::JoinLobby()
{
	FindAndJoinLobby();
}

void USpaceShipGameInstance::FindAndJoinLobby()
{
	if (bOperationInProgress)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		SetLobbyStatus(FText::FromString(TEXT("Online sessions are unavailable.")));
		ShowLobbyMenu();
		return;
	}

	bOperationInProgress = true;
	HideLobbyMenu();
	SetLobbyStatus(FText::FromString(TEXT("Searching for LAN lobbies...")));
	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 100;

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("Could not search for LAN lobbies.")));
		ShowLobbyMenu();
	}
}

void USpaceShipGameInstance::LeaveLobby()
{
	if (bOperationInProgress)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(LobbySessionName))
	{
		bOperationInProgress = true;
		bReturnToMenuAfterDestroy = true;
		SetLobbyStatus(FText::FromString(TEXT("Leaving the lobby...")));
		DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
		if (!SessionInterface->DestroySession(LobbySessionName))
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			bReturnToMenuAfterDestroy = false;
			bOperationInProgress = false;
			ReturnToStandaloneLobby();
		}
		return;
	}

	ReturnToStandaloneLobby();
}

void USpaceShipGameInstance::QuitGame()
{
	FPlatformMisc::RequestExit(false);
}

FText USpaceShipGameInstance::GetLobbyStatus() const
{
	return LobbyStatus;
}

IOnlineSessionPtr USpaceShipGameInstance::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
}

void USpaceShipGameInstance::CreateLobbySession()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("Online sessions are unavailable.")));
		ShowLobbyMenu();
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = 16;
	SessionSettings.NumPrivateConnections = 0;
	SessionSettings.bIsLANMatch = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bAllowInvites = true;

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	if (!SessionInterface->CreateSession(0, LobbySessionName, SessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("Could not create the LAN lobby.")));
		ShowLobbyMenu();
	}
}

void USpaceShipGameInstance::ShowLobbyMenu()
{
	if (GetWorld() && GetWorld()->GetNetMode() != NM_Standalone)
	{
		return;
	}

	bMenuShouldBeVisible = true;
	if (!LobbyMenuWidget.IsValid())
	{
		LobbyMenuWidget = SNew(SLobbyMenuWidget).GameInstance(this);
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	if (!bLobbyMenuAddedToViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(LobbyMenuWidget.ToSharedRef(), 100);
		bLobbyMenuAddedToViewport = true;
	}
	if (APlayerController* PlayerController = GetFirstLocalPlayerController())
	{
		PlayerController->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(LobbyMenuWidget);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
}

void USpaceShipGameInstance::HideLobbyMenu()
{
	bMenuShouldBeVisible = false;
	if (bLobbyMenuAddedToViewport && GEngine && GEngine->GameViewport && LobbyMenuWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LobbyMenuWidget.ToSharedRef());
		bLobbyMenuAddedToViewport = false;
	}

	if (APlayerController* PlayerController = GetFirstLocalPlayerController())
	{
		PlayerController->bShowMouseCursor = false;
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void USpaceShipGameInstance::SetLobbyStatus(const FText& Status)
{
	LobbyStatus = Status;
}

void USpaceShipGameInstance::ReturnToStandaloneLobby()
{
	bOperationInProgress = false;
	bMenuShouldBeVisible = true;
	if (UWorld* World = GetWorld())
	{
		if (World->GetNetMode() != NM_Standalone)
		{
			UGameplayStatics::OpenLevel(World, LobbyMapName);
			return;
		}
	}
	ShowLobbyMenu();
}

void USpaceShipGameInstance::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	bOperationInProgress = false;
	if (!bWasSuccessful)
	{
		SetLobbyStatus(FText::FromString(TEXT("Could not create the LAN lobby.")));
		ShowLobbyMenu();
		return;
	}

	SetLobbyStatus(FText::FromString(TEXT("Lobby created. Waiting for players...")));
	UGameplayStatics::OpenLevel(this, LobbyMapName, true, TEXT("listen"));
}

void USpaceShipGameInstance::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("LAN lobby search failed.")));
		ShowLobbyMenu();
		return;
	}

	const FOnlineSessionSearchResult* AvailableSession = nullptr;
	for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
	{
		if (SearchResult.IsValid() && SearchResult.Session.NumOpenPublicConnections > 0)
		{
			AvailableSession = &SearchResult;
			break;
		}
	}

	if (!AvailableSession)
	{
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("No joinable LAN lobbies were found.")));
		ShowLobbyMenu();
		return;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("Online sessions are unavailable.")));
		ShowLobbyMenu();
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	if (!SessionInterface->JoinSession(0, LobbySessionName, *AvailableSession))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("Could not join the LAN lobby.")));
		ShowLobbyMenu();
	}
}

void USpaceShipGameInstance::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	FString ConnectString;
	if (Result == EOnJoinSessionCompleteResult::Success &&
		SessionInterface.IsValid() &&
		SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		bOperationInProgress = false;
		SetLobbyStatus(FText::FromString(TEXT("Joining lobby...")));
		HideLobbyMenu();
		if (APlayerController* PlayerController = GetFirstLocalPlayerController())
		{
			PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
			return;
		}
	}

	bOperationInProgress = false;
	SetLobbyStatus(FText::FromString(TEXT("Could not connect to the LAN lobby.")));
	ShowLobbyMenu();
}

void USpaceShipGameInstance::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	if (bCreateSessionAfterDestroy)
	{
		bCreateSessionAfterDestroy = false;
		if (!bWasSuccessful)
		{
			bOperationInProgress = false;
			SetLobbyStatus(FText::FromString(TEXT("Could not close the previous lobby.")));
			ShowLobbyMenu();
			return;
		}
		CreateLobbySession();
		return;
	}

	if (bReturnToMenuAfterDestroy)
	{
		bReturnToMenuAfterDestroy = false;
		ReturnToStandaloneLobby();
	}
}

void USpaceShipGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	RecoverFromTravelFailure(ErrorString);
}

void USpaceShipGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	RecoverFromTravelFailure(ErrorString);
}

void USpaceShipGameInstance::RecoverFromTravelFailure(const FString& ErrorString)
{
	const FString Message = ErrorString.IsEmpty() ? TEXT("The network travel failed.") : ErrorString;
	SetLobbyStatus(FText::FromString(Message));
	ReturnToStandaloneLobby();
}
