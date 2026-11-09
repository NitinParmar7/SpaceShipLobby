// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpaceShipPortal.generated.h"

class ACharacter;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UChildActorComponent;

UCLASS( Blueprintable )
class SPACESHIP_API ASpaceShipPortal : public AActor
{
	GENERATED_BODY()

public:
	ASpaceShipPortal();

	bool TeleportCharacter( ACharacter* Character ) const;
	virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;

	UPROPERTY( EditAnywhere, Replicated, Category = "Portal" )
	TObjectPtr<ASpaceShipPortal> DestinationPortal;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = "Portal|VFX" )
	TObjectPtr<UChildActorComponent> PortalVFX;

	UPROPERTY( EditAnywhere, Category = "Portal" )
	bool bTwoWay = true;

	UPROPERTY( EditAnywhere, Category = "Portal", meta = ( ClampMin = "0.0" ) )
	float ExitDistance = 200.0f;

	UPROPERTY( EditAnywhere, Category = "Portal", meta = ( ClampMin = "0.0" ) )
	float InteractionRadius = 400.0f;

	UPROPERTY( EditAnywhere, Category = "Portal" )
	bool bShowFrame = false;

	UPROPERTY( EditAnywhere, Category = "Portal" )
	FText PromptMessage;

protected:
	virtual void PostRegisterAllComponents() override;
	virtual void OnConstruction( const FTransform& Transform ) override;
	virtual void BeginPlay() override;

private:
	void ConfigurePortalVFX();

	UFUNCTION()
	void HandleBeginOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult );

	UFUNCTION()
	void HandleEndOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex );

	UPROPERTY( VisibleAnywhere )
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY( VisibleAnywhere )
	TObjectPtr<USphereComponent> InteractionVolume;

	UPROPERTY( VisibleAnywhere )
	TObjectPtr<UTextRenderComponent> PromptText;

	UPROPERTY( VisibleAnywhere )
	TArray<TObjectPtr<UStaticMeshComponent>> PortalFrame;
};
