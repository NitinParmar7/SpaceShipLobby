// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class USpaceShipGameInstance;
class SEditableTextBox;

class SLobbyMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SLobbyMenuWidget )
		{
		}
		SLATE_ARGUMENT( TWeakObjectPtr<USpaceShipGameInstance>, GameInstance )
	SLATE_END_ARGS()

	void Construct( const FArguments& Arguments );

private:
	FReply HandleHostClicked();
	FReply HandleFindClicked();
	FReply HandleQuitClicked();
	FText GetStatusText() const;
	void CommitProfileFields();

	TWeakObjectPtr<USpaceShipGameInstance> GameInstance;
	TSharedPtr<SEditableTextBox> PlayerNameTextBox;
};