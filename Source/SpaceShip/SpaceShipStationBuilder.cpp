// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceShipStationBuilder.h"

#include "SpaceShipPortal.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float RoomHalfWidth = 3000.0f;
	constexpr float RoomHalfDepth = 3000.0f;
	constexpr float RoomHeight = 2400.0f;
	constexpr float WallThickness = 120.0f;
	constexpr float RoomSeparation = 8000.0f;
}

ASpaceShipStationBuilder::ASpaceShipStationBuilder()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>( TEXT( "SceneRoot" ) );
	SceneRoot->SetMobility( EComponentMobility::Static );
	SetRootComponent( SceneRoot );
}

FVector ASpaceShipStationBuilder::GetLobbySpawnLocation() const
{
	return StationOrigin + FVector( 0.0f, 0.0f, 100.0f );
}

void ASpaceShipStationBuilder::BeginPlay()
{
	Super::BeginPlay();
	if( !GetWorld() )
	{
		return;
	}

	const FVector SecondRoomOrigin = StationOrigin + FVector( RoomSeparation, 0.0f, 0.0f );
	BuildRoom( StationOrigin, true );
	BuildRoom( SecondRoomOrigin, false );

	if( !HasAuthority() )
	{
		return;
	}

	FActorSpawnParameters PortalSpawnParameters;
	PortalSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FirstPortal = GetWorld()->SpawnActor<ASpaceShipPortal>( ASpaceShipPortal::StaticClass(), FTransform( FRotator( 0.0f, 180.0f, 0.0f ), StationOrigin + FVector( RoomHalfWidth - 80.0f, 0.0f, 0.0f ) ), PortalSpawnParameters );
	SecondPortal = GetWorld()->SpawnActor<ASpaceShipPortal>( ASpaceShipPortal::StaticClass(), FTransform( FRotator::ZeroRotator, SecondRoomOrigin + FVector( -RoomHalfWidth + 80.0f, 0.0f, 0.0f ) ), PortalSpawnParameters );

	if( IsValid( FirstPortal ) && IsValid( SecondPortal ) )
	{
		FirstPortal->DestinationPortal = SecondPortal;
		SecondPortal->DestinationPortal = FirstPortal;
	}
}

void ASpaceShipStationBuilder::BuildRoom( const FVector& RoomOrigin, bool bPortalOpeningOnRight )
{
	AddBox( RoomOrigin + FVector( 0.0f, 0.0f, -50.0f ), FVector( RoomHalfWidth * 2.0f, RoomHalfDepth * 2.0f, 100.0f ) );
	AddBox( RoomOrigin + FVector( 0.0f, -RoomHalfDepth, RoomHeight * 0.5f ), FVector( RoomHalfWidth * 2.0f, WallThickness, RoomHeight ) );
	AddBox( RoomOrigin + FVector( 0.0f, RoomHalfDepth, RoomHeight * 0.5f ), FVector( RoomHalfWidth * 2.0f, WallThickness, RoomHeight ) );
	AddBox( RoomOrigin + FVector( -RoomHalfWidth, 0.0f, RoomHeight * 0.5f ), FVector( WallThickness, RoomHalfDepth * 2.0f, RoomHeight ) );

	if( bPortalOpeningOnRight )
	{
		AddBox( RoomOrigin + FVector( RoomHalfWidth, -1700.0f, RoomHeight * 0.5f ), FVector( WallThickness, 2600.0f, RoomHeight ) );
		AddBox( RoomOrigin + FVector( RoomHalfWidth, 1700.0f, RoomHeight * 0.5f ), FVector( WallThickness, 2600.0f, RoomHeight ) );
		AddBox( RoomOrigin + FVector( RoomHalfWidth, 0.0f, 2250.0f ), FVector( WallThickness, 800.0f, 300.0f ) );
	}
	else
	{
		AddBox( RoomOrigin + FVector( RoomHalfWidth, 0.0f, RoomHeight * 0.5f ), FVector( WallThickness, RoomHalfDepth * 2.0f, RoomHeight ) );
		AddBox( RoomOrigin + FVector( -RoomHalfWidth, -1700.0f, RoomHeight * 0.5f ), FVector( WallThickness, 2600.0f, RoomHeight ) );
		AddBox( RoomOrigin + FVector( -RoomHalfWidth, 1700.0f, RoomHeight * 0.5f ), FVector( WallThickness, 2600.0f, RoomHeight ) );
		AddBox( RoomOrigin + FVector( -RoomHalfWidth, 0.0f, 2250.0f ), FVector( WallThickness, 800.0f, 300.0f ) );
	}

	AddRoomLight( RoomOrigin + FVector( 0.0f, 0.0f, RoomHeight - 100.0f ) );
}

void ASpaceShipStationBuilder::AddBox( const FVector& Location, const FVector& Size )
{
	static UStaticMesh* CubeMesh = LoadObject<UStaticMesh>( nullptr, TEXT( "/Engine/BasicShapes/Cube.Cube" ) );
	if( !CubeMesh )
	{
		UE_LOG( LogTemp, Warning, TEXT("Could not load the engine cube mesh for station geometry.") );
		return;
	}

	const FName ComponentName( *FString::Printf( TEXT( "StationBox_%02d" ), StationComponents.Num() ) );
	UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>( this, ComponentName );
	if( !Box )
	{
		return;
	}
	Box->SetNetAddressable();
	Box->SetupAttachment( SceneRoot );
	Box->SetStaticMesh( CubeMesh );
	Box->SetRelativeLocation( Location - GetActorLocation() );
	Box->SetRelativeScale3D( Size / 100.0f );
	Box->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
	Box->SetMobility( EComponentMobility::Static );
	Box->RegisterComponent();
	StationComponents.Add( Box );
}

void ASpaceShipStationBuilder::AddRoomLight( const FVector& Location )
{
	const FName ComponentName( *FString::Printf( TEXT( "StationLight_%02d" ), RoomLights.Num() ) );
	UPointLightComponent* Light = NewObject<UPointLightComponent>( this, ComponentName );
	if( !Light )
	{
		return;
	}
	Light->SetNetAddressable();
	Light->SetupAttachment( SceneRoot );
	Light->SetRelativeLocation( Location - GetActorLocation() );
	Light->SetIntensity( 9000.0f );
	Light->SetAttenuationRadius( 6000.0f );
	Light->SetMobility( EComponentMobility::Movable );
	Light->RegisterComponent();
	RoomLights.Add( Light );
}