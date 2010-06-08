#include "lib/universal_include.h"
#include "lib/render/renderer.h"
#include "lib/gucci/window_manager.h"
#include "lib/language_table.h"

#include <string.h>

#include "message_dialog.h"



//*****************************************************************************
// Class MessageDialog
//*****************************************************************************

MessageDialog::MessageDialog(char *_name, char *_message, bool _messageIsLanguagePhrase, char *_title, bool _titleIsLanguagePhrase)
:	InterfaceWindow(_name, _title, _titleIsLanguagePhrase),
	m_numLines(0),
	m_messageIsLanguagePhrase(_messageIsLanguagePhrase),
	m_messageText(NULL)
{
	m_message = new char[ strlen( _message ) + 1 ];
	strcpy( m_message, _message );
}


MessageDialog::~MessageDialog()
{
	for (int i = 0; i < m_numLines; ++i)
	{
		delete [] m_messageLines[i];
	}

	delete [] m_message;
	if( m_messageText )
	{
		delete [] m_messageText;
	}
}


void MessageDialog::Create()
{
	GetLines();

    //
    // background button

    InvertedBox *invertedBox = new InvertedBox();
    invertedBox->SetProperties( "invert", 10, 30, m_w-20, m_h-60, " ", " ", false, false );
    RegisterButton( invertedBox );


	//
    // Close button
	
    CloseButton *closeButton = new CloseButton();
    closeButton->SetProperties( "Close", (m_w - 80)/2, m_h - 25, 80, 18, "dialog_close", " ", true, false );
    RegisterButton( closeButton );
}


void MessageDialog::Render(bool _hasFocus)
{
	GetLines();

	InterfaceWindow::Render(_hasFocus);
	
	for (int i = 0; i < m_numLines; ++i)
	{
		g_renderer->Text(m_x + 15, m_y + 35 + i * 12, White, 12, m_messageLines[i]);
	}
}


void MessageDialog::GetLines()
{
	char *messageLanguagePhrase;
	if( m_messageIsLanguagePhrase )
	{
		messageLanguagePhrase = LANGUAGEPHRASE(m_message);
	}
	else
	{
		messageLanguagePhrase = m_message;
	}

	if( !m_messageText || ( m_messageIsLanguagePhrase && strcmp( m_messageText, messageLanguagePhrase ) != 0 ) )
	{
		char *oldMessageText = m_messageText;
		m_messageText = new char[ strlen( messageLanguagePhrase ) + 1 ];
		strcpy( m_messageText, messageLanguagePhrase );

		for (int i = 0; i < m_numLines; ++i)
		{
			delete [] m_messageLines[i];
		}

		m_numLines = 0;
		char *lineStart = messageLanguagePhrase;

		while(1)
		{
			if (messageLanguagePhrase[0] == '\n' || messageLanguagePhrase[0] == '\0')
			{
				int lineLen = messageLanguagePhrase - lineStart;
				m_messageLines[m_numLines] = new char [lineLen + 1];
				strncpy(m_messageLines[m_numLines], lineStart, lineLen);
				m_messageLines[m_numLines][lineLen] = '\0';
				m_numLines++;
				lineStart = messageLanguagePhrase + 1;
			}

			if (messageLanguagePhrase[0] == '\0')
			{
				break;
			}

			messageLanguagePhrase++;
		}


		int widestWidth = 100;
		for( int i = 0; i < m_numLines; ++i )
		{
			int thisWidth = strlen(m_messageLines[i]) * 12 * 0.55f;
			if( thisWidth > widestWidth ) widestWidth = thisWidth;
		}

		int nm_w = widestWidth + 30;
		nm_w = std::max(nm_w, 250);
		int nm_h = 80 + m_numLines * 12;
		nm_h = std::max(nm_h, 150);
		if( !oldMessageText || nm_w > m_w || nm_h > m_h )
		{
			SetSize( nm_w, nm_h );

			for( int i = 0; i < m_buttons.Size(); i++ )
			{
				EclButton *b = m_buttons.GetData( i );
				if( strcmp( b->m_name, "invert" ) == 0 )
				{
					b->m_w = nm_w - 20;
					b->m_h = nm_h - 60;
				}
				else if( strcmp( b->m_name, "Close" ) == 0 )
				{
					b->m_x = ( nm_w - 80 ) / 2;
					b->m_y = nm_h - 25;
				}
			}
		}

		if( !oldMessageText )
		{
			int nm_x = g_windowManager->WindowW()/2 - nm_w/2;
			int nm_y = g_windowManager->WindowH()/2 - nm_h/2;
			SetPosition( nm_x, nm_y );
		}
		else
		{
			delete [] oldMessageText;
		}
	}
}

