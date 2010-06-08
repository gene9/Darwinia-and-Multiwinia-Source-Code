#include "lib/universal_include.h"

#include "shaman_interface.h"
#include "app.h"
#include "global_world.h"
#include "location.h"
#include "renderer.h"
#include "team.h"

#include "network/ClientToServer.h"

#include "lib/input/input.h"
#include "lib/resource.h"
#include "lib/targetcursor.h"
#include "lib/text_renderer.h"

#include "sound/soundsystem.h"

#include "worldobject/entity.h"
#include "worldobject/portal.h"
#include "worldobject/shaman.h"
#include "worldobject/triffid.h"

ShamanInterface::ShamanInterface()
:	m_visible(false),
	m_currentSelectedIcon(-1),
	m_alpha(0.0),
	m_circleOffset(0.0),
	m_movement(0)
{
	m_currentShamanId.SetInvalid();
}

void ShamanInterface::Advance()
{
	if( !m_visible ) return;
	if( m_alpha < 1.0 ) m_alpha += 1.0;
	static float movementAmount = 0.0;

	if( m_movement == 0 )
	{
		AdvanceSelectionInput();

		if( g_inputManager->controlEvent( ControlActivateTMButton ) )
		{
			g_app->m_clientToServer->RequestShamanSummon( m_currentShamanId.GetTeamId(), m_currentShamanId.GetUniqueId(), m_icons[m_currentSelectedIcon]->m_type );
			/*if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )*/ HideInterface();
		}
	}
	else
	{
		m_circleOffset += 0.1 * m_movement;
		movementAmount += 0.1;
		if( m_circleOffset > M_PI * 2  || 
			m_circleOffset < M_PI * -2 ) m_circleOffset = 0.0;

		if( movementAmount >= (M_PI * 2) / m_icons.Size() )
		{
			m_movement = 0;
			movementAmount = 0.0;
			m_circleOffset = ((M_PI * 2) / m_icons.Size()) * -m_currentSelectedIcon;
		}
	}

	//AdvanceMouseInIcon();
}

void ShamanInterface::AdvanceMouseInIcon()
{
	Vector2 mousePos( g_target->X(), g_target->Y() );
	for( int i = 0; i < m_icons.Size(); ++i )
	{
		float distance = (m_icons[i]->m_screenPos - mousePos).Mag();
		if( distance <= 60.0 ) // half icon size
		{	
			if( m_currentSelectedIcon != i )
			{
				m_currentSelectedIcon = i;
				g_app->m_soundSystem->TriggerOtherEvent( NULL, "MouseOverIcon", SoundSourceBlueprint::TypeInterface );
			}
			break;
		}
	}
}

void ShamanInterface::AdvanceSelectionInput()
{
	int oldSelection = m_currentSelectedIcon;

	if( g_inputManager->controlEvent( ControlMenuLeft ) ) 
	{
		m_movement = -1;
		m_currentSelectedIcon -= m_movement;
	}

	if( g_inputManager->controlEvent( ControlMenuRight ) ) 
	{
		m_movement = 1;
		m_currentSelectedIcon -= m_movement;
	}

	if( m_currentSelectedIcon < 0 ) m_currentSelectedIcon = m_icons.Size() - 1;
	if( m_currentSelectedIcon >= m_icons.Size() ) m_currentSelectedIcon = 0;


	if( oldSelection != m_currentSelectedIcon )
	{
		g_app->m_soundSystem->TriggerOtherEvent( NULL, "MouseOverIcon", SoundSourceBlueprint::TypeInterface );
	}
}

void ShamanInterface::Render()
{
	if( !m_visible ) return;

	g_gameFont.BeginText2D();

	RenderBackground();
	for( int i = 0; i < m_icons.Size(); ++i )
	{
		m_icons[i]->Render(m_alpha);
	}
	RenderSelectionIcon();
	RenderEggs();

	// render summon time
	/*if( m_icons.ValidIndex( m_currentSelectedIcon ) )
	{
		float summonTime = Shaman::GetSummonTime(m_icons[m_currentSelectedIcon]->m_type);
		char time[64];
		sprintf( time, "Time: %2.0", summonTime );

		int x = g_app->m_renderer->ScreenW() / 2.0;
		int y = (g_app->m_renderer->ScreenH() / 2.0) + 75;

		glColor4f( 1.0, 1.0, 1.0, 0.0);
		g_gameFont.SetRenderOutline(true);
		g_gameFont.DrawText2DCentre( x, y, 20.0, time );
		
		g_gameFont.SetRenderOutline(false);
		glColor4f( 1.0, 1.0, 1.0, 1.0 );
		g_gameFont.DrawText2DCentre( x, y, 20.0, time );
	}*/

	g_gameFont.EndText2D();
}

void ShamanInterface::RenderBackground()
{
	int screenCenterX = g_app->m_renderer->ScreenW() / 2;
	int screenCenterY = g_app->m_renderer->ScreenH() / 2;

	int x = screenCenterX - 275;
	int y = screenCenterY - 275;
	int size = 550;

	glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/shaman_shadow.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glDepthMask     ( false );
    glColor4f       ( m_alpha, m_alpha, m_alpha, 0.0 );         
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( x, y + size );
        glTexCoord2i( 1, 1 );           glVertex2f( x + size, y + size );
        glTexCoord2i( 1, 0 );           glVertex2f( x + size, y );
        glTexCoord2i( 0, 0 );           glVertex2f( x, y );
    glEnd();

	char bmpFilename[256];
    sprintf( bmpFilename, "textures/shaman_background.bmp" );
    unsigned int texId = g_app->m_resource->GetTexture( bmpFilename );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, texId );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    
    glColor4f   ( 1.0, 1.0, 1.0, m_alpha );        
         
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( x, y + size );
        glTexCoord2i( 1, 1 );           glVertex2f( x + size, y + size );
        glTexCoord2i( 1, 0 );           glVertex2f( x + size, y );
        glTexCoord2i( 0, 0 );           glVertex2f( x, y );
    glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );
}

void ShamanInterface::RenderEggs()
{
	Shaman *shaman = (Shaman *)g_app->m_location->GetEntitySafe( m_currentShamanId, Entity::TypeShaman );
	if( !shaman ) return;

	float iconSize = 120.0;
    float shadowSize = iconSize + 5;   

	int x = (g_app->m_renderer->ScreenW() / 2);
	int y = g_app->m_renderer->ScreenH() / 2;


	int queueId = -1;
	bool renderedCurrentSummon = false;

	Vector2 iconCentre = Vector2( x, y );

	glEnable        ( GL_TEXTURE_2D );
	glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/shaman_shadow.bmp" ) );
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
	glDepthMask     ( false );
	glColor4f       ( m_alpha, m_alpha, m_alpha, 0.0 );         
	glBegin( GL_QUADS );
		glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowSize/2, iconCentre.y - shadowSize/2 );
		glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowSize/2, iconCentre.y - shadowSize/2 );
		glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowSize/2, iconCentre.y + shadowSize/2 );
		glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowSize/2, iconCentre.y + shadowSize/2 );
	glEnd();

    int entityId = shaman->GetSummonType();
    if( entityId != Entity::TypeInvalid )
	{
        glColor4f( 0.5, 0.5, 0.5, 1.0 );
		char bmpFilename[256];
		sprintf( bmpFilename, "icons/%s", GetIconFileName( entityId ) );    
		unsigned int texId = g_app->m_resource->GetTexture( bmpFilename );       

		glEnable        ( GL_TEXTURE_2D );
		glBindTexture   ( GL_TEXTURE_2D, texId );
		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
	         
		glBegin( GL_QUADS );
			glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y - iconSize/2 );
			glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y - iconSize/2 );
			glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y + iconSize/2 );
			glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y + iconSize/2 );
		glEnd();
	}

	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable       ( GL_TEXTURE_2D );
}

void ShamanInterface::RenderSelectionIcon()
{
	int x, y;
	GetIconPos( 0, &x, &y, false );
	Vector2 iconCentre(x, y);

	int iconSize = 130;
    float shadowSize = iconSize;

	char shadowFileName[256];
    sprintf( shadowFileName, "shadow_icons/mouse_selection.bmp");

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( shadowFileName ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glDepthMask     ( false );
    glColor4f       ( m_alpha, m_alpha, m_alpha, 0.0 );         
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowSize/2, iconCentre.y - shadowSize/2 );
        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowSize/2, iconCentre.y - shadowSize/2 );
        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowSize/2, iconCentre.y + shadowSize/2 );
        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowSize/2, iconCentre.y + shadowSize/2 );
    glEnd();


    char bmpFilename[256];
    sprintf( bmpFilename, "icons/mouse_selection.bmp" );
    unsigned int texId = g_app->m_resource->GetTexture( bmpFilename );       

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, texId );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    
    glColor4f   ( 1.0, 1.0, 0.0, m_alpha );        
         
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y - iconSize/2 );
        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y - iconSize/2 );
        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y + iconSize/2 );
        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y + iconSize/2 );
    glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );

}

void ShamanInterface::ShowInterface(WorldObjectId _shamanId)
{
	if( m_visible ) return;

	Shaman *shaman = (Shaman *)g_app->m_location->GetEntity(_shamanId );
	if( shaman && shaman->m_type == Entity::TypeShaman )
	{
		m_visible = true;
		m_currentShamanId = _shamanId;

		SetupIcons();

		if( m_icons.Size() > 0 ) m_currentSelectedIcon = 0;
	}	
}

void ShamanInterface::SetupIcons()
{
	int numIcons = 0;
	for( int i = 0; i < Entity::NumEntityTypes; ++i )
	{
        if( Shaman::IsSummonable(i) )
		{
			ShamanIcon *icon = new ShamanIcon(i);
			icon->m_iconId = numIcons;
			numIcons++;
			m_icons.PutData( icon );
		}
	}

    bool specialSummon = false;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( i == g_app->m_location->GetMonsterTeamId() ) continue;
        if( g_app->m_location->GetFuturewinianTeamId() == i ) continue;
        if( g_app->m_location->m_teams[i]->m_teamType == TeamTypeUnused ) continue;
        /*if( Portal::s_masterPortals[i]->m_vunerable[g_app->m_globalWorld->m_myTeamId] )
        {
            specialSummon = true;
        }*/
    }
    
    if( specialSummon )
    {
        ShamanIcon *icon = new ShamanIcon(-1);
	    icon->m_iconId = numIcons;
        numIcons++;
	    m_icons.PutData( icon );
    }

    Shaman *shaman = (Shaman *)g_app->m_location->GetEntitySafe( m_currentShamanId, Entity::TypeShaman );
    if( shaman )
    {
        if( shaman->CanCapturePortal() )
        {
            ShamanIcon *icon = new ShamanIcon( Entity::NumEntityTypes );
            icon->m_iconId = numIcons;
            numIcons++;
            m_icons.PutData( icon );
        }
    }

	for( int i = 0; i < m_icons.Size(); ++i )
	{
		int x, y;
		GetIconPos( i, &x, &y );
		m_icons[i]->SetPos( x, y );
	}
}

void ShamanInterface::GetIconPos( int _iconNumber, int *_x, int *_y, bool _useOffset )
{
	int screenCenterX = g_app->m_renderer->ScreenW() / 2;
	int screenCenterY = g_app->m_renderer->ScreenH() / 2;

	int numIcons = m_icons.Size();
	int ringRadius = 175;// + (int(numIcons / 8) * 25);

	float degreesPerIcon = (2 * M_PI) / numIcons;
	float degrees = -(M_PI/2.0) + ( _iconNumber * degreesPerIcon );
	if( _useOffset ) degrees += m_circleOffset;

	*_x = screenCenterX + iv_cos(degrees) * ringRadius;
	*_y = screenCenterY + iv_sin(degrees) * ringRadius;
}

void ShamanInterface::HideInterface()
{
	m_visible = false;
	m_currentShamanId.SetInvalid();
	m_currentSelectedIcon = -1;
	m_circleOffset = 0.0;
	m_movement = 0;

	m_icons.EmptyAndDelete();
}

bool ShamanInterface::IsVisible()
{
	return m_visible;
}

int ShamanInterface::GetNumIcons()
{
    return m_icons.Size();
}

WorldObjectId ShamanInterface::GetShamanId()
{
	return m_currentShamanId;
}

char *ShamanInterface::GetIconFileName(int _type)
{
    if( _type == -1 || _type == 255 ) return "shaman_special.bmp";

	switch( _type )
	{
		case Entity::TypeCentipede:			return "shaman_centipede.bmp";
		case Entity::TypeArmyAnt:			return "shaman_ants.bmp";
		case Entity::TypeSoulDestroyer:		return "shaman_souldestroyer.bmp";
		case Entity::TypeSpider:			return "shaman_spider.bmp";
		case Entity::TypeSporeGenerator:	return "shaman_sporegenerator.bmp";
		case Entity::TypeTripod:			return "shaman_tripod.bmp";
		case Entity::TypeVirii:				return "shaman_virii.bmp";
		case Entity::TypeDarwinian:			return "shaman_darwinian.bmp";
        case Entity::NumEntityTypes:        return "shaman_capture.bmp";

		default: return NULL;
	}
}

// ======================================================

ShamanIcon::ShamanIcon(int _type)
:	m_type(_type),
	m_iconId(-1)
{
	m_screenPos.Zero();
	m_iconFilename = ShamanInterface::GetIconFileName( _type );
}

void ShamanIcon::SetPos( int _x, int _y )
{
	m_screenPos.x = _x;
	m_screenPos.y = _y;
}

void ShamanIcon::Render(float _alpha)
{
	Shaman *shaman = (Shaman *)g_app->m_location->GetEntitySafe( g_app->m_shamanInterface->GetShamanId(), Entity::TypeShaman );
	if( !shaman ) return;

    float iconSize = 120.0;
    if( g_app->m_shamanInterface->GetNumIcons() > 8 )
    {
        iconSize *= (8.0 / (float)g_app->m_shamanInterface->GetNumIcons());   // 8 is the max full size icons that can fit, so scale down if there are more
    }
    float shadowSize = iconSize + 5;


	int x, y;
	g_app->m_shamanInterface->GetIconPos( m_iconId, &x, &y );
	Vector2 iconCentre = Vector2( x, y );
//    Vector2 iconCentre = Vector2(m_screenPos.x, m_screenPos.y);

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/shaman_shadow.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glDepthMask     ( false );
    glColor4f       ( _alpha, _alpha, _alpha, 0.0 );         
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowSize/2, iconCentre.y - shadowSize/2 );
        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowSize/2, iconCentre.y - shadowSize/2 );
        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowSize/2, iconCentre.y + shadowSize/2 );
        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowSize/2, iconCentre.y + shadowSize/2 );
    glEnd();


    char bmpFilename[256];
    sprintf( bmpFilename, "icons/%s", m_iconFilename );    
    unsigned int texId = g_app->m_resource->GetTexture( bmpFilename );       

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, texId );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    
    glColor4f( 1.0, 1.0, 1.0, _alpha );

    if( shaman->CanSummonEntity( m_type ) || m_type == -1 || m_type == Entity::NumEntityTypes )
	{
		glColor4f( 1.0, 1.0, 1.0, _alpha );        
	}
	else
	{
		glColor4f( 0.5, 0.5, 0.5, _alpha );
	}
         
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y - iconSize/2 );
        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y - iconSize/2 );
        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y + iconSize/2 );
        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y + iconSize/2 );
    glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );

}