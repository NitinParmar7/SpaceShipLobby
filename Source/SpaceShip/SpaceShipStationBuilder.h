// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpaceShipStationBuilder.generated.h"

class ASpaceShipPortal;
class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class SPACESHIP_API ASpaceShipStationBuilder : public AActor
{
	GENERATED_BODY()

public:
	ASpaceShipStationBuilder();

	FVector GetLobbySpawnLocation() const;

protected:
	virtual void BeginPlay() override;

private:
	void BuildRoom( const FVector& RoomOrigin, bool bPortalOpeningOnRight );
	void AddBox( const FVector& Location, const FVector& Size );
	void AddRoomLight( const FVector& Location );

	UPROPERTY( EditDefaultsOnly, Category = "Station" )
	FVector StationOrigin = FVector( 50000.0f, 0.0f, 0.0f );

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY( Transient )
	TArray<TObjectPtr<UStaticMeshComponent>> StationComponents;

	UPROPERTY( Transient )
	TArray<TObjectPtr<UPointLightComponent>> RoomLights;

	UPROPERTY( Transient )
	TObjectPtr<ASpaceShipPortal> FirstPortal;

	UPROPERTY( Transient )
	TObjectPtr<ASpaceShipPortal> SecondPortal;
};