#include "SpaceShipHUD.h"
#include "SpaceShipCharacter.h"
#include "SpaceShipPortal.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ASpaceShipHUD::DrawHUD()
{
	Super::DrawHUD();
	if( Canvas && GetNetMode() == NM_ListenServer )
	{
		DrawRect( FLinearColor( 0.01f, 0.02f, 0.03f, 0.85f ), 20.0f, 20.0f, 170.0f, 42.0f );
		DrawText( TEXT( "Hosting" ), FLinearColor( 0.3f, 1.0f, 0.5f ), 34.0f, 29.0f, nullptr, 1.5f );
	}
	const ASpaceShipCharacter* Character = Cast<ASpaceShipCharacter>( GetOwningPawn() );
	if( Canvas && Character )
	{
		if( const ASpaceShipPortal* Portal = Character->FindUsablePortal() )
		{
			const FString Message = Portal->PromptMessage.IsEmpty()
				? TEXT( "Press E to use portal" ) : Portal->PromptMessage.ToString();
			UFont* Font = GEngine->GetMediumFont();
			float TextWidth = 0.0f;
			float TextHeight = 0.0f;
			GetTextSize( Message, TextWidth, TextHeight, Font );
			const float X = (Canvas->SizeX - TextWidth) * 0.5f;
			const float Y = Canvas->SizeY * 0.82f;
			DrawRect( FLinearColor( 0.01f, 0.02f, 0.03f, 0.85f ), X - 20.0f, Y - 10.0f, TextWidth + 40.0f, TextHeight + 20.0f );
			DrawText( Message, FLinearColor::White, X, Y, Font );
		}
	}
}
