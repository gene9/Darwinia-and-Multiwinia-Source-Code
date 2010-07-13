#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/math_utils.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/math/random_number.h"
#include "lib/resource.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#include "sound/soundsystem.h"

#include "worldobject/multiwiniazone.h"
#include "worldobject/ai.h"
#include "worldobject/statue.h"
#include "worldobject/darwinian.h"
#include "worldobject/flag.h"

#include "app.h"
#include "location.h"
#include "team.h"
#include "entity_grid.h"
#include "main.h"
#include "particle_system.h"
#include "global_world.h"
#include "camera.h"
#include "multiwinia.h"
#include "multiwiniahelp.h"
#include "level_file.h"

int MultiwiniaZone::s_blitzkriegBaseZone[NUM_TEAMS];


MultiwiniaZone::MultiwiniaZone()
:   Building(),
    m_size(50.0),
    m_recountTimer(0.0),
    m_life(-1.0),
    m_aiTarget(NULL),
    m_totalCount(0),
    m_blitzkriegFlag(NULL),
    m_blitzkriegOwnership(0.0),
    m_blitzkriegCaptureTeam(255),
    m_blitzkriegUpOrDown(0),
    m_blitzkriegLocked(true),
    m_numStandardLinks(0),
    m_blitzkriegUnderAttackMessageTimer(0.0)
{
    m_type = TypeMultiwiniaZone;
    memset( m_teamCount, 0, NUM_TEAMS * sizeof(int) );
}

MultiwiniaZone::~MultiwiniaZone()
{
    if( m_aiTarget )
    {
        m_aiTarget->m_destroyed = true;
        m_aiTarget->m_neighbours.EmptyAndDelete();
		// AITarget will be part of the Location's building list
		// and will be properly deleted by the Location
    }


	delete m_blitzkriegFlag;
	m_blitzkriegFlag = NULL;
}


void MultiwiniaZone::Initialise ( Building *_template )
{
    Building::Initialise( _template );
    m_size = ((MultiwiniaZone *) _template)->m_size;
    m_centrePos = m_pos + Vector3(0,m_radius/2,0);
    
    MultiwiniaZone *zone = (MultiwiniaZone *)_template;
    for( int i = 0; i < zone->m_blitzkriegLinks.Size(); ++i )
    {
        m_blitzkriegLinks.PutData( zone->m_blitzkriegLinks[i] );
    }

    m_numStandardLinks = m_blitzkriegLinks.Size();

    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig )
    {
        if( m_id.GetTeamId() != 255 )
        {
            s_blitzkriegBaseZone[m_id.GetTeamId()] = m_id.GetUniqueId();
            SetTeamId(255);
        }
    }
}


void MultiwiniaZone::SetDetail( int _detail )
{
    Building::SetDetail( _detail );

    m_radius = m_size;
}


void MultiwiniaZone::Advance_KingOfTheHill()
{
#ifdef INCLUDEGAMEMODE_KOTH

    if( GetNetworkTime() > m_recountTimer )
    {
        m_totalCount = 0;

        memset( m_teamCount, 0, NUM_TEAMS*sizeof(int) );
        m_totalCount = 0;

        bool includes[NUM_TEAMS];
        memset( includes, true, sizeof(bool) * NUM_TEAMS );
        int numFound;
        g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, m_size, &numFound, includes );
        int entitiesCounted = 0;

        for( int i = 0; i < numFound; ++i )
        {
            WorldObjectId id = s_neighbours[i];
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && 
                entity->m_type == Entity::TypeDarwinian && 
                !entity->m_dead &&
                !((Darwinian *)entity)->IsOnFire() &&
                ((Darwinian *)entity)->m_state != Darwinian::StateInsideArmour )
            {
                m_teamCount[ entity->m_id.GetTeamId() ]++;
                m_totalCount++;        
            }
        }

        int newTeamId = 255;
        int highestCount = 0;

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            if( m_teamCount[t] > highestCount )
            {
                newTeamId = t;
                highestCount = m_teamCount[t];
            }
        }

        SetTeamId( newTeamId );
        m_recountTimer = GetNetworkTime() + 1.0;
    }

#endif
}


void MultiwiniaZone::Advance_Blitzkrieg()
{
#ifdef INCLUDEGAMEMODE_BLITZ
    if( GetNetworkTime() > m_recountTimer )
    {
        //
        // Update our locked status

        m_blitzkriegLocked = IsBlitzkriegZoneLocked();


        //
        // Count entities nearby

        m_totalCount = 0;
        int newTeamId = 255;
        int highestCount = 0;

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            if( g_app->m_location->m_teams[t]->m_teamType != TeamTypeUnused )
            {
                bool includes[NUM_TEAMS];
                memset( includes, false, sizeof(bool) * NUM_TEAMS );
                includes[t] = true;

                if( m_blitzkriegLocked  )
                {
                    // If we are locked dont count any teams other than our own
                }
                else
                {
                    int numFound;
                    double range = 40.0;
                    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, range, &numFound, includes );
                    int entitiesCounted = 0;

                    for( int i = 0; i < numFound; ++i )
                    {
                        WorldObjectId id = s_neighbours[i];
                        Entity *entity = g_app->m_location->GetEntity( id );
                        if( entity && 
                            entity->m_type == Entity::TypeDarwinian && 
                            !entity->m_dead &&
                            !((Darwinian *)entity)->IsOnFire() &&
                            ((Darwinian *)entity)->m_state != Darwinian::StateInsideArmour )
                        {
                            ++entitiesCounted;
                        }
                    }

                    m_teamCount[t] = entitiesCounted;
                    m_totalCount += entitiesCounted;

                    if( m_teamCount[t] > highestCount )
                    {
                        newTeamId = t;
                        highestCount = m_teamCount[t];
                    }
                }
            }
        }

        //
        // Calculate change of ownership

        double oldBlitzkriegOwnership = m_blitzkriegOwnership;
        
        if( newTeamId != 255 )
        {            
            double fractionOwned = (double)highestCount/(double)m_totalCount;
            double change = 5.0 * highestCount * fractionOwned;

            if( m_id.GetTeamId() != 255 )
            {
                m_blitzkriegCaptureTeam = m_id.GetTeamId();
            }

            if( newTeamId != m_blitzkriegCaptureTeam )
            {                
                m_blitzkriegOwnership -= change * SERVER_ADVANCE_PERIOD;
                if( m_blitzkriegOwnership <= 0.0 )
                {
                    SetTeamId( 255 );
                    m_blitzkriegCaptureTeam = newTeamId;
                }
            }
            else if( newTeamId == m_blitzkriegCaptureTeam )
            {
                m_blitzkriegOwnership += change * SERVER_ADVANCE_PERIOD;                
                if( m_blitzkriegOwnership >= 100.0 )
                {
                    SetTeamId( newTeamId );
                }
            }
        }
        else
        {
            double change = 10.0;

            if( m_id.GetTeamId() == 255 )
            {
                m_blitzkriegOwnership -= change * SERVER_ADVANCE_PERIOD;
                if( m_blitzkriegOwnership <= 0.0 )
                {
                    m_blitzkriegCaptureTeam = 255;
                }
            }
            else
            {
                //m_blitzkriegOwnership += change * SERVER_ADVANCE_PERIOD;
            }
        }
        

        if( m_blitzkriegOwnership < 0.0 ) m_blitzkriegOwnership = 0.0;
        if( m_blitzkriegOwnership > 100.0 ) m_blitzkriegOwnership = 100.0;

        if      ( m_blitzkriegOwnership > oldBlitzkriegOwnership )      m_blitzkriegUpOrDown = +1;        
        else if ( m_blitzkriegOwnership < oldBlitzkriegOwnership )      m_blitzkriegUpOrDown = -1;       
        else                                                            m_blitzkriegUpOrDown = 0;

        if( m_blitzkriegUnderAttackMessageTimer > 0.0 ) m_blitzkriegUnderAttackMessageTimer -= SERVER_ADVANCE_PERIOD;
        int baseTeam = GetBaseTeamId();
        if( baseTeam != -1 && m_blitzkriegUnderAttackMessageTimer <= 0.0 && !g_app->m_multiwinia->m_eliminated[baseTeam])
        {
            if( baseTeam == m_id.GetTeamId() && m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId &&
                g_app->m_location->m_entityGrid->GetNumEnemies( m_pos.x, m_pos.z, m_size, m_id.GetTeamId() ) > 0 )
            {
                m_blitzkriegUnderAttackMessageTimer = 20.0;
                g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_baseunderattack") );
            }
        }

        AITarget *t = (AITarget *)g_app->m_location->GetBuilding( AI::FindNearestTarget(m_pos) );
        if( t && t->m_type == Building::TypeAITarget )
        {
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                double p = 1.0;
                if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeHard ||
                    g_app->m_multiwinia->m_aiType == Multiwinia::AITypeStandard )
                {
                    p = GetBlitzkriegPriority( m_id.GetUniqueId(), i );
                }

                t->m_priority[i] = p;
                if( p > 0.0 ) t->m_aiObjectiveTarget[i] = true;
                else t->m_aiObjectiveTarget[i] = false;
            }
        }

        m_recountTimer = GetNetworkTime() + 1.0;
    }

#endif
}

int MultiwiniaZone::GetBaseTeamId()
{
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( s_blitzkriegBaseZone[i] == m_id.GetUniqueId() ) return i;
    }
    return -1;
}

double MultiwiniaZone::GetBlitzkriegPriority(int _zoneId, int _teamId)
{
    int score = g_app->m_multiwinia->m_teams[_teamId].m_score;
    double priority = 0.0;
    MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->GetBuilding( _zoneId );

    if( zone )
    {
        switch( score )
        {
            case BLITZKRIEGSCORE_SAFE:
            {
                if( zone->m_id.GetTeamId() != _teamId ) 
                {
                    priority = 1.0;
                }
                break;
            }

            case BLITZKRIEGSCORE_ENEMYOCCUPIED:
            case BLITZKRIEGSCORE_UNDERATTACK:
            case BLITZKRIEGSCORE_UNOCCUPIED:
            {
                if( zone->IsBlitzkriegBaseZone() )
                {
                    if( zone->m_id.GetTeamId() != 255 &&
                        !g_app->m_multiwinia->m_eliminated[zone->m_id.GetTeamId()] )
                    {
                        priority = 1.0;
                    }
                }
                break;
            }

            case BLITZKRIEGSCORE_UNDERTHREAT:
            {
                for( int i = 0; i < zone->m_blitzkriegLinks.Size(); ++i )
                {
                    if( zone->m_blitzkriegLinks[i] == s_blitzkriegBaseZone[_teamId] &&
                        zone->m_id.GetTeamId() != _teamId )
                    {
                        priority = 1.0;
                    }
                }
                break;
            }
        }
    }

    if( !zone->IsBlitzkriegBaseZone() && zone->m_id.GetTeamId() == _teamId ) priority = 0.0;

    return priority;
}

void MultiwiniaZone::Advance_CaptureTheStatue()
{
#ifdef INCLUDEGAMEMODE_CTS

    if( m_id.GetTeamId() != 255 )
    {
        if( GetNetworkTime() > m_recountTimer )
        {
            m_recountTimer = GetNetworkTime() + 2.0;

            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *building = g_app->m_location->m_buildings[i];
                    if( building &&
                        building->m_type == Building::TypeStatue &&
                        building->m_id.GetTeamId() == m_id.GetTeamId() ) 
                    {
                        double distance = ( building->m_pos - m_pos ).Mag();
                        if( distance < 30 )
                        {
                            ((Statue *)building)->Destroy();

                            g_app->m_multiwinia->AwardPoints( m_id.GetTeamId(), 1 );

                            if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
                            {
                                g_app->m_multiwiniaHelp->NotifyCapturedStatue();
                            }

                            Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];

                            UnicodeString msg = LANGUAGEPHRASE("multiwinia_cts_statuecapture");
                            msg.ReplaceStringFlag( L'T', team->GetTeamName() );
                            g_app->m_location->SetCurrentMessage( msg );

                            g_app->m_soundSystem->TriggerBuildingEvent( this, "CaptureStatue" );
                        }
                    }
                }
            }
        }
    }

#endif
}



bool MultiwiniaZone::Advance()
{
    //
    // Is the world awake yet ?

    if( !g_app->m_location ) return false;
    if( !g_app->m_location->m_teams ) return false;
    
    if( m_aiTarget )
    {
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            // set the ai target to high priority
            m_aiTarget->m_priority[i] = 0.9 + syncfrand(0.1);
            if( m_id.GetTeamId() != i &&
                m_id.GetTeamId() != -1 )
            {
                // if another team is holding this point, maximise priority
                m_aiTarget->m_priority[i] = 1.1;
            }
        }
    }

    switch( g_app->m_multiwinia->m_gameType )
    {
        case Multiwinia::GameTypeKingOfTheHill:         Advance_KingOfTheHill();        break;
        case Multiwinia::GameTypeCaptureTheStatue:      Advance_CaptureTheStatue();     break;
        case Multiwinia::GameTypeBlitzkreig:            Advance_Blitzkrieg();           break;
    }



    //
    // Are we supposed to shut down now?

    if( m_life > -1.0 )
    {
        m_life -= SERVER_ADVANCE_PERIOD;
        if( m_life <= 0.0 ) return true;
    }

    return false;
}

 
void MultiwiniaZone::Render( double predictionTime )
{
    if( g_app->m_editing )
    {
#ifdef DEBUG_RENDER_ENABLED
        RenderSphere( m_pos, 20.0, RGBAColour(255,255,255,255) );
#endif
    }


    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig )
    {
        glDisable( GL_LIGHTING );
        RenderBlitzkrieg();
        glEnable( GL_LIGHTING );
    }
}


void MultiwiniaZone::RenderZoneEdge( double startAngle, double totalAngle, RGBAColour colour, bool animated )
{
    int numSteps = totalAngle * 3;
    numSteps = max( numSteps, 1 );


    //
    // Basic outline on ground

    double angle = startAngle;
    
    double timeFactor = g_gameTimer.GetAccurateTime();
    if( animated ) timeFactor *= 2.0;
    
    glBegin( GL_QUAD_STRIP );
    for( int i = 0; i <= numSteps; ++i )
    {        
        double colMod = 0.25 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.75);
        double alpha = 0.2 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.8);
        if( alpha < 0.2 ) alpha = 0.2;
        if( colMod < 0.25 ) colMod = 0.25;

        glColor4ub( colour.r * colMod, colour.g * colMod, colour.b * colMod, alpha*colour.a );

        double xDiff = m_size * iv_sin(angle);
        double zDiff = m_size * iv_cos(angle);
        Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        pos.y -= 20;
        glVertex3dv( pos.GetData() );
        pos.y += 50;
        glVertex3dv( pos.GetData() );
        angle += totalAngle / (double) numSteps;
    }
    glEnd();


    //
    // Small glow tube

    if( animated )
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser-long.bmp" ) );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

        //glDisable( GL_DEPTH_TEST );

        angle = startAngle;

        glBegin( GL_QUAD_STRIP );
        for( int i = 0; i <= numSteps; ++i )
        {
            double alpha = 0.5 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.5);

            glColor4ub( colour.r, colour.g, colour.b, alpha*colour.a );

            double xDiff = m_size * iv_sin(angle);
            double zDiff = m_size * iv_cos(angle);
            Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
            pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
            pos.y -= 20;        
            glTexCoord2f(i + timeFactor,0);       glVertex3dv( pos.GetData() );
            pos.y += 100;
            glTexCoord2f(i + timeFactor,1);       glVertex3dv( pos.GetData() );
            angle += totalAngle / (double) numSteps;
        }
        glEnd();

        glDisable( GL_TEXTURE_2D );

        //glEnable( GL_DEPTH_TEST );
    }


    //
    // Glow tube running up to sky

    angle = startAngle;

    glShadeModel( GL_SMOOTH );
    glBegin( GL_QUAD_STRIP );
    for( int i = 0; i <= numSteps; ++i )
    {
        double colMod = 0.25 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.75);
        double alpha = 0.2 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.7);
        alpha *= 0.1;

        glColor4ub( colour.r * colMod, colour.g * colMod, colour.b * colMod, alpha*colour.a );

        double xDiff = m_size * iv_sin(angle);
        double zDiff = m_size * iv_cos(angle);
        Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        if( pos.y < 2 ) pos.y = 2;
        glVertex3dv( pos.GetData() );
        pos.y += 1200;
        glColor4ub( colour.r * colMod, colour.g * colMod, colour.b * colMod, 0.1 );
        glVertex3dv( pos.GetData() );
        angle += totalAngle / (double) numSteps;
    }
    glEnd();
    glShadeModel( GL_FLAT );

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}


void MultiwiniaZone::RenderAlphas( double predictionTime )
{
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    //glShadeModel( GL_SMOOTH );
    glDepthMask( false );

    double angle = 0;
    double timeFactor = g_gameTimer.GetAccurateTime() * 0.5;
        
    double camDist = (g_app->m_camera->GetPos() - m_pos).Mag();
    double alphaFactor = 1.0;
    if( camDist < 500 ) alphaFactor = camDist/500;
    if( alphaFactor < 0.4 ) alphaFactor = 0.4;

    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeKingOfTheHill )
    {
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( m_teamCount[i] > 0 )
            {
                double thisAngle = 2.0 * M_PI * m_teamCount[i] / m_totalCount;
                RGBAColour colour = g_app->m_location->m_teams[i]->m_colour;
                colour.a = 100;
                bool animated = false;
                if( m_id.GetTeamId() == i )
                {
                    animated = true;
                    colour.a = 255;
                }
                colour.a *= alphaFactor;
                RenderZoneEdge( angle + timeFactor, thisAngle, colour, animated );
                angle += thisAngle;
            }
        }

        double remainder = 2.0 * M_PI - angle;
        if( remainder > 0.01 )
        {
            RGBAColour colour( 128, 128, 128, 255 );
            RenderZoneEdge( angle + timeFactor, remainder, colour, false );
        }
    }
    else if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeCaptureTheStatue )
    {
        // Capture statue
        RGBAColour colour(128,128,128,255);
        if( m_id.GetTeamId() != 255 ) 
        {
            colour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
            colour.a = 100;
        }

        RenderZoneEdge( 0, 2.0 * M_PI, colour, true );
    }
    else if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig )
    {
        //
        // Render zone

        RGBAColour colour(128,128,128,255);
        if( m_id.GetTeamId() != 255 ) 
        {
            colour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
            colour.a = 100;
        }
        else if( m_blitzkriegCaptureTeam != 255 )
        {
            colour = g_app->m_location->m_teams[m_blitzkriegCaptureTeam]->m_colour;
            colour.a = 100;
        }

        double ownedAngle = m_blitzkriegOwnership/100.0 * 2.0 * M_PI;
        double unclaimedAngle = 2.0 * M_PI - ownedAngle;

        RenderZoneEdge( 0, ownedAngle, colour, true );
        RenderZoneEdge( ownedAngle, unclaimedAngle, RGBAColour(128,128,128,255), true );
    }
    else if( g_app->m_editing )
    {
        // Capture statue
        RGBAColour colour(128,128,128,255);
        if( m_id.GetTeamId() != 255 ) 
        {
            colour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
            colour.a = 100;
        }

        RenderZoneEdge( 0, 2.0 * M_PI, colour, true );

        for( int i = 0; i < m_blitzkriegLinks.Size(); ++i )
        {
            Building *link = g_app->m_location->GetBuilding( m_blitzkriegLinks[i] );
            if( link )
            {
                RenderArrow( m_pos, link->m_pos, 5.0 );
            }
        }
    }

    glEnable( GL_CULL_FACE );
    glShadeModel( GL_FLAT );
    glDepthMask( true );
}


void MultiwiniaZone::RenderBlitzkrieg()
{   
    //
    // Sort out flag pos
    /*if( m_id.GetTeamId() != 255 )
    {
        RGBAColour col = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
        
        for( int i = 0; i < m_blitzkriegLinks.Size(); ++i )
        {
            MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->GetBuilding( m_blitzkriegLinks[i] );
            if( zone )
            {
                Vector3 fromPos, toPos;
                fromPos = m_pos;
                toPos = zone->m_pos;
                fromPos.y += 50.0;
                toPos.y += 50.0;
                RenderArrow( fromPos, toPos, 2.0, col );
            }
        }
    }*/

    double size = 40.0;
    Vector3 up = g_upVector;
    Vector3 front = m_front;
    front.y = 0;
    front.Normalise();

    double scaleFactor = g_prefsManager->GetFloat( "DarwinianScaleFactor" );
    double camDist = ( g_app->m_camera->GetPos() - m_pos ).Mag();
    if( camDist > 100 ) size *= 1.0 + scaleFactor * ( camDist - 100 ) / 1500;

    double factor = ( camDist ) / 200;            
    if( factor < 0.0 ) factor = 0.0;
    if( factor > 1.0 ) factor = 1.0;

    up = ( up * (1-factor) ) + ( g_app->m_camera->GetUp() * factor );     
    up = g_upVector;
    front = ( front * (1-factor) ) + ( g_app->m_camera->GetRight() * factor );

    up.Normalise();
    front.Normalise();

    //
    // Render flag

    if( !m_blitzkriegFlag )
    {
        m_blitzkriegFlag = new Flag();
        m_blitzkriegFlag->SetOrientation( m_front, g_upVector );
        m_blitzkriegFlag->SetSize( 50 );
        m_blitzkriegFlag->Initialise();
    }

    Vector3 flagPos = m_pos + up * 50;
    if( m_blitzkriegOwnership < 100 )
    {
        flagPos -= up * ( 100 - m_blitzkriegOwnership );
    }

    int teamId = m_id.GetTeamId();
    if( teamId == 255 ) teamId = m_blitzkriegCaptureTeam;

    char *filename = NULL;

    switch( m_blitzkriegUpOrDown )
    {
        case -1:    filename = "icons/flag_down.bmp";   break;
        case 0:     filename = "icons/flag.bmp";        break;
        case 1:     filename = "icons/flag_up.bmp";     break;
    }

    if( IsBlitzkriegZoneLocked() )
    {
        filename = "icons/flag_locked.bmp";
    }


    m_blitzkriegFlag->SetPosition( flagPos );    
    m_blitzkriegFlag->SetOrientation( front, up );
    m_blitzkriegFlag->SetSize( size );    
    m_blitzkriegFlag->Render( teamId, filename );

    glColor4f( 1.0, 1.0, 1.0, 1.0 );
    glLineWidth( 5.0 );

    glBegin( GL_LINES );
        glVertex3dv( (m_pos).GetData() );
        glVertex3dv( (m_pos + up * 50 + up * size * 1.5).GetData() );
    glEnd();

    //RenderSphere( m_pos, 20, RGBAColour(255,255,255,255) );
}


bool MultiwiniaZone::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool MultiwiniaZone::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool MultiwiniaZone::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          double _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    if( g_app->m_editing )
    {
        return Building::DoesRayHit( _rayStart, _rayDir, _rayLen, _pos, _norm );
    }
    else
    {
        return false;
    }
}


int MultiwiniaZone::GetBuildingLink()
{
    if(m_blitzkriegLinks.Size() )
    {
        return m_blitzkriegLinks[0];
    }
    
    return  -1;
}

bool MultiwiniaZone::IsBlitzkriegBaseZone()
{
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( m_id.GetUniqueId() == MultiwiniaZone::s_blitzkriegBaseZone[i] ) return true;
    }

    return false;
}


bool MultiwiniaZone::IsBlitzkriegZoneLocked()
{
    // New rules:
    // (to work with 2/3/4 player)

    //
    // If we are un-owned, and a base flag, we are unlocked

    if( m_id.GetTeamId() == 255 && IsBlitzkriegBaseZone() )
    {
        return false;
    }

    if( m_blitzkriegOwnership > 0.0 && m_blitzkriegOwnership < 100.0 )
    {
        // never lock if we are part way captured
        return false;
    }

    //
    // Otherwise
    // We are only locked if all our adjacent flags are owned by the same team as us, or are not owned by anyone
    
    bool locked = true;

    for( int i = 0; i < m_blitzkriegLinks.Size(); ++i )
    {
        MultiwiniaZone *link = (MultiwiniaZone *)g_app->m_location->GetBuilding( m_blitzkriegLinks[i] );
        if( link && 
            link->m_type == TypeMultiwiniaZone )
        {
            if( link->m_id.GetTeamId() != 255 &&
                link->m_id.GetTeamId() != m_id.GetTeamId() )
            {
                locked = false;
                break;
            }   
        }
    }

    return locked;
}

int MultiwiniaZone::GetNumZones(int _teamId)
{
    int num = 0;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b->m_type == Building::TypeMultiwiniaZone &&
                b->m_id.GetTeamId() == _teamId )
            {
                num++;
            }
        }
    }

    return num;
}

void MultiwiniaZone::SetBuildingLink( int _buildingId )
{
    //m_blitzkriegLinks.Empty();
    for( int i = 0; i < m_blitzkriegLinks.Size(); ++i )
    {
        if( m_blitzkriegLinks[i] == _buildingId ) return;
    }
    m_blitzkriegLinks.PutData(_buildingId);
}


void MultiwiniaZone::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_size = atof( _in->GetNextToken() );

    while( _in->TokenAvailable() )
    {
        m_blitzkriegLinks.PutData( atoi( _in->GetNextToken() ) );
    }
}


void MultiwiniaZone::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-6.2f", m_size );
    
    for( int i = 0; i < m_blitzkriegLinks.Size(); ++i )
    {
        _out->printf( "   %d", m_blitzkriegLinks[i] );
    }
}


void MultiwiniaZone::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "CaptureStatue" );
}

