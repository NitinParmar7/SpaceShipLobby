// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SpaceShipCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UTextRenderComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ASpaceShipPortal;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN( LogTemplateCharacter, Log, All );

UCLASS( config=Game )
class ASpaceShipCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true") )
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true") )
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true") )
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true") )
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true") )
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true") )
	UInputAction* LookAction;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true") )
	UInputAction* InteractAction;

	UPROPERTY( Transient )
	UInputMappingContext* InteractMappingContext;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = "Player", meta = (AllowPrivateAccess = "true") )
	UTextRenderComponent* PlayerNameplate;

	UPROPERTY( EditDefaultsOnly, Category = "Player" )
	TObjectPtr<UMaterialInterface> PlayerColorMaterial;

	UPROPERTY( Transient )
	TObjectPtr<UMaterialInstanceDynamic> PlayerColorInstance;

	double LastPortalUseTime = -100.0;

public:
	ASpaceShipCharacter();

	virtual void BeginPlay() override;
	virtual void PossessedBy( AController* NewController ) override;
	virtual void OnRep_PlayerState() override;

	void RefreshPlayerPresentation();

	UFUNCTION( BlueprintPure, Category = "Portal" )
	ASpaceShipPortal* FindUsablePortal() const;

	/** Uses the nearest portal in range (same as pressing E) */
	UFUNCTION( BlueprintCallable, Category = "Portal" )
	void Interact();

protected:
	/** Called for movement input */
	void Move( const FInputActionValue& Value );

	/** Called for looking input */
	void Look( const FInputActionValue& Value );
	void ApplyPlayerColor();
	void UpdatePlayerNameplate();
	UFUNCTION( Server, Reliable )
	void Server_Interact();

protected:
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent( class UInputComponent* PlayerInputComponent ) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
