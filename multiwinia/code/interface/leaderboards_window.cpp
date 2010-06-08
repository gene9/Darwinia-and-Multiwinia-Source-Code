#include "lib/universal_include.h"

#include "lib/preferences.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "lib/input/input.h"
#include "lib/math/math_utils.h"

#include "interface/leaderboards_window.h"
#include "UI/GameMenuWindow.h"

class LeaderboardSmallFontRenderingButton : public GameMenuButton
{
public:
	LeaderboardSmallFontRenderingButton(const UnicodeString& _caption)
	: GameMenuButton(_caption)
	{
	}
	void Render( int realX, int realY, bool highlighted, bool clicked )
    {
		DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
    
		float realXf = (float)realX;
		double timeNow = GetHighResTime();
		int numButtons = parent->m_buttonOrder.Size();
		double showTime = m_birthTime + 0.6f * (float)realY / (float)g_app->m_renderer->ScreenH();
		double flashShowTime = m_birthTime + (5.0f/numButtons) * (float)realY / (float)g_app->m_renderer->ScreenH();

		if( m_birthTime > 0.0f && timeNow < flashShowTime )
		{
			realXf -= ( showTime - timeNow ) * g_app->m_renderer->ScreenW() * 0.1f;            

			float boxAlpha = (flashShowTime-timeNow);
			Clamp( boxAlpha, 0.0f, 0.9f );

			glColor4f( 1.0f, 1.0f, 1.0f, boxAlpha );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE );
			glBegin( GL_QUADS);
				glVertex2f( realXf, realY );
				glVertex2f( realXf + m_w, realY );
				glVertex2f( realXf + m_w, realY + m_h );
				glVertex2f( realXf, realY + m_h );
			glEnd();
		}


		if( !m_mouseHighlightMode )
		{
			highlighted = false;
		}

		if( parent->m_buttonOrder[parent->m_currentButton] == this )
		{
			highlighted = true;
		}

		if( highlighted )
		{
			//RenderHighlightBeam( realX, realY+m_h/2.0f, m_w, m_h );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
			glBegin( GL_QUADS);
				glVertex2f( realXf, realY );
				glVertex2f( realXf + m_w, realY );
				glVertex2f( realXf + m_w, realY + m_h );
				glVertex2f( realXf, realY + m_h );
			glEnd();
		}

		UpdateButtonHighlight();

	    
		glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
		g_editorFont.SetRenderOutline(true);

		float texY = realY + m_h/2 - m_fontSize/2 + 7.0f;
		TextRenderer *font = &g_editorFont;
		//if( m_useEditorFont ) font = &g_editorFont;

		if( m_centered )
		{
			font->DrawText2DCentre( realXf + m_w/2, texY, m_fontSize, m_caption );
		}
		else
		{
			font->DrawText2D( realXf + m_w*0.05f, texY, m_fontSize, m_caption );
		}

		glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );

		if( m_useEditorFont ) glColor4f( 1.0f, 1.0f, 0.3f, 1.0f );


		if( highlighted  )
		{
			glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
		}

		if( m_inactive ) glColor4f( m_ir, m_ig, m_ib, m_ia );
	    
		g_editorFont.SetRenderOutline(false);

		if( m_centered )
		{
			font->DrawText2DCentre( realXf + m_w/2, texY, m_fontSize, m_caption );
		}
		else
		{
			font->DrawText2D( realXf + m_w*0.05f, texY, m_fontSize, m_caption );
		}

		if( m_birthTime > 0.0f )
		{
			if( timeNow >= showTime )
			{
				m_birthTime = -timeNow;
			}
		}
		else if( m_birthTime > -999 )
		{
			double fxLen = 0.1f;
			double timePassed = timeNow - (m_birthTime * -1);

			glColor4f( 1.0f, 1.0f, 1.0f, max(0.0, 1.0-timePassed/fxLen) );
			if( m_centered )
			{
				font->DrawText2DCentre( realXf + m_w/2, texY, m_fontSize, m_caption );
			}
			else
			{
				font->DrawText2D( realXf + m_w*0.05f, texY, m_fontSize, m_caption );

				glColor4f( 1.0f, 1.0f, 1.0f, max(0.0, 0.5-timePassed/fxLen) );
				font->DrawText2D( realXf + m_w*0.05f, texY, m_fontSize*1.3f, m_caption );
			}    
	        
			if( timePassed >= fxLen ) m_birthTime = -1000;
		}


		if( highlighted && s_previousHighlightButton != this )
		{
			g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuRollOver", SoundSourceBlueprint::TypeMultiwiniaInterface );
			s_previousHighlightButton = this;
		}
	}
};

class LeaderBoardGamertagButton : public LeaderboardSmallFontRenderingButton
{
public:
	bool			m_usingSmallFont;
	int				m_thisButtonPosNum;
	UnicodeString	m_extraInfoString[3];

	LeaderBoardGamertagButton()
	:	LeaderboardSmallFontRenderingButton(UnicodeString()),
	m_thisButtonPosNum(0),
	m_usingSmallFont(false)
	{
		m_centered = true;
		m_ir = m_ig = m_ib = 0.8f;
	}

	void MouseUp()
	{
        GameMenuButton::MouseUp();
	}

	void RenderBackground( int realX, int realY, bool &highlighted, bool &clicked )
	{
		GameMenuWindow *main = (GameMenuWindow *) m_parent;

		if( highlighted && g_inputManager->getInputMode() != INPUT_MODE_KEYBOARD )
        {
            // ignore any mouse inputs if we are using the gamepad
            highlighted = false;
        }

        UpdateButtonHighlight();

        if( main->m_buttonOrder.ValidIndex(main->m_currentButton) &&
            main->m_buttonOrder[main->m_currentButton] == this )
        {
            highlighted = true;
        }

        if( highlighted )
        {
            glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
        }
        else
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
        }

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glBegin( GL_QUADS);
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();
        
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
    {
		int parentCursorPos = ((LeaderboardWindow*)m_parent)->m_cursorPosition;
		bool thisButton = false;

		if( parentCursorPos == m_thisButtonPosNum )
		{
			UnicodeString caption(m_caption);
			bool oldInactive = m_inactive;
			int oldWidth = m_w;
			m_w /= 3;

			for( int i = 0; i < 3; i++ )
			{
				m_caption = m_extraInfoString[i];
				//m_centered = false;
				m_inactive = true;

				int x = realX + (i*m_w);

				if( m_usingSmallFont )
				{
					LeaderboardSmallFontRenderingButton::Render( x, realY+m_h, false, false );
				}
				else
				{
					GameMenuButton::Render( realX, realY+m_h, false, false );
				}
			}

			m_caption = caption;
			m_centered = true;
			m_inactive = oldInactive;
			m_w = oldWidth;

			thisButton = true;
		}
		else if( m_thisButtonPosNum > parentCursorPos )
		{
			realY += m_h;
		}

		if( !thisButton )
		{
			RenderBackground(realX, realY, highlighted, clicked);
		}

		if( m_usingSmallFont )
		{
			LeaderboardSmallFontRenderingButton::Render( realX, realY, false, false );
		}
		else
		{
			GameMenuButton::Render( realX, realY, false, false );
		}
	}
};

class LeaderBoardTypeButton : public GameMenuButton
{
public:
	enum
	{
		LeaderboardTypeNone,
		LeaderboardTypeSP,
		LeaderboardTypeCPU,
		LeaderboardTypeMPRanked,
		LeaderboardTypeMPStandard
	};
	int	m_leaderboardType;
public:
	LeaderBoardTypeButton()
	:	GameMenuButton(UnicodeString()),
	m_leaderboardType(LeaderboardTypeNone)
	{
	}

	void MouseUp()
	{
		if( m_leaderboardType != LeaderboardTypeNone )
		{
			EclRegisterWindow( new LeaderboardWindow(m_leaderboardType), m_parent );
		}

		GameMenuWindow* gmw = (GameMenuWindow*) EclGetWindow("multiwinia_mainmenu_title");
		if( gmw )
		{
			gmw->SetupNewPage( GameMenuWindow::PageLeaderBoard );
		}
	}
};

class LeaderBoardMessageButton : public LeaderboardSmallFontRenderingButton
{
public:
	int		m_thisButtonPosNum;
	bool	m_moveable;
	bool	m_usingSmallFont;

	LeaderBoardMessageButton()
	:	LeaderboardSmallFontRenderingButton(UnicodeString()),
	m_moveable(false),
	m_thisButtonPosNum(-1),
	m_usingSmallFont(false)
	{
		m_centered = true;
		m_inactive = true;
		m_ir = m_ig = m_ib = 0.8f;
	}
	void MouseUp()
	{
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
    {
		int parentCursorPos = ((LeaderboardWindow*)m_parent)->m_cursorPosition;

		if( m_moveable )
		{
			if( m_thisButtonPosNum > parentCursorPos )
			{
				realY += m_h;
			}
		}

		if( m_usingSmallFont )
		{
			LeaderboardSmallFontRenderingButton::Render( realX, realY, false, false );
		}
		else
		{
			GameMenuButton::Render( realX, realY, false, false );
		}
	}
};

class LeaderBoardWeeklyButton : public LeaderBoardMessageButton
{
private:
	bool m_weekly, m_justChangedWeekly;
public:
	LeaderBoardWeeklyButton()
	:	LeaderBoardMessageButton(),
	m_weekly(false),
	m_justChangedWeekly(false)
	{
		m_centered = false;
	}

	bool GetIsWeekly()
	{
		return m_weekly;
	}

	void MouseUp()
	{
		// Do nothing!
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
    {
		if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
        {
			if( g_inputManager->controlEvent( ControlLeaderboardWeekly ) && !m_justChangedWeekly )
            {
				m_justChangedWeekly = true;
				m_weekly = !m_weekly;
				//Changing m_weekly

				char captionID[64];
				sprintf(captionID,"multiwinia_leaderboard_weekly_%s", m_weekly ? "yes" : "no");
				SetCaption(LANGUAGEPHRASE(captionID));
			}
			else
			{
				m_justChangedWeekly = false;
			}
		}

		LeaderBoardMessageButton::Render( realX, realY, false, false );

		float iconSize = m_fontSize * 1.5f;
        float iconAlpha = 1.0f;
        float shadowOffset = 0;
        float shadowSize = iconSize; 
		float shadowX = shadowSize/2;
		int xPos = realX + m_w - (iconSize * 2);
		int yPos = realY + m_fontSize / 2.0f;// + (iconSize/2);

		Vector2 iconCentre = Vector2(xPos, yPos);

		// Render the shadows
		glEnable        ( GL_TEXTURE_2D );
		glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_abxy_shadow.bmp" ) );
	    
		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
		glDepthMask     ( false );
		glColor4f       ( iconAlpha, iconAlpha, iconAlpha, 0.0f );         
		
		glBegin( GL_QUADS );
			glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
			glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
			glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
			glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
		glEnd();	

		// Render the icons
		glEnable        ( GL_TEXTURE_2D );
		glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_y.bmp" ) );
		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
	    
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
};

class LeaderBoardListButton : public GameMenuButton
{
private:
	LList<UnicodeString*> m_captions;
	int m_captionIndex;
	int m_direction;
	int m_moveTextOffset;
	float m_moveTextMoveSpeed;
	float m_moveTextAlpha;
	float m_moveTextAlphaChange;
	float m_moveTextMoveChange;
	bool m_movingText;
	double m_lastUpdateTime;
	bool m_needToChangeText;
	int m_dirToMove;
public:
	int m_thisButtonNumber;
	ControlType	m_inputEnumUp;
	ControlType	m_inputEnumDown;

	LeaderBoardListButton()
	:	GameMenuButton(UnicodeString()),
	m_captions(LList<UnicodeString*>()),
	m_captionIndex(0),
	m_direction(0),
	m_moveTextOffset(0),
	m_moveTextAlpha(1.0f),
	m_moveTextMoveSpeed(0.025f),
	m_movingText(false),
	m_moveTextAlphaChange(-0.1f),
	m_moveTextMoveChange(-1.0f),
	m_lastUpdateTime(-10),
	m_needToChangeText(false),
	m_dirToMove(0)
	{
		m_centered = true;
		m_ir = m_ig = m_ib = 0.8f;
		m_inputEnumUp = ControlXBoxMenuCycleUp;
		m_inputEnumDown = ControlXBoxMenuCycleDown;
	}

	void AddCaptionToList(UnicodeString* _string)
	{
		m_captions.PutDataAtEnd(_string);
	}

	void ToggleCaptions(int direction)
	{
		m_direction = direction < 0 ? -1 : 1;
		m_captionIndex+=m_direction;
		if (m_captionIndex >= m_captions.Size())
		{
			m_captionIndex = 0;
		}
		else if (m_captionIndex < 0)
		{
			m_captionIndex = m_captions.Size()-1;
		}

		SetCaption(*(m_captions.GetData(m_captionIndex)));
		LeaderBoardMessageButton* leaderboardMessageButton = (LeaderBoardMessageButton*) m_parent->GetButton("multiwinia_leaderboard_score");
		if( leaderboardMessageButton )
		{
			static int originalFontSize = leaderboardMessageButton->m_fontSize;
			if( wcscmp(m_caption.m_unicodestring, LANGUAGEPHRASE("multiwinia_leaderboards_tslb").m_unicodestring) == 0  )
			{
				leaderboardMessageButton->SetCaption(LANGUAGEPHRASE("multiwinia_leaderboards_tsr"));
				// LEANDER : This looks shit, but stops overlapping code and stuff. SHIT.
				leaderboardMessageButton->m_fontSize = originalFontSize * 0.5f;
			}
			else 
			{
				leaderboardMessageButton->SetCaption(LANGUAGEPHRASE("multiwinia_leaderboard_score"));
				leaderboardMessageButton->m_fontSize = originalFontSize;
			}
		}

		// Load next board
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
	{
		// update text movement
		if( m_movingText && GetHighResTime() > m_lastUpdateTime + m_moveTextMoveSpeed )
		{
			m_lastUpdateTime = GetHighResTime();
			// update the alpha and pos
			m_moveTextAlpha += m_moveTextAlphaChange;
			m_moveTextOffset += (m_moveTextMoveChange*m_dirToMove*-1);

			// if the alpha has gone below 0, change the caption and the alpha
			if( m_moveTextAlpha <= 0.0f )
			{
				m_needToChangeText = true;
				m_moveTextAlpha = 0.0f;
			}
			// otherwise if it's above 1.0f, stop animating
			else if( m_moveTextAlpha >= 1.0f )
			{
				m_moveTextAlpha = 1.0f;
				m_movingText = false;
				m_moveTextAlphaChange = -m_moveTextAlphaChange;
				m_moveTextOffset = 0.0f;
				m_dirToMove = 0;
			}

			// If we need to change the caption, do so, and switch the 
			if( m_needToChangeText )
			{
				ToggleCaptions(m_dirToMove);
				m_needToChangeText = false;
				m_moveTextAlphaChange = -m_moveTextAlphaChange;
				m_moveTextOffset = -m_moveTextOffset;
			}
		}

		if( m_inputEnumDown == ControlXBoxMenuCycleDown &&
			m_inputEnumUp == ControlXBoxMenuCycleUp )
		{
			static int originalFontSize = m_fontSize;
			m_fontSize = (originalFontSize*m_moveTextAlpha)+0.01f;
			//
			// Fill  box

			RenderHighlightBeam( realX, realY + m_h/2, m_w, m_h );

			//
			// Caption

			UnicodeString caption(m_caption);
			caption.StrUpr();

			float fontSize = m_fontSize * 1.5f;

			float texY = realY + m_h/2.0f - fontSize/2.0f + 7.0f;

			g_titleFont.SetRenderOutline(true);
			glColor4f( 0.75f, 1.0f, 0.45f, m_moveTextAlpha/*1.0f*/ );
			int modifier = m_moveTextOffset < 0 ? -1 : 1;
			g_titleFont.DrawText2DCentre( realX + m_w*0.5f+(m_moveTextOffset*m_moveTextOffset*m_moveTextOffset*0.5f), texY, fontSize, caption );

			g_titleFont.SetRenderOutline(false);
			g_titleFont.SetRenderShadow(true);
			glColor4f( m_moveTextAlpha, m_moveTextAlpha*0.9f, m_moveTextAlpha*0.9f, 0.0f );
			g_titleFont.DrawText2DCentre( realX + m_w*0.5f+(m_moveTextOffset*m_moveTextOffset*m_moveTextOffset*0.5f), texY, fontSize, caption );

			g_titleFont.SetRenderShadow(false);
		}
		else
		{
			glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
			glBegin( GL_QUADS );
				glVertex2f( realX, realY );
				glVertex2f( realX + m_w, realY );
				glVertex2f( realX + m_w, realY + m_h );
				glVertex2f( realX, realY + m_h );
			glEnd();

			float texY = realY + m_h - m_fontSize + 5;

			for( int i = 0; i < 3; ++i )
			{
				if( i == 0 )
				{
					g_titleFont.SetRenderOutline(true);
					glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
				}
				else
				{
					g_titleFont.SetRenderOutline(false);
					glColor4f( 0.6f, 0.6f, 0.6f, m_moveTextAlpha/* 1.0f */);
				}

				if( !(i == 0 && m_movingText) )
				{
					g_titleFont.DrawText2DCentre( realX + m_w*0.5f + m_moveTextOffset, texY, m_fontSize, m_caption );
				}
			}
		}

        g_titleFont.SetRenderOutline(false);
        g_titleFont.SetRenderShadow(false);

		bool up = g_inputManager->controlEvent( m_inputEnumUp ) && !m_movingText;
		bool down = g_inputManager->controlEvent( m_inputEnumDown ) && !m_movingText;

		if( up )
		{
			m_dirToMove = -1;
			m_movingText = true;
			m_lastUpdateTime = -10;
		}
		else if( down )
		{
			m_dirToMove = 1;
			m_movingText = true;
			m_lastUpdateTime = -10;
		}

        float iconSize = m_fontSize;
        float iconAlpha = 1.0f;
        float shadowOffset = 0;
        float shadowSize = iconSize; 

		int xPos = realX + iconSize;
		int yPos = realY + m_h/2;

        Vector2 iconCentre = Vector2(xPos+iconSize, yPos);

        float shadowX = shadowSize/2;

        // Render the shadows
        glEnable        ( GL_TEXTURE_2D );
		if( m_inputEnumUp == ControlXBoxMenuCycleUp )
		{
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_lb_shadow.bmp" ) );
		}
		else
		{
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_trigger_shadow.bmp" ) );
		}           
        
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glDepthMask     ( false );
        glColor4f       ( iconAlpha, iconAlpha, iconAlpha, 0.0f );         
    	
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
        glEnd();	

		iconCentre = Vector2(realX+m_w-(iconSize*2), yPos);

		if( m_inputEnumUp == ControlXBoxMenuCycleUp )
		{
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_rb_shadow.bmp" ) );
		}
		else
		{
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_trigger_shadow.bmp" ) );
		} 
		
		glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
        glEnd();

        // Render the icons
		iconCentre = Vector2(xPos+iconSize, yPos);

        glEnable        ( GL_TEXTURE_2D );
		if( m_inputEnumUp == ControlXBoxMenuCycleUp )
		{
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_lb.bmp" ) );
		}
		else
		{
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_lt.bmp" ) );
		}

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        
        glColor4f   ( 1.0f, 1.0f, 1.0f, iconAlpha );

        float x = iconSize/2;

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + x, iconCentre.y + iconSize/2 );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - x, iconCentre.y + iconSize/2 );
        glEnd();

		iconCentre = Vector2(realX+m_w-(iconSize*2), yPos);
		if( m_inputEnumUp == ControlXBoxMenuCycleUp )
		{
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_rb.bmp" ) );
		}
		else
		{
			glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_rt.bmp" ) );
		}
		glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + x, iconCentre.y + iconSize/2 );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - x, iconCentre.y + iconSize/2 );
        glEnd();


		// Arrows
		glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/virii.bmp" ) );//"icons/arrow.bmp" ) );
        
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glDepthMask     ( false );
        glColor4f       ( iconAlpha, iconAlpha, iconAlpha, 0.0f );  
		iconCentre = Vector2(xPos, yPos);
    	
        glBegin( GL_QUADS );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
        glEnd();	

		iconCentre = Vector2(realX+m_w-iconSize, yPos);
		
		glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
        glEnd();

		iconCentre = Vector2(xPos, yPos);

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        
        glColor4f   ( 1.0f, 1.0f, 1.0f, iconAlpha );

        glBegin( GL_QUADS );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x - x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x + x, iconCentre.y + iconSize/2 );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - x, iconCentre.y + iconSize/2 );
        glEnd();

		iconCentre = Vector2(realX+m_w-iconSize, yPos);

		glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x + x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + x, iconCentre.y + iconSize/2 );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x - x, iconCentre.y + iconSize/2 );
        glEnd();

		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_TEXTURE_2D );
	}
};

class LeaderBoardMiniListButton : public LeaderBoardListButton
{
public:
	LeaderBoardMiniListButton() : LeaderBoardListButton()
	{
		m_centered = false;
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
    {
		GameMenuWindow* gmw = (GameMenuWindow*)m_parent;
		GameMenuButton::Render( realX, realY, highlighted, clicked );

		bool up = g_inputManager->controlEvent( m_inputEnumUp );
		bool down = g_inputManager->controlEvent( m_inputEnumDown );

		if( up )
		{
			ToggleCaptions(-1);
		}
		else if( down )
		{
			ToggleCaptions(1);
		}

        float iconSize = m_fontSize * 1.5f;
		float iconAlpha = 1.0f;
		float shadowOffset = 0;
		float shadowSize = iconSize; 
		float shadowX = shadowSize/2;
		int xPos = realX - (iconSize/2);
		int yPos = realY + m_fontSize / 2.0f;// + (iconSize/2);

		Vector2 iconCentre = Vector2(xPos, yPos);

		// Render the shadows
		glEnable        ( GL_TEXTURE_2D );
		glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_abxy_shadow.bmp" ) );
	    
		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
		glDepthMask     ( false );
		glColor4f       ( iconAlpha, iconAlpha, iconAlpha, 0.0f );         
		
		glBegin( GL_QUADS );
			glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
			glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
			glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
			glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
		glEnd();	

		// Render the icons
		glEnable        ( GL_TEXTURE_2D );
		glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_x.bmp" ) );
		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
	    
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
};


LeaderboardLoader::LeaderboardLoader()
{
}

LeaderboardLoader::~LeaderboardLoader()
{
}

bool LeaderboardLoader::IsNewSearchNeeded()
{
	bool newSearch = false;
	return newSearch;
}

int LeaderboardLoader::GetSearchOffset()
{
	return m_searchIndexOffset;
}

bool LeaderboardLoader::ScrollUp()
{
	return false;
}

bool LeaderboardLoader::ScrollDown()
{
	return false;
}

void LeaderboardLoader::DoLeaderboardSearch()
{
}

bool LeaderboardLoader::GetRowsToShow( char* _rowsArray, int _numRows )
{
	// WhateverObjectWeUseToDescribeLeaderboardRows* rows = (WhateverObjectWeUseToDescribeLeaderboardRows*)_rowsArray;
	// for( int i = 0; i < _numRows; i++ ) { rows[i] = new WhateverObjectWeUseToDescribeLeaderboardRows(); ... }
	return true;
}

LeaderboardWindow::LeaderboardWindow(int _type)
:   GameOptionsWindow( "leaderboards" ),
	m_leaderboardType(_type),
    m_leaderboardScrollOffset(0),
	m_cursorPosition(1),
	m_numRowsShowing(0),
	m_buttonYPos(0.0f)
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);

	m_renderWholeScreen = true;
}

int LeaderboardWindow::GetLeaderboardType()
{
	return m_leaderboardType;
}

void LeaderboardWindow::SetUpBoardButton(LeaderBoardListButton* _button)
{
	UnicodeString* temp = NULL;
	switch( m_leaderboardType )
	{
		case LeaderBoardTypeButton::LeaderboardTypeSP :
			temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_menu_alllevels"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_garden"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_containment"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_mine"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_generator"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_yard"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_escort"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_pattern_buffer"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_biosphere"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_receiver"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_temple"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("location_prologue"));
			_button->AddCaptionToList(temp);
			break;

		default :
			temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_menu_allgames"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_gametype_domination"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_gametype_kingofthehill"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_gametype_rocketriot"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_gametype_capturethestatue"));
			_button->AddCaptionToList(temp);
			temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_gametype_blitzkrieg"));
			_button->AddCaptionToList(temp);
			break;
	}
}

void LeaderboardWindow::Create()
{
	float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
	float buttonW, buttonH, buttonGap;
	float fontLarge, fontMedium, fontSmall;
	float titleBarX, titleBarY, titleBarW, titleBarH;
	UnicodeString *temp;

    GetPosition_LeftBox( leftX, leftY, leftW, leftH );
    GetPosition_RightBox( rightX, rightY, rightW, rightH );
	GetButtonSizes(buttonW, buttonH, buttonGap);
	GetFontSizes(fontLarge, fontMedium, fontSmall);
	GetPosition_TitleBar(titleBarX, titleBarY, titleBarW, titleBarH);
	float yPos = leftY+10;
	float buttonX = leftX + (leftW-buttonW)/2.0f;

    /*GameMenuTitleButton *title = new GameMenuTitleButton();
	title->SetShortProperties( "leaderboardpagetitle", leftX+10, yPos, titleBarW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_leaderboards" ) );
    title->m_fontSize = fontMedium;
    RegisterButton( title );
    yPos += buttonH*1.5f + buttonGap;
	*/

	LeaderBoardListButton* allboards = new LeaderBoardListButton();
	allboards->SetShortProperties( "multiwinia_menu_allboards", leftX+10, yPos, titleBarW-20, buttonH*1.5f, LANGUAGEPHRASE(m_leaderboardType == LeaderBoardTypeButton::LeaderboardTypeSP ? "multiwinia_menu_alllevels" : "multiwinia_menu_allgames") );
	SetUpBoardButton( allboards );
	RegisterButton( allboards );
    allboards->m_fontSize = fontMedium;
	allboards->m_thisButtonNumber = m_buttonOrder.FindData(GetButton("multiwinia_menu_allboards"));
	//yPos+= (buttonH) + buttonGap;
	yPos += buttonH*1.5f + buttonGap;

	LeaderBoardListButton* allviews = new LeaderBoardListButton();
	allviews->m_inputEnumUp = ControlLeaderboardLTrigger;
	allviews->m_inputEnumDown = ControlLeaderboardRTrigger;
	allviews->SetShortProperties( "multiwinia_menu_allviews", leftX+10, yPos, titleBarW-20, buttonH * 0.7f, LANGUAGEPHRASE("multiwinia_menu_friends") );
	temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_menu_friends"));
	allviews->AddCaptionToList(temp);
	temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_menu_myscore"));
	allviews->AddCaptionToList(temp);
	temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_menu_overall"));
	allviews->AddCaptionToList(temp);
	RegisterButton( allviews );
    allviews->m_fontSize = fontSmall;
	allviews->m_thisButtonNumber = m_buttonOrder.FindData(GetButton("multiwinia_menu_allviews"));
	m_buttonYPos = yPos;

	/*LeaderBoardMessageButton* leaderboardtitle = new LeaderBoardMessageButton();
	leaderboardtitle->SetShortProperties("multiwinia_leaderboard_rank", leftX+xPos, leftY +7, leftW/4.0f, buttonH, LANGUAGEPHRASE("multiwinia_leaderboard_rank"));
	leaderboardtitle->m_fontSize = fontMedium;
	RegisterButton( leaderboardtitle );*/

	LeaderBoardMessageButton* loading = new LeaderBoardMessageButton();
	loading->SetShortProperties("multiwinia_leaderboard_loading", leftX, leftY+((leftH-buttonH)/2), titleBarW, buttonH, LANGUAGEPHRASE("multiwinia_leaderboard_loading"));
	loading->m_fontSize = fontMedium;
	loading->m_centered = true;
	RegisterButton( loading );

	loading = new LeaderBoardMessageButton();
	UnicodeString showingMessage(LANGUAGEPHRASE("multiwinia_leaderboard_showing"));
	showingMessage.ReplaceStringFlag(L'R', UnicodeString("0"));
	showingMessage.ReplaceStringFlag(L'S', UnicodeString("0"));
	showingMessage.ReplaceStringFlag(L'T', UnicodeString("0"));
	loading->SetShortProperties("multiwinia_leaderboard_showing", leftX, leftY+leftH-buttonGap-buttonH, titleBarW, buttonH, showingMessage);
	loading->m_fontSize = fontSmall;
	loading->m_centered = true;
	RegisterButton( loading );

	m_currentButton = -1;
	m_leaderboardLoader.DoLeaderboardSearch();

	yPos = leftY + leftH - buttonH * 2;

	MenuCloseButton *back = new MenuCloseButton( "closeleaderboards" );
	back->SetShortProperties( "closeleaderboards", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMedium;
	m_backButton = back;

	/* Do a small button in the bottom right if too big up top 
	LeaderBoardMiniListButton* allminiviews = new LeaderBoardMiniListButton();
	allminiviews->SetShortProperties( "multiwinia_menu_allboards", leftX+(titleBarW*0.5f), yPos, buttonW, buttonH*0.4f, LANGUAGEPHRASE("multiwinia_menu_allgames") );
	allminiviews->m_fontSize = fontSmall;
	allminiviews->m_inputEnumUp = ControlDarwinianPromote;
	allminiviews->m_inputEnumDown = ControlDarwinianPromote;
	temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_menu_myscore"));
	allminiviews->AddCaptionToList(temp);
	temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_gametype_overall"));
	allminiviews->AddCaptionToList(temp);
	temp = new UnicodeString(LANGUAGEPHRASE("multiwinia_menu_friends"));
	allminiviews->AddCaptionToList(temp);
	RegisterButton( allminiviews );
	/**/

	LeaderBoardWeeklyButton* lbwb = new LeaderBoardWeeklyButton();
	UnicodeString weeklyCaption = LANGUAGEPHRASE("multiwinia_leaderboard_weekly_no");
	int length = weeklyCaption.WcsLen();
	lbwb->m_fontSize = fontSmall;
	int weeklyButtonW = titleBarW/3.0f;
	lbwb->SetShortProperties("multiwinia_leaderboard_weekly", leftX+titleBarW-weeklyButtonW, yPos+(buttonH*0.6f), weeklyButtonW, buttonH*0.4f, weeklyCaption);
	RegisterButton(lbwb);

}

void LeaderboardWindow::GetRankTextBit(unsigned _rank, char* _buffer)
{
	unsigned unit = _rank - (((int)(_rank/10))*10);
	unsigned tens = _rank - (((int)(_rank/100))*100) - unit;

	if( tens >= 10 && tens < 20 )
	{
		sprintf(_buffer, "th");
	}
	else
	{
		switch( unit )
		{
			case 1:
				sprintf(_buffer, "st");
				break;

			case 2:
				sprintf(_buffer, "nd");
				break;

			case 3:
				sprintf(_buffer, "rd");
				break;

			default:
				sprintf(_buffer, "th");
				break;
		}
	}
}

void LeaderboardWindow::SetCurrentButton( EclButton *_button )
{
	GameOptionsWindow::SetCurrentButton(_button);
}

void LeaderboardWindow::Update()
{
}

LeaderboardSelectionWindow::LeaderboardSelectionWindow()
:   GameOptionsWindow( "leaderboardselection" )
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);

    m_renderRightBox = false;
}

void LeaderboardSelectionWindow::Create()
{
	float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
	float buttonW, buttonH, gap;
	float fontLarge, fontMed, fontSmall;

    GetPosition_LeftBox( leftX, leftY, leftW, leftH );
    GetPosition_RightBox( rightX, rightY, rightW, rightH );
	GetButtonSizes(buttonW, buttonH, gap);
	GetFontSizes(fontLarge, fontMed, fontSmall);

	float buttonX = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY;
    float fontSize = fontMed;

    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "leaderboardselectiontitle", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_leaderboards" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

	// Single player boards

	LeaderBoardTypeButton *button = new LeaderBoardTypeButton();
    button->SetShortProperties( "multiwinia_leaderboards_sp_title", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_leaderboards_sp_title") );
	button->m_leaderboardType = LeaderBoardTypeButton::LeaderboardTypeSP;
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

	// Multiplayer versus CPU

	button = new LeaderBoardTypeButton();
    button->SetShortProperties( "multiwinia_leaderboards_cpu_title", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_leaderboards_cpu_title") );
	button->m_leaderboardType = LeaderBoardTypeButton::LeaderboardTypeCPU;
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

	// Multiplayer Ranked boards

	button = new LeaderBoardTypeButton();
    button->SetShortProperties( "multiwinia_leaderboards_mpr_title", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_leaderboards_mpr_title") );
	button->m_leaderboardType = LeaderBoardTypeButton::LeaderboardTypeMPRanked;
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

	// Multiplayer Standard boards

	button = new LeaderBoardTypeButton();
    button->SetShortProperties( "multiwinia_leaderboards_mpn_title", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_leaderboards_mpn_title") );
	button->m_leaderboardType = LeaderBoardTypeButton::LeaderboardTypeMPStandard;
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

	// Back button

	yPos = leftY + leftH - buttonH * 2;

	MenuCloseButton *back = new MenuCloseButton( "leaderboardselectionclose" );
    back->SetShortProperties( "leaderboardselectionclose", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    back->m_fontSize = fontSize;
    RegisterButton( back );
    m_buttonOrder.PutData( back );
	m_backButton = back;
}
