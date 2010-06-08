#ifndef _included_soundsliderbutton_h
#define _included_soundsliderbutton_h

#include "interface/darwinia_window.h"
#include "interface/mainmenus.h"
#include "sound/sound_library_3d.h"
#include "user_input.h"
#include "app.h"
#include "lib/targetcursor.h"

class VolumeControl : public DarwiniaButton
{
public:
	float m_zeroPos;
	float m_endPos;
	float m_length;
	double m_timer;

	float m_volume;
	float m_old_volume;

	VolumeControl() : DarwiniaButton(), m_timer(0.0)
	{
		m_volume = (float)g_prefsManager->GetInt("SoundMasterVolume", 255) / 255.0f;
		m_zeroPos = 0.0f;
		m_endPos = 0.0f;
		m_length = 0.0f;
		m_old_volume = m_volume;
	}

	void CalcDimensions()
	{
		m_zeroPos = m_w*0.5f;
		m_endPos = m_w*0.95f;
		m_length = m_endPos-m_zeroPos;
	}

	void Render(int realX, int realY, bool highlighted, bool clicked)
	{
		DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;

		//Render background
		glColor4f( 0.3f, 1.0f, 0.3f, 1.0f );

		UpdateButtonHighlight();

		if( parent->m_buttonOrder[parent->m_currentButton] == this )
		{
			highlighted = true;

#ifdef _XBOX
			if( g_inputManager->controlEvent( ControlMenuLeft ) )   m_volume -= 0.1f;
            if( g_inputManager->controlEvent( ControlMenuRight ) )  m_volume += 0.1f;
#endif

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

		//Render the "track"
        glColor4f( 0.5f, 0.6f, 0.5f, 0.5f );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glBegin( GL_QUADS);
            glVertex2f( realX + m_zeroPos , realY + m_h/2 - m_h/6 );
            glVertex2f( realX + m_endPos, realY + m_h/2 - m_h/6);
            glVertex2f( realX + m_endPos, realY + m_h/2 + m_h/6 );
            glVertex2f( realX + m_zeroPos , realY + m_h/2 + m_h/6 );
        glEnd();

		//Do the Slider

		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );   
        glBegin( GL_QUADS);
            glVertex2f( m_x + m_zeroPos + m_length*m_volume - m_w*0.01f, realY );
            glVertex2f( m_x + m_zeroPos + m_length*m_volume + m_w*0.01f, realY );
            glVertex2f( m_x + m_zeroPos + m_length*m_volume + m_w*0.01f, realY + m_h );
            glVertex2f( m_x + m_zeroPos + m_length*m_volume - m_w*0.01f, realY + m_h );
        glEnd();

		//And Text

		float texY = realY + m_h/2 - m_fontSize/2 + 7.0f;

		glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, m_caption );
        g_titleFont.SetRenderOutline(false);

        glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );
        if( highlighted  )
        {
            glColor4f( 1.0, 0.3f, 0.3, 1.0f );
        }
        g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, m_caption );
	}

	void MouseDown()
	{
		float m_MouseX = g_target->X();
		float new_vol = (float)(m_MouseX - (m_x + m_zeroPos)) / (float)(m_length);

		if(new_vol != m_old_volume)
		{
			m_old_volume = m_volume;
			m_volume = new_vol;

			if(m_MouseX > m_x+m_endPos)
				m_volume = 1.0f;
			if(m_MouseX < m_x+(m_zeroPos))
				m_volume = 0.0f;

            g_soundLibrary3d->SetMasterVolume(m_volume*255);
		}
	}

	void MouseUp()
	{
		g_prefsManager->SetInt("SoundMasterVolume", m_volume*255);
		g_prefsManager->Save();
	}

};

#endif
