// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceShipGameMode.h"
#include "SpaceShipCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "UObject/ConstructorHelpers.h"

ASpaceShipGameMode::ASpaceShipGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void ASpaceShipGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= MaxLobbyPlayers)
	{
		ErrorMessage = TEXT("Lobby is full.");
		UE_LOG(LogTemp, Warning, TEXT("Rejected lobby connection from %s: lobby is full."), *Address);
	}
}

void ASpaceShipGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const FString PlayerName = NewPlayer && NewPlayer->PlayerState
		? NewPlayer->PlayerState->GetPlayerName()
		: TEXT("Unknown");
	UE_LOG(LogTemp, Log, TEXT("Lobby player joined: %s (%d/%d)"), *PlayerName, GetNumPlayers(), MaxLobbyPlayers);
}

void ASpaceShipGameMode::Logout(AController* Exiting)
{
	const FString PlayerName = Exiting && Exiting->PlayerState
		? Exiting->PlayerState->GetPlayerName()
		: TEXT("Unknown");

	int32 SpawnSlot = INDEX_NONE;
	if (PlayerSpawnSlots.RemoveAndCopyValue(Exiting, SpawnSlot))
	{
		if (APlayerStart** SpawnStart = SpawnStartsBySlot.Find(SpawnSlot))
		{
			if (IsValid(*SpawnStart))
			{
				(*SpawnStart)->Destroy();
			}
			SpawnStartsBySlot.Remove(SpawnSlot);
		}
	}

	Super::Logout(Exiting);
	UE_LOG(LogTemp, Log, TEXT("Lobby player left: %s (%d/%d)"), *PlayerName, GetNumPlayers(), MaxLobbyPlayers);
}

AActor* ASpaceShipGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!IsValid(Player) || !GetWorld())
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	if (const int32* ExistingSlot = PlayerSpawnSlots.Find(Player))
	{
		if (APlayerStart* const* ExistingStart = SpawnStartsBySlot.Find(*ExistingSlot); ExistingStart && IsValid(*ExistingStart))
		{
			return *ExistingStart;
		}
	}

	if (!bCheckedForBasePlayerStart)
	{
		bCheckedForBasePlayerStart = true;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			BasePlayerStart = *It;
			break;
		}
	}

	int32 SpawnSlot = 0;
	while (SpawnSlot < MaxLobbyPlayers && SpawnStartsBySlot.Contains(SpawnSlot))
	{
		++SpawnSlot;
	}
	if (SpawnSlot >= MaxLobbyPlayers)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	FTransform SpawnTransform = BasePlayerStart.IsValid()
		? BasePlayerStart->GetActorTransform()
		: FTransform(FRotator::ZeroRotator, LobbySpawnOrigin);
	constexpr int32 GridWidth = 4;
	constexpr float GridSpacing = 250.0f;
	const int32 Column = SpawnSlot % GridWidth;
	const int32 Row = SpawnSlot / GridWidth;
	const FVector GridOffset(
		(static_cast<float>(Column) - 1.5f) * GridSpacing,
		(static_cast<float>(Row) - 1.5f) * GridSpacing,
		0.0f);
	SpawnTransform.SetLocation(SpawnTransform.GetLocation() + SpawnTransform.TransformVector(GridOffset));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerStart* SpawnStart = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnTransform, SpawnParameters);
	if (!IsValid(SpawnStart))
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	PlayerSpawnSlots.Add(Player, SpawnSlot);
	SpawnStartsBySlot.Add(SpawnSlot, SpawnStart);
	return SpawnStart;
}
