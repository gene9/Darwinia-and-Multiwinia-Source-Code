#include "lib/universal_include.h"
#include "lib/text_renderer.h"

#include "interface/controllerunplugged_window.h"
#include "interface/mainmenus.h"


ControllerUnpluggedWindow::ControllerUnpluggedWindow()
:   GameOptionsWindow( "controller_unplugged" ),
    m_dialogCreated(false)
{
}

void ControllerUnpluggedWindow::Update()
{
    if( !m_dialogCreated )
    {
        m_dialogCreated = true;
        CreateErrorDialogue( LANGUAGEPHRASE("dialog_unplugged_pc") );
    }

    if( !m_showingErrorDialogue ||
        g_inputManager->controlEvent( ControlMenuEscape ) )
    {
        EclRemoveWindow( m_name );
    }
}

void ControllerUnpluggedWindow::Render(bool _hasFocus)
{
    if( m_showingErrorDialogue )
    {
        RenderErrorDialogue();
    }
}