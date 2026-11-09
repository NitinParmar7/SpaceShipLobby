// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceShipCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "SpaceShipPlayerState.h"
#include "SpaceShipPortal.h"

DEFINE_LOG_CATEGORY( LogTemplateCharacter );

//////////////////////////////////////////////////////////////////////////
// ASpaceShipCharacter

ASpaceShipCharacter::ASpaceShipCharacter()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterial( TEXT( "/Game/Characters/Materials/M_PlayerColor.M_PlayerColor" ) );
	PlayerColorMaterial = ColorMaterial.Object;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize( 42.f, 96.0f );

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator( 0.0f, 500.0f, 0.0f ); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>( TEXT( "CameraBoom" ) );
	CameraBoom->SetupAttachment( RootComponent );
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>( TEXT( "FollowCamera" ) );
	FollowCamera->SetupAttachment( CameraBoom, USpringArmComponent::SocketName ); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	InteractAction = CreateDefaultSubobject<UInputAction>( TEXT( "InteractAction" ) );
	InteractAction->ValueType = EInputActionValueType::Boolean;
	InteractMappingContext = CreateDefaultSubobject<UInputMappingContext>( TEXT( "InteractMappingContext" ) );
	InteractMappingContext->MapKey( InteractAction, EKeys::E );

	PlayerNameplate = CreateDefaultSubobject<UTextRenderComponent>( TEXT( "PlayerNameplate" ) );
	PlayerNameplate->SetupAttachment( GetRootComponent() );
	PlayerNameplate->SetRelativeLocation( FVector( 0.0f, 0.0f, 220.0f ) );
	PlayerNameplate->SetHorizontalAlignment( EHTA_Center );
	PlayerNameplate->SetWorldSize( 38.0f );

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASpaceShipCharacter::BeginPlay()
{
	Super::BeginPlay();
	RefreshPlayerPresentation();
}

void ASpaceShipCharacter::PossessedBy( AController* NewController )
{
	Super::PossessedBy( NewController );
	RefreshPlayerPresentation();
}

void ASpaceShipCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	RefreshPlayerPresentation();
}

void ASpaceShipCharacter::RefreshPlayerPresentation()
{
	UpdatePlayerNameplate();
	ApplyPlayerColor();
}

void ASpaceShipCharacter::UpdatePlayerNameplate()
{
	if( PlayerNameplate )
	{
		const APlayerState* CurrentPlayerState = GetPlayerState();
		PlayerNameplate->SetText( CurrentPlayerState ? FText::FromString( CurrentPlayerState->GetPlayerName() ) : FText::GetEmpty() );
	}
}

void ASpaceShipCharacter::ApplyPlayerColor()
{
	const ASpaceShipPlayerState* State = GetPlayerState<ASpaceShipPlayerState>();
	if( !State || !PlayerColorMaterial || GetNetMode() == NM_DedicatedServer )
	{
		return;
	}
	if( !PlayerColorInstance )
	{
		PlayerColorInstance = UMaterialInstanceDynamic::Create( PlayerColorMaterial, this );
	}
	PlayerColorInstance->SetVectorParameterValue( TEXT( "PlayerColor" ), State->GetPlayerColor() );
	for( int32 Slot = 0; Slot < GetMesh()->GetNumMaterials(); ++Slot )
	{
		GetMesh()->SetMaterial( Slot, PlayerColorInstance );
	}
	UE_LOG( LogTemplateCharacter, Log, TEXT( "Applied player color %d to %s (%d material slots)" ),
		State->GetPlayerColorIndex(), *State->GetPlayerName(), GetMesh()->GetNumMaterials() );
}

void ASpaceShipCharacter::Interact()
{
	if( IsLocallyControlled() )
	{
		Server_Interact();
	}
}

ASpaceShipPortal* ASpaceShipCharacter::FindUsablePortal() const
{
	UWorld* World = GetWorld();
	if( !World )
	{
		return nullptr;
	}

	ASpaceShipPortal* ClosestPortal = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	for( TActorIterator<ASpaceShipPortal> It( World ); It; ++It )
	{
		const float DistanceSquared = FVector::DistSquared( GetActorLocation(), It->GetActorLocation() );
		if( IsValid( It->DestinationPortal ) && DistanceSquared <= FMath::Square( It->InteractionRadius ) && DistanceSquared < ClosestDistanceSquared )
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestPortal = *It;
		}
	}
	return ClosestPortal;
}

void ASpaceShipCharacter::Server_Interact_Implementation()
{
	UWorld* World = GetWorld();
	if( !World || !HasAuthority() || World->GetTimeSeconds() - LastPortalUseTime < 1.0 )
	{
		return;
	}
	ASpaceShipPortal* ClosestPortal = FindUsablePortal();

	if( IsValid( ClosestPortal ) && ClosestPortal->TeleportCharacter( this ) )
	{
		LastPortalUseTime = World->GetTimeSeconds();
	}
}

void ASpaceShipCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if( APlayerController* PlayerController = Cast<APlayerController>( Controller ) )
	{
		if( UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>( PlayerController->GetLocalPlayer() ) )
		{
			if( DefaultMappingContext )
			{
				Subsystem->AddMappingContext( DefaultMappingContext, 0 );
			}
			if( InteractMappingContext )
			{
				Subsystem->AddMappingContext( InteractMappingContext, 1 );
			}
		}
	}
}

void ASpaceShipCharacter::SetupPlayerInputComponent( UInputComponent* PlayerInputComponent )
{
	// Set up action bindings
	if( UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>( PlayerInputComponent ) )
	{

		// Jumping
		if( JumpAction )
		{
			EnhancedInputComponent->BindAction( JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump );
			EnhancedInputComponent->BindAction( JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping );
		}

		// Moving
		if( MoveAction )
		{
			EnhancedInputComponent->BindAction( MoveAction, ETriggerEvent::Triggered, this, &ASpaceShipCharacter::Move );
		}

		// Looking
		if( LookAction )
		{
			EnhancedInputComponent->BindAction( LookAction, ETriggerEvent::Triggered, this, &ASpaceShipCharacter::Look );
		}
		if( InteractAction )
		{
			EnhancedInputComponent->BindAction( InteractAction, ETriggerEvent::Started, this, &ASpaceShipCharacter::Interact );
		}
	}
	else
	{
		UE_LOG( LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this) );
	}
}

void ASpaceShipCharacter::Move( const FInputActionValue& Value )
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if( Controller != nullptr )
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation( 0, Rotation.Yaw, 0 );

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix( YawRotation ).GetUnitAxis( EAxis::X );

		// get right vector 
		const FVector RightDirection = FRotationMatrix( YawRotation ).GetUnitAxis( EAxis::Y );

		// add movement 
		AddMovementInput( ForwardDirection, MovementVector.Y );
		AddMovementInput( RightDirection, MovementVector.X );
	}
}

void ASpaceShipCharacter::Look( const FInputActionValue& Value )
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if( Controller != nullptr )
	{
		// add yaw and pitch input to controller
		AddControllerYawInput( LookAxisVector.X );
		AddControllerPitchInput( LookAxisVector.Y );
	}
}
