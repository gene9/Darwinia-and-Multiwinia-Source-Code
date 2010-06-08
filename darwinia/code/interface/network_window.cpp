#include "lib/universal_include.h"


#include <stdio.h>

#include "lib/text_renderer.h"
#include "lib/profiler.h"

#include "network/server.h"
#include "network/clienttoserver.h"

#include "app.h"
#include "main.h"

#include "interface/network_window.h"


NetworkWindow::NetworkWindow( char *name )
:   DarwiniaWindow( name )
{
}


void NetworkWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    //
    // Render some Networking stats

    if( g_app->m_server )
    {
#ifdef PROFILER_ENABLED
//        g_editorFont.DrawText2D( m_x + 10, m_y + 120, DEF_FONT_SIZE,
//			"Server SEND  : %4.0f bytes", g_app->m_profiler->GetTotalTime("Server Send") );
//        g_editorFont.DrawText2D( m_x + 10, m_y + 135, DEF_FONT_SIZE,
//			"Server RECV  : %4.0f bytes", g_app->m_profiler->GetTotalTime("Server Receive") );
#endif // PROFILER_ENABLED
        g_editorFont.DrawText2D( m_x + 10, m_y + 30,  DEF_FONT_SIZE,
			"SERVER SeqID : %d", g_app->m_server->m_sequenceId );

        int diff = g_app->m_server->m_sequenceId - g_lastProcessedSequenceId;
        g_editorFont.DrawText2D( m_x + 10, m_y + 60, DEF_FONT_SIZE, "Diff         : %d", diff );
    }

#ifdef PROFILER_ENABLED
//    g_editorFont.DrawText2D( m_x + 10, m_y + 160, DEF_FONT_SIZE,
//		"Client SEND  : %4.0f bytes", g_app->m_profiler->GetTotalTime("Client Send") );
//    g_editorFont.DrawText2D( m_x + 10, m_y + 175, DEF_FONT_SIZE,
//		"Client RECV  : %4.0f bytes", g_app->m_profiler->GetTotalTime("Client Receive") );
#endif // PROFILER_ENABLED
    g_editorFont.DrawText2D( m_x + 10, m_y + 45, DEF_FONT_SIZE,
		"CLIENT SeqID : %d", g_lastProcessedSequenceId );

    g_editorFont.DrawText2D( m_x + 10, m_y + 80, DEF_FONT_SIZE,
		"Inbox: %d", g_app->m_clientToServer->m_inbox.Size() );

    int nextSeqId = g_app->m_clientToServer->GetNextLetterSeqID();
    g_editorFont.DrawText2D( m_x + 10, m_y + 96, DEF_FONT_SIZE, "First Letter SeqID: %d", nextSeqId );
}

