#include "lib/universal_include.h"

#include <string.h>

#include "lib/debug_utils.h"
#include "lib/targetcursor.h"
#include "lib/resource.h"

#include "interface/scrollbar.h"

#include "app.h"


ScrollBar::ScrollBar( EclWindow *parent )
:   m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_numRows(0),
    m_winSize(0),
    m_currentValue(0)
{
    AppDebugAssert( parent );
    strcpy( m_parentWindow, parent->m_name );
    strcpy( m_name, "New Scrollbar" );
}

ScrollBar::~ScrollBar()
{
}

void ScrollBar::Create( char *name, 
                        int x, int y, int w, int h,
                        int numRows, int winSize, 
                        int stepSize )
{

    strcpy( m_name, name );
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_numRows = numRows;
    m_winSize = winSize;

    EclWindow *parent = EclGetWindow( m_parentWindow );
    AppDebugAssert( parent );

    char barName[256];
    char upName[256];
    char downName[256];

    sprintf( barName, "%s bar", name );
    sprintf( upName, "%s up", name );
    sprintf( downName, "%s down", name );

    ScrollChangeButton *up = new ScrollChangeButton(this, stepSize*-1);
    up->SetProperties( upName, x, y, w, 18, UnicodeString(), UnicodeString(" ") );
    parent->RegisterButton( up );

    ScrollBarButton *bar = new ScrollBarButton(this);
    bar->SetProperties( barName, x, y+18, w, h-36, UnicodeString(" "), UnicodeString(" ") );
    parent->RegisterButton( bar );

    ScrollChangeButton *down = new ScrollChangeButton(this, stepSize);
    down->SetProperties( downName, x, y+h-18, w, 18, UnicodeString(), UnicodeString(" ") );
    parent->RegisterButton( down );

}

void ScrollBar::Remove()
{
    EclWindow *parent = EclGetWindow( m_parentWindow );
    if( parent )
    {
        char barName[256];
        char upName[256];
        char downName[256];

        sprintf( barName, "%s bar", m_name );
        sprintf( upName, "%s up", m_name );
        sprintf( downName, "%s down", m_name );

        parent->RemoveButton( barName );
        parent->RemoveButton( upName );
        parent->RemoveButton( downName );
    }
}

void ScrollBar::SetNumRows ( int newValue )
{
    m_numRows = newValue;
    SetCurrentValue( m_currentValue );
    EclDirtyWindow( m_parentWindow );
}

void ScrollBar::SetCurrentValue( int newValue )
{
    m_currentValue = newValue;

    if( m_numRows <= m_winSize )
    {
        m_currentValue = 0;
    }
    else
    {
        if( m_currentValue < 0 )
            m_currentValue = 0;

        if( m_currentValue > m_numRows - m_winSize )
            m_currentValue = m_numRows - m_winSize;
    }

    EclDirtyWindow( m_parentWindow );
}

void ScrollBar::SetWinSize ( int newValue )
{
    m_winSize = newValue;
    SetCurrentValue( m_currentValue );
    EclDirtyWindow( m_parentWindow );
}

void ScrollBar::ChangeCurrentValue( int newValue )
{
    SetCurrentValue( m_currentValue + newValue );
}


ScrollBarButton::ScrollBarButton( ScrollBar *scrollBar )
:   EclButton(),
    m_scrollBar( scrollBar ),
    m_grabOffset(-1)
{
}

void ScrollBarButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
    
    AppDebugAssert( m_scrollBar );

    // Background

    glColor3ub( 85, 93, 78 );
    glBegin( GL_QUADS );
        glVertex2i( realX, realY );
        glVertex2i( realX + m_w, realY );
        glVertex2i( realX + m_w, realY + m_h );
        glVertex2i( realX, realY + m_h );
    glEnd();

    glColor3ub( 187, 187, 187 );
    glBegin( GL_LINE_LOOP );
        glVertex2i( realX, realY );
        glVertex2i( realX + m_w, realY );
        glVertex2i( realX + m_w, realY + m_h );
        glVertex2i( realX, realY + m_h );
    glEnd();
    
    // Bar
    int barTop = int( m_h * (float) m_scrollBar->m_currentValue / (float) m_scrollBar->m_numRows);
    int barEnd = int( m_h * (float) (m_scrollBar->m_currentValue + m_scrollBar->m_winSize) / (float) m_scrollBar->m_numRows );    
    if( barEnd >= m_h ) barEnd = m_h-1;


    if( clicked )           glColor3ub(50,55,120);
    else if( highlighted )  glColor3ub(55,60,120);
    else                    glColor3ub(65,71,120);
        
    glBegin( GL_QUADS );
        glVertex2i( realX+1, realY+barTop+1 );
        glVertex2i( realX+m_w, realY+barTop+1 );
        glVertex2i( realX+m_w, realY+barEnd);
        glVertex2i( realX+1, realY+barEnd);
    glEnd();
}

void ScrollBarButton::MouseUp()
{    
    if( m_grabOffset == -1 )
    {

        AppDebugAssert( m_scrollBar );

        EclWindow *parent = EclGetWindow( m_scrollBar->m_parentWindow );
        AppDebugAssert( parent );

        int barTop = int( parent->m_y + m_y + m_h * (float) m_scrollBar->m_currentValue / (float) m_scrollBar->m_numRows );
        int barEnd = int( parent->m_y + m_y + m_h * (float) (m_scrollBar->m_currentValue + m_scrollBar->m_winSize) / (float) m_scrollBar->m_numRows );    
        if( barEnd >= m_h ) barEnd = m_h-1;

		int mouseY = g_target->Y();

        if( mouseY < barTop )
        {
            m_scrollBar->ChangeCurrentValue( -m_scrollBar->m_winSize );
        }
        else if( mouseY > barEnd )
        {
            m_scrollBar->ChangeCurrentValue( m_scrollBar->m_winSize );
        }

    }
    else
    {
        m_grabOffset = -1;
    }

}

void ScrollBarButton::MouseDown()
{
    if( m_grabOffset > -1 )
    {
        AppDebugAssert( m_scrollBar );
        int pixelsInside = g_target->Y() - ( m_parent->m_y + m_y );
        float fractionInside = (float) pixelsInside / (float) m_h;
        int centreVal = fractionInside * m_scrollBar->m_numRows;
        int desiredVal = centreVal - m_grabOffset;
        m_scrollBar->SetCurrentValue( desiredVal );
    }
    else
    {
        AppDebugAssert( m_scrollBar );
        EclWindow *parent = EclGetWindow( m_scrollBar->m_parentWindow );
        AppDebugAssert( parent );
        int mouseY = g_target->Y();
        int barTop = int( m_parent->m_y + m_y + m_h * (float) m_scrollBar->m_currentValue / (float) m_scrollBar->m_numRows );
        int barEnd = int( m_parent->m_y + m_y + m_h * (float) (m_scrollBar->m_currentValue + m_scrollBar->m_winSize) / (float) m_scrollBar->m_numRows );    
        //if( barEnd >= m_h ) barEnd = m_h-1;
        if( mouseY >= barTop && mouseY <= barEnd )
        {
            m_grabOffset = m_scrollBar->m_winSize * float( mouseY - barTop ) / float( barEnd - barTop );
        }
    }
}

ScrollChangeButton::ScrollChangeButton( ScrollBar *scrollbar, int amount )
:   DarwiniaButton(),
    m_scrollBar( scrollbar ),
    m_amount( amount )
{
    m_centered = true;
}

void ScrollChangeButton::MouseDown()
{
    AppDebugAssert( m_scrollBar );
    m_scrollBar->ChangeCurrentValue( m_amount );
}

void ScrollChangeButton::Render(int realX, int realY, bool highlighted, bool clicked)
{
    DarwiniaButton::Render( realX, realY, highlighted, clicked );

    int x = realX + float(m_w) * 0.05f;
    int y = realY + float(m_h) * 0.05f;
    int w = m_w * 0.9f;
    int h = m_h * 0.9f;   

    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/selectionarrow.bmp" ) );

    int dir = 0;
    if( m_amount > 0 ) dir = 1;
    else dir = -1;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    if( dir == 1 )
    {
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex2f( x, y );
            glTexCoord2i(1,0);      glVertex2f( x + w, y );
            glTexCoord2i(1,1);      glVertex2f( x + w, y + h );
            glTexCoord2i(0,1);      glVertex2f( x, y + h );
        glEnd();
    }
    else
    {
        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( x, y );
            glTexCoord2i(1,1);      glVertex2f( x + w, y );
            glTexCoord2i(1,0);      glVertex2f( x + w, y + h );
            glTexCoord2i(0,0);      glVertex2f( x, y + h );
        glEnd();
    }

    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );
}