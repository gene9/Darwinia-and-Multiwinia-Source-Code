#include "lib/universal_include.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/math_utils.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/hi_res_time.h"
#include "lib/preferences.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "location.h"
#include "entity_grid.h"
#include "team.h"
#include "location_editor.h"
#include "global_world.h"
#include "main.h"
#include "level_file.h"
#include "multiwinia.h"
#include "multiwiniahelp.h"

#include "worldobject/spawnpoint.h"
#include "worldobject/darwinian.h"
#include "worldobject/virii.h"

int SpawnPoint::s_numSpawnPoints[NUM_TEAMS+1];

SpawnBuildingSpirit::SpawnBuildingSpirit()
:   m_targetBuildingId(-1),
    m_currentProgress(0.0)
{
}

SpawnBuildingLink::SpawnBuildingLink()
:   m_targetBuildingId(-1)
{
}

SpawnBuildingLink::~SpawnBuildingLink()
{
	m_spirits.EmptyAndDelete();
}

SpawnBuilding::SpawnBuilding()
:   Building(),
    m_spiritLink(NULL),
    m_visibilityRadius(0.0)
{
}

SpawnBuilding::~SpawnBuilding()
{
	m_links.EmptyAndDelete();
	//SAFE_DELETE(m_spiritLink); probably not necessary
}

void SpawnBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );

    SpawnBuilding *spawn = (SpawnBuilding *) _template;
    for( int i = 0; i < spawn->m_links.Size(); ++i )
    {   
        SpawnBuildingLink *link = spawn->m_links[i];
        SetBuildingLink( link->m_targetBuildingId );
    }
}


bool SpawnBuilding::IsInView()
{
    if( m_visibilityRadius == 0.0 || g_app->m_editing )
    {
        if( m_links.Size() == 0 )
        {
            m_visibilityRadius = m_radius;
            m_visibilityMidpoint = m_centrePos;
            return Building::IsInView();
        }
        else
        {
            m_visibilityMidpoint = m_centrePos;
            int numLinks = 1;
            m_visibilityRadius = 0.0;

            // Find mid point

            for( int i = 0; i < m_links.Size(); ++i )
            {
                SpawnBuildingLink *link = m_links[i];
                SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
                if( building )
                {
                    m_visibilityMidpoint += building->m_centrePos;
                    ++numLinks;
                }
            }

            m_visibilityMidpoint /= (double) numLinks;

            // Find radius

            for( int i = 0; i < m_links.Size(); ++i )
            {
                SpawnBuildingLink *link = m_links[i];
                SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
                if( building )
                {
                    double distance = ( building->m_centrePos - m_visibilityMidpoint ).Mag();
                    distance += building->m_radius / 2.0;
                    m_visibilityRadius = max( m_visibilityRadius, distance );
                }
            }
        }
    }

    // Determine visibility

    return g_app->m_camera->SphereInViewFrustum( m_visibilityMidpoint, m_visibilityRadius );

}


void SpawnBuilding::RenderSpirit( Vector3 const &_pos, int teamId )
{
    Vector3 pos = _pos;

    int innerAlpha = 255;
    int outerAlpha = 150;
    int spiritInnerSize = 2;
    int spiritOuterSize = 6;
 
    RGBAColour colour(150,50,25,innerAlpha);
    if( g_app->m_gameMode == App::GameModeMultiwinia )
    {
        if( teamId >= 0 && teamId < NUM_TEAMS )
        {
            Team *team = g_app->m_location->m_teams[teamId];
            colour = team->m_colour;
            colour.a = innerAlpha;
        }
        else
        {
            colour.Set( 120, 120, 120, innerAlpha );
        }
    }

    float size = spiritInnerSize;
    glColor4ubv(colour.GetData());            
    
    //glBegin( GL_QUADS );
        glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    //glEnd();    

    colour.Set(150,50,25,outerAlpha);
    if( g_app->m_gameMode == App::GameModeMultiwinia )
    {
        if( teamId >= 0 && teamId < NUM_TEAMS )
        {
            Team *team = g_app->m_location->m_teams[teamId];
            colour = team->m_colour;
            colour.a = outerAlpha;
        }
        else
        {
            colour.Set( 120, 120, 120, outerAlpha );
        }
    }

    size = spiritOuterSize;
    glColor4ubv(colour.GetData());            
        
    //glBegin( GL_QUADS );
        glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    //glEnd();    
}


bool SpawnBuilding::IsSpawnBuilding ( int _type )
{
    return ( _type == TypeSpawnPoint ||
             _type == TypeSpawnLink ||
             _type == TypeSpawnPointMaster );            
}


void SpawnBuilding::RenderAlphas( double _predictionTime )
{
    Vector3 ourPos = GetSpiritLink();
    
    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );

    glEnable    ( GL_BLEND );

    if( buildingDetail == 1 )
    {
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glEnable        ( GL_TEXTURE_2D );
    }

    glDisable   ( GL_CULL_FACE );
    glDepthMask ( false );


    for( int i = 0; i < m_links.Size(); ++i )
    {
        SpawnBuildingLink *link = m_links[i];
        SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
        if( building && IsSpawnBuilding(building->m_type) )
        {
            Vector3 theirPos = building->GetSpiritLink();
    
            Vector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
            Vector3 ourPosRight = camToOurPos ^ ( theirPos - ourPos );

            Vector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
            Vector3 theirPosRight = camToTheirPos ^ ( theirPos - ourPos );
            
            double size = 1.0;

            theirPosRight.SetLength( size );
            ourPosRight.SetLength( size );
                       
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
            glColor4f   ( 1.0f, 1.0f, 1.0f, 0.0f );

            glBegin( GL_QUADS );
                glTexCoord2f(0.0f, 0);      glVertex3dv( (ourPos - ourPosRight * 3).GetData() );
                glTexCoord2f(0.0f, 1);      glVertex3dv( (ourPos + ourPosRight * 3).GetData() );
                glTexCoord2f(1.0f, 1);      glVertex3dv( (theirPos + theirPosRight * 3).GetData() );
                glTexCoord2f(1.0f, 0);      glVertex3dv( (theirPos - theirPosRight * 3).GetData() );
            glEnd();

            glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
            glColor4f   ( 0.9f, 0.9f, 0.5f, 1.0f );

            glBegin( GL_QUADS );
                glTexCoord2f(0.1f, 0);      glVertex3dv( (ourPos - ourPosRight).GetData() );
                glTexCoord2f(0.1f, 1);      glVertex3dv( (ourPos + ourPosRight).GetData() );
                glTexCoord2f(0.9f, 1);      glVertex3dv( (theirPos + theirPosRight).GetData() );
                glTexCoord2f(0.9f, 0);      glVertex3dv( (theirPos - theirPosRight).GetData() );   

                glTexCoord2f(0.1f, 0);      glVertex3dv( (ourPos - ourPosRight).GetData() );
                glTexCoord2f(0.1f, 1);      glVertex3dv( (ourPos + ourPosRight).GetData() );
                glTexCoord2f(0.9f, 1);      glVertex3dv( (theirPos + theirPosRight).GetData() );
                glTexCoord2f(0.9f, 0);      glVertex3dv( (theirPos - theirPosRight).GetData() );   
            glEnd();
        }
    }


    glDisable       ( GL_TEXTURE_2D );

    //
    // Render spirits in transit

    glBegin( GL_QUADS );

    for( int i = 0; i < m_links.Size(); ++i )
    {
        SpawnBuildingLink *link = m_links[i];
        SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
        if( building && IsSpawnBuilding(building->m_type) )
        {
            Vector3 theirPos = building->GetSpiritLink();

            for( int j = 0; j < link->m_spirits.Size(); ++j )
            {
                SpawnBuildingSpirit *spirit = link->m_spirits[j];                
                double predictedProgress = spirit->m_currentProgress + _predictionTime;
                if( predictedProgress >= 0.0 && predictedProgress <= 1.0 )
                {
                    Vector3 position = ourPos + ( theirPos - ourPos ) * predictedProgress;
                    int targetTeamId = 255;
                    Building *targetBuilding = g_app->m_location->GetBuilding( spirit->m_targetBuildingId );
                    if( targetBuilding ) targetTeamId = targetBuilding->m_id.GetTeamId();
                    RenderSpirit( position, targetTeamId );
                }
            }            
        }
    }

    glEnd();

    Building::RenderAlphas( _predictionTime );
}


void SpawnBuilding::SetBuildingLink( int _buildingId )
{
	/*Building *b = g_app->m_location->GetBuilding( _buildingId );
	if( !b ) return;

	if( b->m_type == Building::TypeSpawnLink ||
		b->m_type == Building::TypeSpawnPoint ||
		b->m_type == Building::TypeSpawnPointMaster )*/
	{
		bool linkFound = false;
		for( int i = 0; i < m_links.Size(); ++i )
		{
			if( m_links[i]->m_targetBuildingId == _buildingId )	
			{
				linkFound = true;
				break;
			}
		}

		if( !linkFound )
		{
			SpawnBuildingLink *link = new SpawnBuildingLink();
			link->m_targetBuildingId = _buildingId;
			m_links.PutData( link );
		}
	}
}


void SpawnBuilding::ClearLinks()
{
    m_links.EmptyAndDelete();
}


LList<int> *SpawnBuilding::ExploreLinks()
{
    LList<int> *result = new LList<int>();

    if( m_type == TypeSpawnPoint )
    {
        result->PutData( m_id.GetUniqueId() );
    }

    for( int i = 0; i < m_links.Size(); ++i )
    {
        SpawnBuildingLink *link = m_links[i];
        link->m_targets.Empty();

        if( link->m_targetBuildingId == m_id.GetUniqueId() )
        {
            delete result;
            char msg[512];
            sprintf( msg, "SpawnPoint %d Linked to itself", m_id.GetUniqueId() );
            AppReleaseAssert( false, msg );
            return NULL;
        }

        SpawnBuilding *target = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
        if( target && IsSpawnBuilding( target->m_type ) )
        {
            LList<int> *availableLinks = target->ExploreLinks();
            for( int j = 0; j < availableLinks->Size(); ++j )
            {
                int thisLink = availableLinks->GetData(j);
                result->PutData( thisLink );
                link->m_targets.PutData( thisLink );
            }
            delete availableLinks;
        }
    }

    return result;
}


Vector3 SpawnBuilding::GetSpiritLink()
{
    if( !m_spiritLink )
    {    
        if( !m_shape ) return g_zeroVector;

        const char spiritLinkName[] = "MarkerSpiritLink";
        m_spiritLink = m_shape->m_rootFragment->LookupMarker( spiritLinkName );    
        AppReleaseAssert( m_spiritLink, "SpawnBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", spiritLinkName, m_shape->m_name );
    }

    Matrix34 mat( m_front, g_upVector, m_pos );

    return m_spiritLink->GetWorldMatrix(mat).pos;
}


void SpawnBuilding::TriggerSpirit( SpawnBuildingSpirit *_spirit, double progress )
{
    int numAdds = 0;

    for( int i = 0; i < m_links.Size(); ++i )
    {
        SpawnBuildingLink *link = m_links[i];
        for( int j = 0; j < link->m_targets.Size(); ++j )
        {
            int thisLink = link->m_targets[j];
            if( link->m_targets[j] == _spirit->m_targetBuildingId )
            {
                link->m_spirits.PutData( _spirit );
                _spirit->m_currentProgress = progress;                
                ++numAdds;
                break;
            }
        }
    }

    if( numAdds == 0 )
    {
        // We have nowhere else to go
        // Or maybe we have arrived
        delete _spirit;
    }
}


bool SpawnBuilding::Advance()
{
    //
    // Advance all spirits in transit

    for( int i = 0; i < m_links.Size(); ++i )
    {
        SpawnBuildingLink *link = m_links[i];
        for( int j = 0; j < link->m_spirits.Size(); ++j )
        {
            SpawnBuildingSpirit *spirit = link->m_spirits[j];
            spirit->m_currentProgress += SERVER_ADVANCE_PERIOD;
            if( spirit->m_currentProgress >= 1.0 )
            {
                double progress = spirit->m_currentProgress - 1.0f;
                link->m_spirits.RemoveData(j);
                --j;
                SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
                if( building && IsSpawnBuilding(building->m_type))
                {
                    building->TriggerSpirit( spirit, progress );
                }
                else
                {
                    delete spirit;
                }
            }
        }
    }
  
//#ifdef TARGET_DEBUG
//    if( g_inputManager->controlEvent( ControlCameraLeft ) )
//    {
//        m_pos.x += 1.0f;
//    }
//#endif

    return Building::Advance();
}


void SpawnBuilding::Render( double _predictionTime )
{
    Building::Render( _predictionTime );
}


void SpawnBuilding::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    while( _in->TokenAvailable() )
    {
        int nextLink = atoi( _in->GetNextToken() );
        SetBuildingLink( nextLink );
    }
}


void SpawnBuilding::Write( TextWriter *_out )
{
    Building::Write( _out );

    for( int i = 0; i < m_links.Size(); ++i )
    {
        SpawnBuildingLink *link = m_links[i];
        _out->printf( "%-6d", link->m_targetBuildingId );
    }
}


// ============================================================================


SpawnLink::SpawnLink()
:   SpawnBuilding()
{
    m_type = TypeSpawnLink;

    SetShape( g_app->m_resource->GetShape("spawnlink.shp") );
}


// ============================================================================

int MasterSpawnPoint::s_masterSpawnPointId = -1;


MasterSpawnPoint::MasterSpawnPoint()
:   SpawnBuilding(),
    m_exploreLinks(false)
{
    m_type = TypeSpawnPointMaster;

    SetShape( g_app->m_resource->GetShape("masterspawnpoint.shp") );    
}


MasterSpawnPoint *MasterSpawnPoint::GetMasterSpawnPoint()
{
    Building *building = g_app->m_location->GetBuilding( s_masterSpawnPointId );
    if( building && building->m_type == TypeSpawnPointMaster )
    {
        return (MasterSpawnPoint *) building;
    }

    return NULL;
}


void MasterSpawnPoint::RequestSpirit( int _targetBuildingId )
{
    SpawnBuildingSpirit *spirit = new SpawnBuildingSpirit;
    spirit->m_targetBuildingId = _targetBuildingId;
    
    TriggerSpirit( spirit );
}


bool MasterSpawnPoint::Advance()
{
    s_masterSpawnPointId = m_id.GetUniqueId();

    //
    // Explore our links first time through

    if( !m_exploreLinks )
    {
        m_exploreLinks = true;
        double startTime = GetHighResTime();
        LList<int> *availableLinks = ExploreLinks();
        delete availableLinks;
        double timeTaken = GetHighResTime() - startTime;
        AppDebugOut( "Time to Explore all Spawn Point links : %d ms\n", int(timeTaken*1000.0) );
    }
    

    if( !g_app->Multiplayer() )
    {
        //
        // Accelerate spirits to heaven

        if( m_isGlobal )
        {
            for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
            {
                if( g_app->m_location->m_spirits.ValidIndex(i) )
                {
                    Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
                    if( s->m_state == Spirit::StateBirth || s->m_state == Spirit::StateFloating )
                    {
                        s->SkipStage();
                    }
                }
            }
        }
        
        //
        // Have the red guys been wiped out?

        if( g_app->m_location->m_teams[1]->m_teamType != TeamTypeUnused )
        {
            int numRed = g_app->m_location->m_teams[1]->m_others.NumUsed();
            if( numRed < 10 )
            {
                GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
                if( gb ) gb->m_online = true;
            }
        }
    }


    return SpawnBuilding::Advance();
}


void MasterSpawnPoint::Render( double _predictionTime )
{
    if( m_isGlobal || g_app->m_editing )
    {
        SpawnBuilding::Render( _predictionTime );
    }
}


void MasterSpawnPoint::RenderAlphas ( double _predictionTime )
{
    if( m_isGlobal || g_app->m_editing )
    {
        SpawnBuilding::RenderAlphas( _predictionTime );
    }
}



void MasterSpawnPoint::GetObjectiveCounter( UnicodeString& _dest)
{
    static wchar_t result[256];

    if( g_app->m_location->m_teams[1]->m_teamType != TeamTypeUnused )
    {
        int numRed = g_app->m_location->m_teams[1]->m_others.NumUsed();    
		swprintf( result, sizeof(result)/sizeof(wchar_t),
				  L"%ls : %d", LANGUAGEPHRASE("objective_redpopulation").m_unicodestring, numRed );
    }
    else
    {
        swprintf( result, sizeof(result)/sizeof(wchar_t),
				  L"%ls", LANGUAGEPHRASE("objective_redpopulation").m_unicodestring );
    }

    _dest = UnicodeString( result );
}


// ============================================================================

SpawnPoint::SpawnPoint()
:   SpawnBuilding(),
    m_spawnTimer(0.0),
    m_evaluateTimer(0.0),
    m_numFriendsNearby(0),
    m_populationLock(-1),
    m_spawnCounter(0),
    m_activationTimer(30.0),
    m_numSpawned(0)
{
    m_type = Building::TypeSpawnPoint;
    

    if( g_app->Multiplayer() )
    {
        SetShape( g_app->m_resource->GetShape("spawnpointmultiwinia.shp") );
    }
    else
    {
        SetShape( g_app->m_resource->GetShape("spawnpoint.shp") );
    }

    const char doorMarkerName[] = "MarkerDoor";
    m_doorMarker = m_shape->m_rootFragment->LookupMarker( doorMarkerName );
    AppReleaseAssert( m_doorMarker, "SpawnPoint: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", doorMarkerName, m_shape->m_name );

    m_evaluateTimer = syncfrand(2.0);
    m_spawnTimer = 2.0 + syncfrand(2.0);
}

void SpawnPoint::Initialise  ( Building *_template )
{
    SpawnBuilding::Initialise( _template );

    int teamId = m_id.GetTeamId() + 1;
    if( teamId > NUM_TEAMS+1 ) teamId = 0;
    s_numSpawnPoints[teamId]++;
}

bool SpawnPoint::PopulationLocked()
{
    //
    // If a population lock has been specified for the map, use that.
    // Otherwise, fall back on pop-lock buildings.

    if( g_app->Multiplayer() )
    {
        if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial2 )
        {
            if( !g_app->m_multiwiniaHelp->m_tutorial2AICanSpawn )
            {
                if( m_numSpawned >= 100 ) 
                {
                    return true;
                }

                if( m_id.GetTeamId() != 255 && m_id.GetTeamId() != 0 )
                {
                    return true;
                }

                if( m_id.GetTeamId() == 0 && g_app->m_location->m_teams[0]->m_others.NumUsed() > 200 ) 
                {
                    return true;
                }
            }

        }

        if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault )
        {
            if( g_app->m_location->m_levelFile->m_defenderPopulationCap != -1 )
            {
                if( m_id.GetTeamId() == 255 ) return false;

                int defenderCap = g_app->m_location->m_levelFile->m_defenderPopulationCap;
                Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
                int currentPop = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_others.NumUsed() - Virii::s_viriiPopulation[m_id.GetTeamId()];

                if( g_app->m_location->IsDefending( m_id.GetTeamId() ) )
                {
                    return( currentPop >= defenderCap );
                }
                else
                {
                    int numDefenders = g_app->m_location->m_levelFile->m_numPlayers - g_app->m_location->GetNumAttackers();
                    int cap = g_app->m_location->m_levelFile->m_populationCap;
                    cap -= (numDefenders * defenderCap);
                    cap /= g_app->m_location->GetNumAttackers();
                    return( currentPop >= cap );
                }
            }
        }

        if( g_app->m_location->m_levelFile->m_populationCap != -1 )
        {
            if( m_id.GetTeamId() == 255 ) return false;
            int numTeams = g_app->m_location->m_levelFile->m_numPlayers;
            int maxPopPerTeam = g_app->m_location->m_levelFile->m_populationCap / numTeams;
            Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
            int currentPop = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_others.NumUsed() - Virii::s_viriiPopulation[m_id.GetTeamId()];
            return( currentPop >= maxPopPerTeam );
        }
    }

    //
    // If we haven't yet looked up a nearby Pop lock,
    // do so now

    if( m_populationLock == -1 )
    {
        m_populationLock = -2;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && building->m_type == TypeSpawnPopulationLock )
                {
                    SpawnPopulationLock *lock = (SpawnPopulationLock *) building;
                    double distance = ( building->m_pos - m_pos ).Mag();
                    if( distance < lock->m_searchRadius )
                    {
                        m_populationLock = lock->m_id.GetUniqueId();
                        break;
                    }
                }
            }
        }
    }


    //
    // If we are inside a Population Lock, query it now

    if( m_populationLock > 0 )
    {
        SpawnPopulationLock *lock = (SpawnPopulationLock *) g_app->m_location->GetBuilding( m_populationLock );
        if( lock && m_id.GetTeamId() != 255 &&
            lock->m_teamCount[ m_id.GetTeamId() ] >= lock->m_maxPopulation )
        {
            return true;
        }
    }


    return false;
}


void SpawnPoint::RecalculateOwnership()
{
    int teamCount[NUM_TEAMS];
    memset( teamCount, 0, NUM_TEAMS*sizeof(int));

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
            teamCount[id.GetTeamId()] ++;
        }
    }

    int winningTeam = 255;
    bool teamChanged = false;
    bool multipleTeams = false;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( teamCount[i] > 0 && 
            winningTeam == 255 )
        {
            winningTeam = i;
        }
        else
        {
            if( winningTeam != 255 && 
                 teamCount[i] > 2 && 
                 teamCount[i] > teamCount[winningTeam] )
            {
                winningTeam = i;
            }
        }

        if( winningTeam != 255 && 
             teamCount[i] > 0 )
        {
            // no partial control allowed - if the spawn point is contested, switch off
            //winningTeam = 255;
            multipleTeams = true;
        }

        if( winningTeam != 255 && 
            teamCount[winningTeam] < 3 )
        {
            winningTeam = 255;
        }
    }

    if( multipleTeams )
    {
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( teamCount[i] > 0 && !g_app->m_location->IsFriend( i, winningTeam ) )
            {
                winningTeam = 255;
                break;
            }
        }
    }

    if( winningTeam != m_id.GetTeamId() )
    {
        teamChanged = true;

        int currentTeam = m_id.GetTeamId();
        int winner = winningTeam;

        if( currentTeam == 255 ) currentTeam = -1;
        if( winningTeam == 255 ) winner = -1;

        s_numSpawnPoints[currentTeam+1]--;
        s_numSpawnPoints[winner+1]++;
    }

    if( m_id.GetTeamId() != winningTeam )
    {
        m_activationTimer = 30.0;
    }

    SetTeamId(winningTeam);


    if( teamChanged )
    {
        if( winningTeam == 255 )    g_app->m_soundSystem->TriggerBuildingEvent( this, "TurnOff" );
        else                        g_app->m_soundSystem->TriggerBuildingEvent( this, "TurnOn" );
    }

    /*if( winningTeam == -1 )
    {
        SetTeamId(255);
    }
    else
    {
        SetTeamId(winningTeam);
    }*/
}


void SpawnPoint::TriggerSpirit( SpawnBuildingSpirit *_spirit, double progress )
{
    if( m_id.GetUniqueId() == _spirit->m_targetBuildingId )
    {
        // This spirit has arrived at its destination
        if( m_id.GetTeamId() != 255 )
        {
            SpawnDarwinian();
        }

        delete _spirit;
    }
    else
    {
        SpawnBuilding::TriggerSpirit( _spirit, progress );    
    }
}

void SpawnPoint::SpawnDarwinian()
{
    if( m_id.GetTeamId() == 255 )
    {
        return;
    }

    Matrix34 mat( m_front, m_up, m_pos );
    Matrix34 doorMat = m_doorMarker->GetWorldMatrix(mat);

    Vector3 exitPos = doorMat.pos;
    Vector3 exitVel = g_zeroVector;

    if( g_app->Multiplayer() )
    {
        //exitPos.y += 20;
        exitVel = doorMat.f;
        exitVel.x += syncsfrand(1.0);
        exitVel.y += syncsfrand(1.0);
        exitVel.z += syncsfrand(1.0);
        exitVel *= 10;
    }
    
    WorldObjectId spawned = g_app->m_location->SpawnEntities( exitPos, m_id.GetTeamId(), -1, Entity::TypeDarwinian, 1, exitVel, 0.0 );
    Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntitySafe( spawned, Entity::TypeDarwinian );
    if( darwinian )
    {
        darwinian->m_onGround = false;
    }
}

bool SpawnPoint::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_centrePos;
    return true;
}


bool SpawnPoint::Advance()
{    
    //
    // Re-evalulate our situation

    m_evaluateTimer -= SERVER_ADVANCE_PERIOD;
    if( m_evaluateTimer <= 0.0 )
    {
        START_PROFILE( "Evaluate" );
        
        RecalculateOwnership();
        m_evaluateTimer = 2.0 + syncfrand(1.0);

        END_PROFILE(  "Evaluate" );
    }

    if( m_activationTimer > 0.0 && m_id.GetTeamId() != 255)
    {
        m_activationTimer -= SERVER_ADVANCE_PERIOD;
        if( s_numSpawnPoints[m_id.GetTeamId() + 1] == 1 )   // this is our first (or last) spawn point, so dont bother with the initial delay
        {
            m_activationTimer = 0.0;
        }

        m_activationTimer = max( 0.0, m_activationTimer );

        if( !g_app->Multiplayer() )
        {
            m_activationTimer = 0.0;
        }

        if( g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionInstantSpawnPointCapture] > 0 )
        {
            m_activationTimer = 0.0;
        }
    }


    //
    // Time to request more spirits for our Darwinians?

    if( m_activationTimer <= 0.0 )
    {
        START_PROFILE( "SpawnDarwinians" );
        if( m_id.GetTeamId() != 255 && !PopulationLocked() )
        {        
            m_spawnTimer -= SERVER_ADVANCE_PERIOD;
            if( m_spawnTimer <= 0.0 || g_app->m_location->m_spawnManiaTimer > 0.0 )
            {            
                MasterSpawnPoint *masterSpawn = MasterSpawnPoint::GetMasterSpawnPoint();
                if( masterSpawn ) 
                {
                    masterSpawn->RequestSpirit( m_id.GetUniqueId() );
                    if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial2 )
                    {
                        m_numSpawned++;
                    }
                }
                
                double fractionOccupied = (double) GetNumPortsOccupied() / (double) GetNumPorts();
    				
			    if( g_app->Multiplayer() )
                {
                    // Make things go a bit quicker in multiplayer
                    m_spawnTimer = 0.2;

                    ++m_spawnCounter;
                    if( m_spawnCounter >= (int)(20 * CalculateHandicap(m_id.GetTeamId()))  )
                    {
                        m_spawnCounter = 0;
                        m_spawnTimer = 10 + syncfrand(5);
                    }
                }
                else
                {
                    m_spawnTimer = 2.0 + 5.0 * (1.0 - fractionOccupied);
                    // Reds spawn a bit quicker
                    if (m_id.GetTeamId() == 1)  m_spawnTimer -= 1.0 * (g_app->m_difficultyLevel / 10.0);
                    else                        m_spawnTimer -= 0.5 * (g_app->m_difficultyLevel / 10.0);
                    m_spawnTimer += syncfrand(1.0);
                }
            }
        }
        END_PROFILE(  "SpawnDarwinians" );
    }

    return SpawnBuilding::Advance();
}


void SpawnPoint::Render( double _predictionTime )
{
    SpawnBuilding::Render( _predictionTime );
}


void SpawnPoint::RenderAlphas( double _predictionTime )
{
    SpawnBuilding::RenderAlphas( _predictionTime );

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
        
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );
    glBegin         ( GL_QUADS );
    
    double timeIndex = g_gameTimer.GetAccurateTime();
    timeIndex += m_id.GetUniqueId() * 10.0;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 20;
    if( buildingDetail == 2 ) maxBlobs = 10;
    if( buildingDetail == 3 ) maxBlobs = 3;

    float alpha = GetNumPortsOccupied() / (float) GetNumPorts();


    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = m_centrePos;
        pos += Vector3(0,25,0);
        pos.x += iv_sin(timeIndex+i) * i * 0.7;
        pos.y += iv_cos(timeIndex+i) * iv_sin(i*10) * 12;
        pos.z += iv_cos(timeIndex+i) * i * 0.7;

        double size = 10.0 + iv_sin(timeIndex+i*10) * 10.0;
        size = max( size, 5.0 );
        
        if( m_id.GetTeamId() != 255 )
        {            
            if( g_app->Multiplayer() )
            {
                Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
                RGBAColour colour = team->m_colour;
                colour = (( m_activationTimer / 30.0) * RGBAColour(100,100,100,200)) + ( team->m_colour * (1-( m_activationTimer / 30.0)));
                colour.a *= alpha;
                glColor4ubv( colour.GetData() );
            }
            else
            {
                glColor4f( 0.6, 0.2, 0.1, alpha);
            }

            glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
        }
    }
           
    glEnd           ();
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );
}


void SpawnPoint::RenderPorts()
{    
    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glBegin         ( GL_QUADS );

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        Vector3 portPos;
        Vector3 portFront;
        GetPortPosition( i, portPos, portFront );

        Vector3 portUp = g_upVector;
        Matrix34 mat( portFront, portUp, portPos );
         
        //
        // Render the status light

        double size = 6.0;
        Vector3 camR = g_app->m_camera->GetRight() * size;
        Vector3 camU = g_app->m_camera->GetUp() * size;

        Vector3 statusPos = s_controlPadStatus->GetWorldMatrix( mat ).pos;
        statusPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(statusPos.x, statusPos.z);
        statusPos.y += 5.0;
        
        WorldObjectId occupantId = GetPortOccupant(i);
        if( !occupantId.IsValid() ) 
        {
            glColor4ub( 150, 150, 150, 255 );
        }
        else
        {
            RGBAColour teamColour = g_app->m_location->m_teams[occupantId.GetTeamId()]->m_colour;
            glColor4ubv( teamColour.GetData() );
        }

        glTexCoord2i( 0, 0 );           glVertex3dv( (statusPos - camR - camU).GetData() );
        glTexCoord2i( 1, 0 );           glVertex3dv( (statusPos + camR - camU).GetData() );
        glTexCoord2i( 1, 1 );           glVertex3dv( (statusPos + camR + camU).GetData() );
        glTexCoord2i( 0, 1 );           glVertex3dv( (statusPos - camR + camU).GetData() );
    }

    glEnd           ();
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );

}

double SpawnPoint::CalculateHandicap(int _teamId)
{
    if( g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionNoHandicap] > 0 ) return 1.0;
    if( _teamId == 255 ) return 0.0;
    if( s_numSpawnPoints[0] > 0 ) return 1.0;  // There are still some unoccupied spawn points on the level - no handicap

    int handicap = g_app->m_multiwinia->GetGameOption("Handicap");
    if( handicap == 0 || handicap == -1) return 1.0;

    int totalSpawnPoints = 0;
    int numTeams = 0;

    int total = 0;

    for( int i = 1; i < NUM_TEAMS+1; ++i )
    {
        if( g_app->m_location->m_teams[i-1]->m_teamType == TeamTypeUnused ) continue;

        total += s_numSpawnPoints[i];

        if( i == _teamId+1 ) continue;

        totalSpawnPoints += s_numSpawnPoints[i];
        numTeams++;
    }
    
    if( numTeams == 0 ) return 1.0;        // dont bother with handicap if the player is by himself for whatever reason

    double average = totalSpawnPoints / numTeams;

    if( average > (double)(s_numSpawnPoints[_teamId+1] * 1.5) )
    {
        double factor = 1 - ( (s_numSpawnPoints[_teamId+1] - 1) / total );
        return ( 1.0 +  ((double)(handicap/100.0) * factor) );
    }
    
    return 1.0;
}


void SpawnPoint::ListSoundEvents( LList<char *> *_list )
{
    SpawnBuilding::ListSoundEvents( _list );

    _list->PutData( "TurnOn" );
    _list->PutData( "TurnOff" );
}


// ============================================================================

double SpawnPopulationLock::s_overpopulationTimer = 0.0;
int SpawnPopulationLock::s_overpopulation = 0;


SpawnPopulationLock::SpawnPopulationLock()
:   Building(),
    m_searchRadius(500.0),
    m_maxPopulation(100),
    m_originalMaxPopulation(100),
    m_recountTeamId(-1)
{
    m_type = Building::TypeSpawnPopulationLock;

    memset( m_teamCount, 0, NUM_TEAMS*sizeof(int) );

    m_recountTimer = syncfrand(10.0);
    m_recountTeamId = 10;
}


void SpawnPopulationLock::Initialise( Building *_template )
{
    Building::Initialise( _template );

    SpawnPopulationLock *lock = (SpawnPopulationLock *) _template;
    m_searchRadius = lock->m_searchRadius;
    m_maxPopulation = lock->m_maxPopulation;

    m_originalMaxPopulation = m_maxPopulation;
}


bool SpawnPopulationLock::Advance()
{
    //
    // If this is Multiwinia and the map has specified a population cap,
    // Dont bother updating and use that population cap instead.
    
    if( g_app->Multiplayer() &&
        g_app->m_location->m_levelFile->m_populationCap != -1 )
    {
        return false;
    }


    //
    // Every once in a while, have a look at the global over-population rate
    // And reduce all local maxPopulations accordingly
    // This only really applies to Green darwinians, as the player can arrange
    // it so an unlimited army gathers on a single island

    if( GetNetworkTime() > s_overpopulationTimer )
    {
        s_overpopulationTimer = GetNetworkTime() + 1.0;

        int totalOverpopulation = 0;
        
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && building->m_type == TypeSpawnPopulationLock )
                {
                    SpawnPopulationLock *lock = (SpawnPopulationLock *) building;
                    if( lock->m_teamCount[0] > lock->m_originalMaxPopulation )
                    {
                        totalOverpopulation += (lock->m_teamCount[0] - lock->m_originalMaxPopulation);
                    }
                }
            }
        }

        s_overpopulation = totalOverpopulation / 2;
    }


    m_maxPopulation = m_originalMaxPopulation - s_overpopulation;
    m_maxPopulation = max( m_maxPopulation, 0 );

    
    //
    // Recount the number of entities nearby

    m_recountTimer -= SERVER_ADVANCE_PERIOD;
    if( m_recountTimer <= 0.0 )
    {
        m_recountTimer = 10.0 + syncsfrand(2.0);
        m_recountTeamId = 10;
    }


    int potentialTeamId = (int) m_recountTimer;
    if( potentialTeamId < m_recountTeamId &&
        potentialTeamId < NUM_TEAMS )
    {
        m_recountTeamId = potentialTeamId;
        m_teamCount[potentialTeamId] = g_app->m_location->m_entityGrid->GetNumFriends( m_pos.x, m_pos.z, m_searchRadius, m_recountTeamId );
    }

    return Building::Advance();
}


void SpawnPopulationLock::Render( double _predictionTime )
{
}


void SpawnPopulationLock::RenderAlphas( double _predictionTime )
{
    if( g_app->m_editing )
    {
        RenderSphere( m_pos, 30.0, RGBAColour(255,255,255,255) );
    
        glDisable( GL_TEXTURE_2D );
        glColor4f( 1.0, 1.0, 1.0, 1.0 );

        Vector3 pos = m_pos + Vector3(0,250,0);
    
        g_editorFont.DrawText3DCentre( pos+Vector3(0,80,0), 10, "SpawnPopulationLock" );
        g_editorFont.DrawText3DCentre( pos+Vector3(0,70,0), 10, "OriginalMaxPopulation = %d", m_originalMaxPopulation );
        g_editorFont.DrawText3DCentre( pos+Vector3(0,60,0), 10, "CurrentMaxPopulation = %d", m_maxPopulation );
        g_editorFont.DrawText3DCentre( pos+Vector3(0,50,0), 10, "Red = %d", m_teamCount[1] );
        g_editorFont.DrawText3DCentre( pos+Vector3(0,40,0), 10, "Green = %d", m_teamCount[0] );
        
        if( m_teamCount[0] > m_originalMaxPopulation )
        {
            g_editorFont.DrawText3DCentre( pos+Vector3(0,30,0), 10, "Green Overpopulated by %d", m_teamCount[0]-m_originalMaxPopulation );
        }

        if( m_teamCount[1] > m_originalMaxPopulation )
        {
            g_editorFont.DrawText3DCentre( pos+Vector3(0,20,0), 10, "Red Overpopulated by %d", m_teamCount[1]-m_originalMaxPopulation );
        }

#ifdef LOCATION_EDITOR
        if( g_app->m_editing &&
            g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
            g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
        {
            RenderSphere( m_pos, m_searchRadius, RGBAColour(255,255,255,100) );
        }
#endif
    }
}

void SpawnPopulationLock::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_searchRadius = iv_atof( _in->GetNextToken() );
    m_maxPopulation = atoi( _in->GetNextToken() );
}


void SpawnPopulationLock::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8.2f %-6d", m_searchRadius, m_maxPopulation );
}


bool SpawnPopulationLock::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool SpawnPopulationLock::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}

void SpawnPopulationLock::Reset()
{
    SpawnPopulationLock::s_overpopulation = 0;
    SpawnPopulationLock::s_overpopulationTimer = 0.0f;
}