#ifndef _included_motd_button_h
#define _included_motd_button_h

#include "UI/ScrollingTextButton.h"
#include "lib/metaserver/metaserver.h"

class MOTDButton : public ScrollingTextButton
{
public:
	bool m_stopUpdating;
	MOTDButton()
	:	ScrollingTextButton(),
		m_stopUpdating(false)
	{
		m_ignoreButtons = true;
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
	{
		if( !m_stopUpdating )
		{
			const char *defaultLang = g_app->GetDefaultLanguage();
			const char *lang = g_prefsManager->GetString( "TextLanguage", defaultLang );
			Directory *motd = MetaServer_RequestData( NET_METASERVER_DATA_MOTD, lang );

			if( motd )
			{
				m_stopUpdating = true;
				if( strcmp( motd->GetDataString( NET_METASERVER_DATA_MOTD ), DIRECTORY_SAFESTRING ) != 0 &&
                    strlen( motd->GetDataString( NET_METASERVER_DATA_MOTD ) ) > 0 )
				{
					m_string = UnicodeString(motd->GetDataString( NET_METASERVER_DATA_MOTD ));
					m_scrollTimer = GetHighResTime() + 5.0f;
                    ClearWrapped();
					((GameMenuWindow *)m_parent)->m_motdFound = true;
				}
			}

			delete motd;
		}
		ScrollingTextButton::Render( realX, realY, highlighted, clicked );
	}

};

#endif