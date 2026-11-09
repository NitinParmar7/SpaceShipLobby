// Copyright Epic Games, Inc. All Rights Reserved.

#include "SLobbyMenuWidget.h"

#include "SpaceShipGameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void SLobbyMenuWidget::Construct( const FArguments& Arguments )
{
	GameInstance = Arguments._GameInstance;
	const USpaceShipGameInstance* Instance = GameInstance.Get();

	ChildSlot
	[
		SNew( SOverlay )
		+ SOverlay::Slot()
		.HAlign( HAlign_Center )
		.VAlign( VAlign_Center )
		[
			SNew( SBorder )
			.Padding( 28.0f )
			[
				SNew( SBox )
				.WidthOverride( 360.0f )
				[
					SNew( SVerticalBox )
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding( 0.0f, 0.0f, 0.0f, 18.0f )
					[
						SNew( STextBlock )
						.Text( FText::FromString( TEXT( "Spaceship Lobby" ) ) )
						.Font( FCoreStyle::GetDefaultFontStyle( "Bold", 28 ) )
						.Justification( ETextJustify::Center )
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding( 0.0f, 0.0f, 0.0f, 18.0f )
					[
						SNew( STextBlock )
						.Text( TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateSP( this, &SLobbyMenuWidget::GetStatusText ) ) )
						.AutoWrapText( true )
						.Justification( ETextJustify::Center )
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding( 0.0f, 0.0f, 0.0f, 8.0f )
					[
						SAssignNew( PlayerNameTextBox, SEditableTextBox )
						.HintText( FText::FromString( TEXT( "Player name" ) ) )
						.Text( FText::FromString( Instance ? Instance->GetPlayerDisplayName() : TEXT( "Player" ) ) )
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding( 0.0f, 0.0f, 0.0f, 8.0f )
					[
						SNew( SButton )
						.HAlign( HAlign_Center )
						.OnClicked( this, &SLobbyMenuWidget::HandleHostClicked )
						[
							SNew( STextBlock ).Text( FText::FromString( TEXT( "Host Lobby" ) ) )
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding( 0.0f, 0.0f, 0.0f, 8.0f )
					[
						SNew( SButton )
						.HAlign( HAlign_Center )
						.OnClicked( this, &SLobbyMenuWidget::HandleFindClicked )
						[
							SNew( STextBlock ).Text( FText::FromString( TEXT( "Find & Join" ) ) )
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew( SButton )
						.HAlign( HAlign_Center )
						.OnClicked( this, &SLobbyMenuWidget::HandleQuitClicked )
						[
							SNew( STextBlock ).Text( FText::FromString( TEXT( "Quit" ) ) )
						]
					]
				]
			]
		]
	];
}

FReply SLobbyMenuWidget::HandleHostClicked()
{
	CommitProfileFields();
	if( USpaceShipGameInstance* Instance = GameInstance.Get() )
	{
		Instance->HostLobby();
	}
	return FReply::Handled();
}

FReply SLobbyMenuWidget::HandleFindClicked()
{
	CommitProfileFields();
	if( USpaceShipGameInstance* Instance = GameInstance.Get() )
	{
		Instance->FindAndJoinLobby();
	}
	return FReply::Handled();
}

void SLobbyMenuWidget::CommitProfileFields()
{
	if( USpaceShipGameInstance* Instance = GameInstance.Get() )
	{
		const FString PlayerName = PlayerNameTextBox.IsValid() ? PlayerNameTextBox->GetText().ToString() : Instance->GetPlayerDisplayName();
		Instance->SetPlayerDisplayName( PlayerName );
	}
}

FReply SLobbyMenuWidget::HandleQuitClicked()
{
	if( USpaceShipGameInstance* Instance = GameInstance.Get() )
	{
		Instance->QuitGame();
	}
	return FReply::Handled();
}

FText SLobbyMenuWidget::GetStatusText() const
{
	if( const USpaceShipGameInstance* Instance = GameInstance.Get() )
	{
		return Instance->GetLobbyStatus();
	}
	return FText::GetEmpty();
}