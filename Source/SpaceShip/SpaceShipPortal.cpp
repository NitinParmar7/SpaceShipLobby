// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceShipPortal.h"
#include "Components/ChildActorComponent.h"
#include "Net/UnrealNetwork.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

ASpaceShipPortal::ASpaceShipPortal()
{
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>( TEXT( "SceneRoot" ) );
	SceneRoot->SetMobility( EComponentMobility::Static );
	SetRootComponent( SceneRoot );

	PortalVFX = CreateDefaultSubobject<UChildActorComponent>( TEXT( "PortalVFX" ) );
	PortalVFX->SetMobility( EComponentMobility::Static );
	PortalVFX->SetupAttachment( SceneRoot );
	PortalVFX->SetRelativeLocation( FVector( 0.0f, 0.0f, 130.0f ) );
	static ConstructorHelpers::FClassFinder<AActor> PortalEffect( TEXT( "/Game/AdvancedPortalsSystemVFX/Blueprints/BP_Portal_1" ) );
	if( PortalEffect.Succeeded() )
	{
		PortalVFX->SetChildActorClass( PortalEffect.Class );
	}

	InteractionVolume = CreateDefaultSubobject<USphereComponent>( TEXT( "InteractionVolume" ) );
	InteractionVolume->SetupAttachment( SceneRoot );
	InteractionVolume->InitSphereRadius( InteractionRadius );
	InteractionVolume->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
	InteractionVolume->SetCollisionResponseToAllChannels( ECR_Ignore );
	InteractionVolume->SetCollisionResponseToChannel( ECC_Pawn, ECR_Overlap );

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder( TEXT( "/Engine/BasicShapes/Cube.Cube" ) );
	if( CubeMeshFinder.Succeeded() )
	{
		UStaticMesh* CubeMesh = CubeMeshFinder.Object;
		const auto AddFramePiece = [this, CubeMesh]( const TCHAR* ComponentName, const FVector& Location, const FVector& Scale )
		{
			UStaticMeshComponent* Piece = CreateDefaultSubobject<UStaticMeshComponent>( ComponentName );
			Piece->SetupAttachment( SceneRoot );
			Piece->SetStaticMesh( CubeMesh );
			Piece->SetRelativeLocation( Location );
			Piece->SetRelativeScale3D( Scale );
			Piece->SetCollisionEnabled( ECollisionEnabled::NoCollision );
			PortalFrame.Add( Piece );
		};

		AddFramePiece( TEXT( "PortalFrameLeft" ), FVector( 0.0f, -115.0f, 130.0f ), FVector( 0.22f, 0.35f, 2.6f ) );
		AddFramePiece( TEXT( "PortalFrameRight" ), FVector( 0.0f, 115.0f, 130.0f ), FVector( 0.22f, 0.35f, 2.6f ) );
		AddFramePiece( TEXT( "PortalFrameTop" ), FVector( 0.0f, 0.0f, 255.0f ), FVector( 0.22f, 2.65f, 0.35f ) );
	}

	PromptText = CreateDefaultSubobject<UTextRenderComponent>( TEXT( "PortalPrompt" ) );
	PromptText->SetupAttachment( SceneRoot );
	PromptText->SetRelativeLocation( FVector( 0.0f, 0.0f, 330.0f ) );
	PromptText->SetRelativeRotation( FRotator( 0.0f, 90.0f, 0.0f ) );
	PromptText->SetHorizontalAlignment( EHTA_Center );
	PromptText->SetWorldSize( 38.0f );
	PromptMessage = FText::FromString( TEXT( "Press E to use portal" ) );
	PromptText->SetText( PromptMessage );
	PromptText->SetVisibility( false );
}

void ASpaceShipPortal::ConfigurePortalVFX()
{
	SceneRoot->SetMobility( EComponentMobility::Static );
	PortalVFX->SetMobility( EComponentMobility::Static );
	AActor* EffectActor = PortalVFX ? PortalVFX->GetChildActor() : nullptr;
	USceneComponent* EffectRoot = EffectActor ? EffectActor->GetRootComponent() : nullptr;
	if( !EffectRoot )
	{
		return;
	}

	if( EffectRoot->GetAttachParent() != PortalVFX )
	{
		EffectRoot->AttachToComponent( PortalVFX, FAttachmentTransformRules::SnapToTargetNotIncludingScale );
	}
	EffectActor->SetActorEnableCollision( false );
}

void ASpaceShipPortal::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	ConfigurePortalVFX();
}

void ASpaceShipPortal::OnConstruction( const FTransform& Transform )
{
	Super::OnConstruction( Transform );
	ConfigurePortalVFX();

	if( InteractionVolume )
	{
		InteractionVolume->SetSphereRadius( InteractionRadius );
	}
	if( PromptText )
	{
		PromptText->SetText( PromptMessage );
	}
	for( UStaticMeshComponent* FramePiece : PortalFrame )
	{
		if( IsValid( FramePiece ) )
		{
			FramePiece->SetVisibility( bShowFrame, true );
		}
	}
}

void ASpaceShipPortal::BeginPlay()
{
	Super::BeginPlay();
	ConfigurePortalVFX();
	if( HasAuthority() && bTwoWay && IsValid( DestinationPortal ) && !IsValid( DestinationPortal->DestinationPortal ) )
	{
		DestinationPortal->DestinationPortal = this;
	}
	
	PromptText->SetVisibility( false );
	if( AActor* EffectActor = PortalVFX->GetChildActor() )
	{
		EffectActor->SetActorEnableCollision( false );
		EffectActor->SetActorHiddenInGame( GetNetMode() == NM_DedicatedServer );
	}
}

void ASpaceShipPortal::GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	DOREPLIFETIME( ASpaceShipPortal, DestinationPortal );
}

bool ASpaceShipPortal::TeleportCharacter( ACharacter* Character ) const
{
	if( !HasAuthority() || !IsValid( Character ) || !IsValid( DestinationPortal ) )
	{
		return false;
	}

	const FVector PortalLocation = DestinationPortal->GetActorLocation();
	const FVector Destination = PortalLocation + DestinationPortal->GetActorForwardVector() * ExitDistance + FVector( 0.0f, 0.0f, 10.0f );
	const FRotator DestinationRotation( 0.0f, DestinationPortal->GetActorRotation().Yaw, 0.0f );
	if( !Character->TeleportTo( Destination, DestinationRotation, false, false ) &&
		!Character->TeleportTo( PortalLocation + FVector( 0.0f, 0.0f, 10.0f ), DestinationRotation, false, false ) )
	{
		return false;
	}

	if( APlayerController* PlayerController = Cast<APlayerController>( Character->GetController() ) )
	{
		PlayerController->SetControlRotation( DestinationRotation );
	}
	return true;
}

void ASpaceShipPortal::HandleBeginOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
	if( const APawn* Pawn = Cast<APawn>( OtherActor ); Pawn && Pawn->IsLocallyControlled() )
	{
		PromptText->SetVisibility( true );
	}
}

void ASpaceShipPortal::HandleEndOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex )
{
	if( const APawn* Pawn = Cast<APawn>( OtherActor ); Pawn && Pawn->IsLocallyControlled() )
	{
		PromptText->SetVisibility( false );
	}
}
