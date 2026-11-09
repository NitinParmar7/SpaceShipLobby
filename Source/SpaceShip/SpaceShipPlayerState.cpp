// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceShipPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "SpaceShipCharacter.h"

ASpaceShipPlayerState::ASpaceShipPlayerState()
{
	bReplicates = true;
}

FLinearColor ASpaceShipPlayerState::GetPlayerColor() const
{
	static const FColor PlayerColors[ PlayerColorCount ] = {
		FColor( 230, 25, 75 ), FColor( 60, 180, 75 ), FColor( 0, 130, 230 ), FColor( 255, 225, 25 ),
		FColor( 245, 130, 48 ), FColor( 145, 30, 180 ), FColor( 70, 240, 240 ), FColor( 240, 50, 230 ),
		FColor( 210, 245, 60 ), FColor( 250, 190, 212 ), FColor( 0, 128, 128 ), FColor( 220, 190, 255 ),
		FColor( 170, 110, 40 ), FColor( 128, 0, 0 ), FColor( 0, 0, 128 ), FColor( 255, 255, 255 )
	};
	return PlayerColorIndex >= 0 && PlayerColorIndex < PlayerColorCount
		? FLinearColor::FromSRGBColor( PlayerColors[ PlayerColorIndex ] ) : FLinearColor::White;
}

void ASpaceShipPlayerState::SetPlayerColorIndex( int32 NewColorIndex )
{
	if( !HasAuthority() || NewColorIndex < 0 || NewColorIndex >= PlayerColorCount || PlayerColorIndex == NewColorIndex )
	{
		return;
	}

	PlayerColorIndex = NewColorIndex;
	NotifyPawnOfProfileChange();
	ForceNetUpdate();
}

void ASpaceShipPlayerState::SetInventory( const TArray<FName>& NewInventory )
{
	Inventory = NewInventory;
}

void ASpaceShipPlayerState::CopyProperties( APlayerState* NewPlayerState )
{
	Super::CopyProperties( NewPlayerState );

	if( ASpaceShipPlayerState* NewSpaceShipPlayerState = Cast<ASpaceShipPlayerState>( NewPlayerState ) )
	{
		NewSpaceShipPlayerState->PlayerColorIndex = PlayerColorIndex;
		NewSpaceShipPlayerState->Inventory = Inventory;
	}
}

void ASpaceShipPlayerState::OverrideWith( APlayerState* OldPlayerState )
{
	Super::OverrideWith( OldPlayerState );

	if( const ASpaceShipPlayerState* OldSpaceShipPlayerState = Cast<ASpaceShipPlayerState>( OldPlayerState ) )
	{
		PlayerColorIndex = OldSpaceShipPlayerState->PlayerColorIndex;
		Inventory = OldSpaceShipPlayerState->Inventory;
		NotifyPawnOfProfileChange();
	}
}

void ASpaceShipPlayerState::OnRep_PlayerColorIndex()
{
	NotifyPawnOfProfileChange();
}

void ASpaceShipPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	NotifyPawnOfProfileChange();
}

void ASpaceShipPlayerState::NotifyPawnOfProfileChange()
{
	if( ASpaceShipCharacter* SpaceShipCharacter = Cast<ASpaceShipCharacter>( GetPawn() ) )
	{
		SpaceShipCharacter->RefreshPlayerPresentation();
	}
}

void ASpaceShipPlayerState::GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	DOREPLIFETIME( ASpaceShipPlayerState, PlayerColorIndex );
	DOREPLIFETIME( ASpaceShipPlayerState, Inventory );
}
