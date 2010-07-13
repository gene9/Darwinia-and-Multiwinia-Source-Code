
#ifndef _included_mainmenus_h
#define _included_mainmenus_h

#include "lib/language_table.h"

#include "interface/darwinia_window.h"

#include "game_menu.h"
#include "UI/GameMenuWindow.h"
#include "app.h"
#include "renderer.h"
#include "sound/soundsystem.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"

class GameOptionsWindow : public DarwiniaWindow
{
public:
    bool    m_renderRightBox;
    bool    m_renderWholeScreen;

    UnicodeString   m_errorMessage;
    bool            m_showingErrorDialogue;
    bool            m_dialogueSuccessMessage;

public:
    GameOptionsWindow( char *_name );
    void Create();

    void Update();
    void Render( bool _hasFocus );
    GameMenuInputField *CreateMenuControl( 
	        char const *name, int dataType, NetworkValue *value, int y, float change, 
	        DarwiniaButton *callback, int x, int w, float fontSize );

    void CreateErrorDialogue( UnicodeString _error, bool _success = false );
    void RenderErrorDialogue();
};

class MainMenuWindow : public GameOptionsWindow
{
public:
    MainMenuWindow();

    void Create();
	void Render( bool _hasFocus );
};


class OptionsMenuWindow : public GameOptionsWindow
{
public:
    OptionsMenuWindow();

    void Create();
};


class LocationWindow : public GameOptionsWindow
{
public:
    LocationWindow();
	~LocationWindow();

    void Create();
};


class ResetLocationWindow : public GameOptionsWindow
{
public:
    ResetLocationWindow();

    void Create();
    void Render( bool _hasFocus );
};


class AboutDarwiniaWindow : public DarwiniaWindow
{
public:
    AboutDarwiniaWindow();

    void Create();
    void Render( bool _hasFocus );
};

class ConfirmExitWindow : public GameOptionsWindow
{
public:
    ConfirmExitWindow();
    void Create();
};

class ConfirmResetWindow : public GameOptionsWindow
{
public:
    ConfirmResetWindow();
    void Create();
};


class MenuCloseButton   :   public GameMenuButton
{
public:
    MenuCloseButton( char *_name )
    :   GameMenuButton( _name )
    {
    }

    void MouseUp()
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuCancel", SoundSourceBlueprint::TypeMultiwiniaInterface );
        g_app->m_renderer->InitialiseMenuTransition(1.0f, -1);
		AppDebugOut("Removing window %s\n", m_parent->m_name);
        EclRemoveWindow( m_parent->m_name );
        g_app->m_doMenuTransition = true;
    }

	void Render( int realX, int realY, bool highlighted, bool clicked )
	{
		GameMenuButton::Render( realX, realY, highlighted, clicked );

		if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
		{
			float iconSize = m_fontSize;
			float iconGap = 10.0f;
			float iconAlpha = 1.0f;
			int xPos = realX + g_gameFont.GetTextWidth( m_caption.Length(), m_fontSize ) + 60;
			int yPos = realY + (m_h / 2.0f);

			if( m_centered )
			{
				xPos += m_w / 2.0f;
			}

			Vector2 iconCentre = Vector2(xPos, yPos);            

			// Render the icon

			glEnable        ( GL_TEXTURE_2D );
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_b.bmp" ) );
			glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	        
			glColor4f   ( 1.0f, 1.0f, 1.0f, iconAlpha );

			float x = iconSize/2;

			glBegin( GL_QUADS );
				glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - x, iconCentre.y - iconSize/2 );
				glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + x, iconCentre.y - iconSize/2 );
				glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + x, iconCentre.y + iconSize/2 );
				glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - x, iconCentre.y + iconSize/2 );
			glEnd();

			glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glDisable       ( GL_TEXTURE_2D );
		}
	}
};

class MenuGameExitButton : public GameMenuButton
{
public:
    MenuGameExitButton()
    : GameMenuButton( LANGUAGEPHRASE("dialog_leavedarwinia") )
    {
    }

    void MouseUp()
    {
        g_app->m_atMainMenu = true;
        g_app->m_renderer->StartFadeOut();
    }
};

class HelpAndOptionsButton : public GameMenuButton
{
public:
    HelpAndOptionsButton();
    void MouseUp();
};

class AchievementsButton : public GameMenuButton
{
public:
    AchievementsButton()
    : GameMenuButton( "multiwinia_menu_achievements" )
    {
    }
};

class LeaderBoardButton : public GameMenuButton
{
public:
	LeaderBoardButton(char *_iconName);
	void MouseUp();
};

class ConfirmResetButton : public GameMenuButton
{
public:
    ConfirmResetButton();

    void MouseUp();
};

#endif