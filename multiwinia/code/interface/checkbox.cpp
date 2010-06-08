#include "lib/universal_include.h"
#include "lib/text_renderer.h"
#include "lib/resource.h"

#include "network/networkvalue.h"

#include "interface/checkbox.h"
#include "interface/darwinia_window.h"

#include "app.h"

CheckBox::CheckBox()
:   DarwiniaButton(),
    m_networkBool(NULL),
	m_allocated(false)
{
}

CheckBox::~CheckBox()
{
	if( m_allocated )
		delete m_networkBool;
}

void CheckBox::Render(int realX, int realY, bool highlighted, bool clicked)
{
    if( highlighted )
    {
        glColor4ub( 199, 214, 220, 255 );

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glBegin( GL_QUADS);
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();
    }

    //float texY = realY + m_h/2 - m_fontSize/4;
    float texY = realY + m_h/2 - m_fontSize/2 + 7.0f;

    if( highlighted )
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_editorFont.SetRenderShadow(true);
        g_editorFont.DrawText2D( realX + 5, texY, m_fontSize, m_caption );
        g_editorFont.SetRenderShadow(false);
    }
    else
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_editorFont.SetRenderOutline(false);
        g_editorFont.DrawText2D( realX + 5, texY, m_fontSize, m_caption );
    }

    
    glColor4f( 0.3f, 0.3f, 0.3f, 1.0f );
    
    float w = m_h * 0.75f;
    float h = m_h * 0.75f;
    float x = (realX + m_w) - h * 2.0f;
    float y = realY + ((m_h - h) / 2.0f);
    
    glBegin( GL_QUADS );
        glVertex2f( x, y );
        glVertex2f( x + w, y );
        glVertex2f( x + w, y + h );
        glVertex2f( x, y + h );
    glEnd();

    if( IsChecked() )
    {
        x+= m_h * 0.05f;
        y+= m_h * 0.05f;
        w = h = m_h * 0.65f;   

        //glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/tick.bmp" ) );

        glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );
        glBegin( GL_QUADS );
            glTexCoord2i(0,1); glVertex2f( x, y );
            glTexCoord2i(1,1); glVertex2f( x + w, y );
            glTexCoord2i(1,0); glVertex2f( x + w, y + h );
            glTexCoord2i(0,0); glVertex2f( x, y + h );
        glEnd();

        glDisable( GL_TEXTURE_2D );
    }

    w = h = m_h * 0.75f;
    x = (realX + m_w) - h * 2.0f;
    y = realY + ((m_h - h) / 2.0f);

    glLineWidth(1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin( GL_LINE_LOOP );
        glVertex2i( x, y );
        glVertex2i( x+w, y );
        glVertex2i( x+w, y+h );
        glVertex2i( x, y+h );
    glEnd();
}

void CheckBox::MouseUp()
{
    Toggle();
}

void CheckBox::Toggle()
{
	if( m_networkBool )
	{
		bool current = *m_networkBool;
		*m_networkBool = !*m_networkBool;
		current = *m_networkBool;
		current = true;
	}
}

void CheckBox::Set( int value )
{
    Set( value != 0 );
}

void CheckBox::Set( bool value )
{
	if( m_networkBool )
    {
        *m_networkBool = value;
    }
}

void CheckBox::RegisterBool(bool *_bool)
{
    if( m_networkBool ) return;

	m_networkBool = new LocalBool( *_bool );
	m_allocated = true;
}

void CheckBox::RegisterInt(int *_int)
{
    if( m_networkBool ) return;

	m_networkBool = new LocalIntAsBool( *_int );
	m_allocated = true;

}

void CheckBox::RegisterNetworkBool( NetworkBool *_bool )
{
    if( m_networkBool ) return;

	m_networkBool = _bool;
	m_allocated = false;
}

void CheckBox::RegisterNetworkInt( NetworkInt *_int )
{
    if( m_networkBool ) return;

    m_networkBool = new NetworkIntAsBool( _int );
	m_allocated = true;
}

bool CheckBox::IsChecked()
{
	if( m_networkBool )
		return *m_networkBool != 0;
	else
		return false;
}