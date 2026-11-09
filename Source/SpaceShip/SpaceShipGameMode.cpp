// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceShipGameMode.h"
#include "SpaceShipCharacter.h"
#include "SpaceShipHUD.h"
#include "SpaceShipGameInstance.h"
#include "SpaceShipPlayerState.h"
#include "SpaceShipStationBuilder.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Engine/PlayerStartPIE.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ASpaceShipGameMode::ASpaceShipGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass( TEXT( "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter" ) );
	if( PlayerPawnBPClass.Class != NULL )
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	PlayerStateClass = ASpaceShipPlayerState::StaticClass();
	HUDClass = ASpaceShipHUD::StaticClass();
}

void ASpaceShipGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> StationBuilders;
	UGameplayStatics::GetAllActorsOfClass( this, ASpaceShipStationBuilder::StaticClass(), StationBuilders );
	if( !StationBuilders.IsEmpty() )
	{
		StationBuilder = Cast<ASpaceShipStationBuilder>( StationBuilders[ 0 ] );
	}

	if( !StationBuilder.IsValid() && bSpawnGeneratedStation && GetWorld() )
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		StationBuilder = GetWorld()->SpawnActor<ASpaceShipStationBuilder>( ASpaceShipStationBuilder::StaticClass(), FTransform::Identity, SpawnParameters );
	}
}

void ASpaceShipGameMode::PreLogin( const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage )
{
	Super::PreLogin( Options, Address, UniqueId, ErrorMessage );

	if( ErrorMessage.IsEmpty() && GetNumPlayers() >= FMath::Min( MaxLobbyPlayers, ASpaceShipPlayerState::PlayerColorCount ) )
	{
		ErrorMessage = TEXT( "Lobby is full." );
		UE_LOG( LogTemp, Warning, TEXT("Rejected lobby connection from %s: lobby is full."), *Address );
	}
}

FString ASpaceShipGameMode::InitNewPlayer( APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal )
{
	const FString ErrorMessage = Super::InitNewPlayer( NewPlayer, UniqueId, Options, Portal );
	if( !ErrorMessage.IsEmpty() || !IsValid( NewPlayer ) )
	{
		return ErrorMessage;
	}

	if( ASpaceShipPlayerState* SpaceShipPlayerState = NewPlayer->GetPlayerState<ASpaceShipPlayerState>() )
	{
		const FString ProfilePlayerName = FGenericPlatformHttp::UrlDecode( UGameplayStatics::ParseOption( Options, TEXT( "PlayerName" ) ) );
		if( !ProfilePlayerName.IsEmpty() )
		{
			SpaceShipPlayerState->SetPlayerName( ProfilePlayerName );
		}

		for( int32 ColorIndex = 0; ColorIndex < ASpaceShipPlayerState::PlayerColorCount; ++ColorIndex )
		{
			bool bInUse = false;
			for( const auto& Entry : PlayerColorSlots )
			{
				bInUse |= Entry.Value == ColorIndex;
			}
			if( !bInUse )
			{
				PlayerColorSlots.Add( NewPlayer, ColorIndex );
				SpaceShipPlayerState->SetPlayerColorIndex( ColorIndex );
				UE_LOG( LogTemp, Log, TEXT( "Assigned player color %d to %s" ), ColorIndex, *SpaceShipPlayerState->GetPlayerName() );
				break;
			}
		}
	}

	return ErrorMessage;
}

void ASpaceShipGameMode::PostLogin( APlayerController* NewPlayer )
{
	Super::PostLogin( NewPlayer );

	if( NewPlayer && NewPlayer->IsLocalController() )
	{
		if( ASpaceShipPlayerState* LocalPlayerState = NewPlayer->GetPlayerState<ASpaceShipPlayerState>() )
		{
			if( const UWorld* World = GetWorld() )
			{
				if( const USpaceShipGameInstance* GameInstance = Cast<USpaceShipGameInstance>( World->GetGameInstance() ) )
				{
					const FString LocalPlayerName = GameInstance->GetPlayerDisplayName();
					if( !LocalPlayerName.IsEmpty() )
					{
						LocalPlayerState->SetPlayerName( LocalPlayerName );
					}
				}
			}
		}
	}

	const FString PlayerName = NewPlayer && NewPlayer->PlayerState
		? NewPlayer->PlayerState->GetPlayerName()
		: TEXT( "Unknown" );
	if( ASpaceShipPlayerState* SpaceShipPlayerState = NewPlayer ? NewPlayer->GetPlayerState<ASpaceShipPlayerState>() : nullptr )
	{
		if( SpaceShipPlayerState->GetInventory().IsEmpty() )
		{
			SpaceShipPlayerState->SetInventory( { FName( TEXT( "OxygenTank" ) ), FName( TEXT( "StationKeycard" ) ) } );
		}
	}
	if( ASpaceShipCharacter* Character = NewPlayer ? Cast<ASpaceShipCharacter>( NewPlayer->GetPawn() ) : nullptr )
	{
		Character->RefreshPlayerPresentation();
	}
	UE_LOG( LogTemp, Log, TEXT("Lobby player joined: %s (%d/%d)"), *PlayerName, GetNumPlayers(), MaxLobbyPlayers );
}

void ASpaceShipGameMode::Logout( AController* Exiting )
{
	PlayerColorSlots.Remove( Exiting );
	const FString PlayerName = Exiting && Exiting->PlayerState
		? Exiting->PlayerState->GetPlayerName()
		: TEXT( "Unknown" );

	int32 SpawnSlot = INDEX_NONE;
	if( PlayerSpawnSlots.RemoveAndCopyValue( Exiting, SpawnSlot ) )
	{
		if( APlayerStart** SpawnStart = SpawnStartsBySlot.Find( SpawnSlot ) )
		{
			if( IsValid( *SpawnStart ) )
			{
				(*SpawnStart)->Destroy();
			}
			SpawnStartsBySlot.Remove( SpawnSlot );
		}
	}

	Super::Logout( Exiting );
	UE_LOG( LogTemp, Log, TEXT("Lobby player left: %s (%d/%d)"), *PlayerName, GetNumPlayers(), MaxLobbyPlayers );
}

AActor* ASpaceShipGameMode::ChoosePlayerStart_Implementation( AController* Player )
{
	if( !IsValid( Player ) || !GetWorld() )
	{
		return Super::ChoosePlayerStart_Implementation( Player );
	}

	if( const int32* ExistingSlot = PlayerSpawnSlots.Find( Player ) )
	{
		if( APlayerStart* const* ExistingStart = SpawnStartsBySlot.Find( *ExistingSlot ); ExistingStart && IsValid( *ExistingStart ) )
		{
			return *ExistingStart;
		}
	}

	if( !bCheckedForBasePlayerStart )
	{
		bCheckedForBasePlayerStart = true;
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass( this, APlayerStart::StaticClass(), PlayerStarts );
		for( AActor* PlayerStartActor : PlayerStarts )
		{
			APlayerStart* PlayerStart = Cast<APlayerStart>( PlayerStartActor );
			if( IsValid( PlayerStart ) && !PlayerStart->IsA<APlayerStartPIE>() && PlayerStart->IsNetStartupActor() )
			{
				BasePlayerStart = PlayerStart;
				break;
			}
		}
	}

	int32 SpawnSlot = 0;
	while( SpawnSlot < MaxLobbyPlayers && SpawnStartsBySlot.Contains( SpawnSlot ) )
	{
		++SpawnSlot;
	}
	if( SpawnSlot >= MaxLobbyPlayers )
	{
		return Super::ChoosePlayerStart_Implementation( Player );
	}

	FTransform SpawnTransform = BasePlayerStart.IsValid()
		? BasePlayerStart->GetActorTransform()
		: FTransform( FRotator::ZeroRotator, StationBuilder.IsValid() ? StationBuilder->GetLobbySpawnLocation() : LobbySpawnOrigin );
	constexpr int32 GridWidth = 4;
	constexpr float GridSpacing = 250.0f;
	const int32 Column = SpawnSlot % GridWidth;
	const int32 Row = SpawnSlot / GridWidth;
	const FVector GridOffset(
		(static_cast<float>(Column) - 1.5f) * GridSpacing,
		(static_cast<float>(Row) - 1.5f) * GridSpacing,
		0.0f );
	SpawnTransform.SetLocation( SpawnTransform.GetLocation() + SpawnTransform.TransformVector( GridOffset ) );

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerStart* SpawnStart = GetWorld()->SpawnActor<APlayerStart>( APlayerStart::StaticClass(), SpawnTransform, SpawnParameters );
	if( !IsValid( SpawnStart ) )
	{
		return Super::ChoosePlayerStart_Implementation( Player );
	}

	PlayerSpawnSlots.Add( Player, SpawnSlot );
	SpawnStartsBySlot.Add( SpawnSlot, SpawnStart );
	return SpawnStart;
}
