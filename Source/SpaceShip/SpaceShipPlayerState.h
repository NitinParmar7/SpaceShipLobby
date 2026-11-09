// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SpaceShipPlayerState.generated.h"

UCLASS()
class SPACESHIP_API ASpaceShipPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASpaceShipPlayerState();

	static constexpr int32 PlayerColorCount = 16;
	void SetPlayerColorIndex( int32 NewColorIndex );
	void SetInventory( const TArray<FName>& NewInventory );

	int32 GetPlayerColorIndex() const { return PlayerColorIndex; }
	FLinearColor GetPlayerColor() const;
	const TArray<FName>& GetInventory() const { return Inventory; }

	virtual void CopyProperties( APlayerState* NewPlayerState ) override;
	virtual void OverrideWith( APlayerState* OldPlayerState ) override;
	virtual void OnRep_PlayerName() override;
	virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;

protected:
	UPROPERTY( ReplicatedUsing = OnRep_PlayerColorIndex, BlueprintReadOnly, Category = "Player" )
	int32 PlayerColorIndex = INDEX_NONE;

	UPROPERTY( Replicated, BlueprintReadOnly, Category = "Player" )
	TArray<FName> Inventory;

	UFUNCTION()
	void OnRep_PlayerColorIndex();

private:
	void NotifyPawnOfProfileChange();
};
