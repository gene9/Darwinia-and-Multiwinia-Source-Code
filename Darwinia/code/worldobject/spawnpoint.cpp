#include "lib/universal_include.h"
#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/math_utils.h"
#include "lib/text_stream_readers.h"
#include "lib/hi_res_time.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#include "app.h"
#include "camera.h"
#include "location.h"
#include "level_file.h"
#include "entity_grid.h"
#include "team.h"
#include "location_editor.h"
#include "global_world.h"
#include "main.h"

#include "worldobject/spawnpoint.h"
#include "worldobject/darwinian.h"


SpawnBuilding::SpawnBuilding()
:   Building(),
    m_spiritLink(NULL),
    m_visibilityRadius(0.0f)
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
    if( m_visibilityRadius == 0.0f || g_app->m_editing )
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
            m_visibilityRadius = 0.0f;

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

            m_visibilityMidpoint /= (float) numLinks;

            // Find radius

            for( int i = 0; i < m_links.Size(); ++i )
            {
                SpawnBuildingLink *link = m_links[i];
                SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
                if( building )
                {
                    float distance = ( building->m_centrePos - m_visibilityMidpoint ).Mag();
                    distance += building->m_radius / 2.0f;
                    m_visibilityRadius = max( m_visibilityRadius, distance );
                }
            }
        }
    }

    // Determine visibility

    return g_app->m_camera->SphereInViewFrustum( m_visibilityMidpoint, m_visibilityRadius );

}


void SpawnBuilding::RenderSpirit( Vector3 const &_pos )
{
    Vector3 pos = _pos;

    int innerAlpha = 255;
    int outerAlpha = 150;
    int spiritInnerSize = 2;
    int spiritOuterSize = 6;

    float size = spiritInnerSize;
    glColor4ub(150, 50, 25, innerAlpha );

    glBegin( GL_QUADS );
        glVertex3fv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3fv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();

    size = spiritOuterSize;
    glColor4ub(150, 50, 25, outerAlpha );

    glBegin( GL_QUADS );
        glVertex3fv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3fv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();
}


void SpawnBuilding::RenderAlphas( float _predictionTime )
{
    Vector3 ourPos = GetSpiritLink();

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );

    for( int i = 0; i < m_links.Size(); ++i )
    {
        SpawnBuildingLink *link = m_links[i];
        SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
        if( building )
        {
            Vector3 theirPos = building->GetSpiritLink();

            Vector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
            Vector3 ourPosRight = camToOurPos ^ ( theirPos - ourPos );

            Vector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
            Vector3 theirPosRight = camToTheirPos ^ ( theirPos - ourPos );

            glDisable   ( GL_CULL_FACE );
            glDepthMask ( false );
            glColor4f   ( 0.9f, 0.9f, 0.5f, 1.0f );

            float size = 0.5f;

            if( buildingDetail == 1 )
            {
                glEnable    ( GL_BLEND );
                glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

                glEnable        ( GL_TEXTURE_2D );
                glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
                glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

                size = 1.0f;
            }

            theirPosRight.SetLength( size );
            ourPosRight.SetLength( size );

            glBegin( GL_QUADS );
                glTexCoord2f(0.1f, 0);      glVertex3fv( (ourPos - ourPosRight).GetData() );
                glTexCoord2f(0.1f, 1);      glVertex3fv( (ourPos + ourPosRight).GetData() );
                glTexCoord2f(0.9f, 1);      glVertex3fv( (theirPos + theirPosRight).GetData() );
                glTexCoord2f(0.9f, 0);      glVertex3fv( (theirPos - theirPosRight).GetData() );
            glEnd();

            glDisable       ( GL_TEXTURE_2D );


            //
            // Render spirits in transit

            for( int j = 0; j < link->m_spirits.Size(); ++j )
            {
                SpawnBuildingSpirit *spirit = link->m_spirits[j];
                float predictedProgress = spirit->m_currentProgress + _predictionTime;
                if( predictedProgress >= 0.0f && predictedProgress <= 1.0f )
                {
                    Vector3 position = ourPos + ( theirPos - ourPos ) * predictedProgress;
                    RenderSpirit( position );
                }
            }
        }
    }

    Building::RenderAlphas( _predictionTime );
}


void SpawnBuilding::SetBuildingLink( int _buildingId )
{
	// Dont allow a spawn building to link to a spawn point master
	if ( g_app->m_location )
	{
		Building *building = g_app->m_location->GetBuilding(_buildingId);
		if ( building && building->m_type == Building::TypeSpawnPointMaster ) { return; }
	}

    SpawnBuildingLink *link = new SpawnBuildingLink();
    link->m_targetBuildingId = _buildingId;
    m_links.PutData( link );
}


void SpawnBuilding::ClearLinks()
{
    m_links.EmptyAndDelete();
}


LList<int> *SpawnBuilding::ExploreLinks()
{
    LList<int> *result = new LList<int>();

    if( m_type == TypeSpawnPoint || m_type == TypeSpawnPointRandom )
    {
        result->PutData( m_id.GetUniqueId() );
    }

    for( int i = 0; i < m_links.Size(); ++i )
    {
        SpawnBuildingLink *link = m_links[i];
        link->m_targets.Empty();

        SpawnBuilding *target = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
        if( target )
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
        m_spiritLink = m_shape->m_rootFragment->LookupMarker( "MarkerSpiritLink" );
        DarwiniaDebugAssert( m_spiritLink );
    }

    Matrix34 mat( m_front, g_upVector, m_pos );

    return m_spiritLink->GetWorldMatrix(mat).pos;
}


void SpawnBuilding::TriggerSpirit( SpawnBuildingSpirit *_spirit )
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
                _spirit->m_currentProgress = 0.0f;
                ++numAdds;
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
            if( spirit->m_currentProgress >= 1.0f )
            {
                link->m_spirits.RemoveData(j);
                --j;
                SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( link->m_targetBuildingId );
                if( building ) building->TriggerSpirit( spirit );
            }
        }
    }

    return Building::Advance();
}


void SpawnBuilding::Render( float _predictionTime )
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


void SpawnBuilding::Write( FileWriter *_out )
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
    SpawnBuildingSpirit *spirit = new SpawnBuildingSpirit();
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
        float startTime = GetHighResTime();
        ExploreLinks();
        float timeTaken = GetHighResTime() - startTime;
        DebugOut( "Time to Explore all Spawn Point links : %d ms\n", int(timeTaken*1000.0f) );
    }


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

    if( g_app->m_location->m_teams[1].m_teamType != Team::TeamTypeUnused )
    {
        int numRed = g_app->m_location->m_teams[1].m_others.NumUsed();
        if( numRed < 10 )
        {
            GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
            if( gb ) gb->m_online = true;
        }
    }

    return SpawnBuilding::Advance();
}


void MasterSpawnPoint::Render( float _predictionTime )
{
    if( m_isGlobal || g_app->m_editing )
    {
        SpawnBuilding::Render( _predictionTime );
    }
}


void MasterSpawnPoint::RenderAlphas ( float _predictionTime )
{
    if( m_isGlobal || g_app->m_editing )
    {
        SpawnBuilding::RenderAlphas( _predictionTime );
    }
}



char *MasterSpawnPoint::GetObjectiveCounter()
{
    static char result[256];

    if( g_app->m_location->m_teams[1].m_teamType != Team::TeamTypeUnused )
    {
        int numRed = g_app->m_location->m_teams[1].m_others.NumUsed();
        sprintf( result, "%s : %d", LANGUAGEPHRASE("objective_redpopulation"), numRed );
    }
    else
    {
        sprintf( result, "%s", LANGUAGEPHRASE("objective_redpopulation") );
    }

    return result;
}


// ============================================================================

SpawnPoint::SpawnPoint()
:   SpawnBuilding(),
    m_spawnTimer(0.0f),
    m_evaluateTimer(0.0f),
    m_numFriendsNearby(0),
    m_populationLock(-1),
	m_teamSpawner(false),
	m_glowFader(4.0f),
	m_currentPort(0)
{
    m_type = Building::TypeSpawnPoint;

    SetShape( g_app->m_resource->GetShape("spawnpoint.shp") );
    m_doorMarker = m_shape->m_rootFragment->LookupMarker( "MarkerDoor" );

    m_evaluateTimer = syncfrand(2.0f);
    m_spawnTimer = 2.0f + syncfrand(2.0f);
}


bool SpawnPoint::PopulationLocked()
{
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
                    float distance = ( building->m_pos - m_pos ).Mag();
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
	bool teamPresent[NUM_TEAMS];
	for ( int i = 0; i < NUM_TEAMS; i++ )
	{
		teamCount[i] = 0;
		teamPresent[i] = false;
	}

	for ( int i = 0; i < GetNumPorts(); i++ )
	{
        WorldObjectId id = GetPortOccupant(i);
		if( id.IsValid() ) {
			teamPresent[id.GetTeamId()] = true;
		}
	}
	/*
    for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
			if ( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS && g_app->m_location->IsFriend(id.GetTeamId(),m_id.GetTeamId()) ) {
	            teamCount[m_id.GetTeamId()] ++;
			} else {
	            teamCount[id.GetTeamId()] ++;
			}
        }
    }
*/

	for ( int i = 0; i < GetNumPorts(); i++ )
	{
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
			int lowestFriendlyTeam = id.GetTeamId();
			for ( int j = 0; j < NUM_TEAMS; j++ )
			{
				if ( teamPresent[j] && j < lowestFriendlyTeam && g_app->m_location->IsFriend(id.GetTeamId(),j) ) { lowestFriendlyTeam = j; }
			}
			teamCount[lowestFriendlyTeam]++;
		}
	}

    int winningTeam = -1;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( teamCount[i] > 2 &&
            winningTeam == -1 )
        {
            winningTeam = i;
        }
        else if( winningTeam != -1 &&
                 teamCount[i] > 2 &&
                 teamCount[i] > teamCount[winningTeam] )
        {
            winningTeam = i;
        }
    }

    if( winningTeam == -1 )
    {
        SetTeamId(255);
    }
	else if ( !g_app->m_location->IsFriend(m_id.GetTeamId(),winningTeam) )
    {
        SetTeamId(winningTeam);
    }
}


void SpawnPoint::TriggerSpirit( SpawnBuildingSpirit *_spirit )
{
    if( m_id.GetTeamId() == 0 )
    {
        int b = 10;
    }

    if( m_id.GetUniqueId() == _spirit->m_targetBuildingId )
    {
        // This spirit has arrived at its destination
		SpawnDarwinian();
        delete _spirit;
    }
    else
    {
        SpawnBuilding::TriggerSpirit( _spirit );
    }
}


bool SpawnPoint::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_centrePos;
    return true;
}


bool SpawnPoint::Advance()
{

	m_glowFader -= SERVER_ADVANCE_PERIOD;
	if ( m_glowFader <= 0 ) {
		m_glowFader = 4;
		m_currentPort++;
		if ( m_currentPort >= GetNumPorts() ) { m_currentPort = 0; }
	}
	
	//
    // Re-evalulate our situation

    m_evaluateTimer -= SERVER_ADVANCE_PERIOD;
    if( m_evaluateTimer <= 0.0f )
    {
        START_PROFILE( g_app->m_profiler, "Evaluate" );

        RecalculateOwnership();
        m_evaluateTimer = 2.0f;

        END_PROFILE( g_app->m_profiler, "Evaluate" );
    }


    //
    // Time to request more spirits for our Darwinians?

    START_PROFILE( g_app->m_profiler, "SpawnDarwinians" );
    if( m_id.GetTeamId() != 255 && !PopulationLocked() )
    {
        m_spawnTimer -= SERVER_ADVANCE_PERIOD;
        if( m_spawnTimer <= 0.0f )
        {
            MasterSpawnPoint *masterSpawn = MasterSpawnPoint::GetMasterSpawnPoint();
            if( masterSpawn ) masterSpawn->RequestSpirit( m_id.GetUniqueId() );

            float fractionOccupied = (float) GetNumPortsOccupied() / (float) GetNumPorts();

            m_spawnTimer = 2.0f + 5.0f * (1.0f - fractionOccupied);

			// Reds spawn a bit quicker
			if (m_id.GetTeamId() == 1)
			  m_spawnTimer -= 1.0 * (g_app->m_difficultyLevel / 10.0);
			else
			  m_spawnTimer -= 0.5 * (g_app->m_difficultyLevel / 10.0);
        }
    }
    END_PROFILE( g_app->m_profiler, "SpawnDarwinians" );

    return SpawnBuilding::Advance();
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
    
	int teamId = m_id.GetTeamId();
	if ( m_teamSpawner )
	{
		teamId = 255;
		while ( !g_app->m_location->IsFriend(m_id.GetTeamId(),teamId) )
		{
			teamId = (int) floor(frand(NUM_TEAMS));
			if ( teamId < 0 || teamId > NUM_TEAMS ) { teamId = 255; }
		}
	}
	if ( teamId == 2 ) { // Instead of hardcoding, look up the team we need to use based on team flags
		for ( int i = 0; i < NUM_TEAMS; i++ ) {
			if ( g_app->m_location->m_levelFile->m_teamFlags[i] & TEAM_FLAG_PLAYER_SPAWN_TEAM ) { teamId = i; }
		}
	}

	if ( teamId < 0 || teamId >= NUM_TEAMS ) { return; }

    WorldObjectId spawned = g_app->m_location->SpawnEntities( exitPos, teamId, -1, Entity::TypeDarwinian, 1, exitVel, 0.0 );
    Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntitySafe( spawned, Entity::TypeDarwinian );
    if( darwinian )
    {
        darwinian->m_onGround = false;
    }
}


void SpawnPoint::Render( float _predictionTime )
{
    SpawnBuilding::Render( _predictionTime );
}


void SpawnPoint::RenderAlphas( float _predictionTime )
{

	int nextPort = m_currentPort + 1;
	if ( nextPort >= GetNumPorts() ) { nextPort = 0; }

	RGBAColour fromColour(153,51,25,255);
	RGBAColour toColour(153,51,25,255);

    WorldObjectId occupantId = GetPortOccupant(m_currentPort);
	if( occupantId.IsValid() ) {
        fromColour = g_app->m_location->m_teams[occupantId.GetTeamId()].m_colour;
    }
    occupantId = GetPortOccupant(nextPort);
	if( occupantId.IsValid() ) {
        toColour = g_app->m_location->m_teams[occupantId.GetTeamId()].m_colour;
    }

	RGBAColour glowColour = (fromColour * (m_glowFader/4)) + (toColour * ((4-m_glowFader)/4));

    SpawnBuilding::RenderAlphas( _predictionTime );

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();

    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );

    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10.0f;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 20;
    if( buildingDetail == 2 ) maxBlobs = 10;
    if( buildingDetail == 3 ) maxBlobs = 0;

    float alpha = GetNumPortsOccupied() / (float) GetNumPorts();

    for( int i = 0; i < maxBlobs; ++i )
    {
        Vector3 pos = m_centrePos;
        pos += Vector3(0,25,0);
        pos.x += sinf(timeIndex+i) * i * 0.7f;
        pos.y += cosf(timeIndex+i) * sinf(i*10) * 12;
        pos.z += cosf(timeIndex+i) * i * 0.7f;

        float size = 10.0f + sinf(timeIndex+i*10) * 10.0f;
        size = max( size, 5.0f );

        glBegin( GL_QUADS );
			glColor4ub(glowColour.r,glowColour.g,glowColour.b,(int) (alpha*255.0));
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }

    glDisable       ( GL_BLEND );
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

        float size = 6.0f;
        Vector3 camR = g_app->m_camera->GetRight() * size;
        Vector3 camU = g_app->m_camera->GetUp() * size;

        Vector3 statusPos = s_controlPadStatus->GetWorldMatrix( mat ).pos;
        statusPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(statusPos.x, statusPos.z);
        statusPos.y += 5.0f;

        WorldObjectId occupantId = GetPortOccupant(i);
        if( !occupantId.IsValid() )
        {
            glColor4ub( 150, 150, 150, 255 );
        }
        else
        {
            RGBAColour teamColour = g_app->m_location->m_teams[occupantId.GetTeamId()].m_colour;
            glColor4ubv( teamColour.GetData() );
        }

        glTexCoord2i( 0, 0 );           glVertex3fv( (statusPos - camR - camU).GetData() );
        glTexCoord2i( 1, 0 );           glVertex3fv( (statusPos + camR - camU).GetData() );
        glTexCoord2i( 1, 1 );           glVertex3fv( (statusPos + camR + camU).GetData() );
        glTexCoord2i( 0, 1 );           glVertex3fv( (statusPos - camR + camU).GetData() );
    }

    glEnd           ();
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );

}

SpawnPointRandom::SpawnPointRandom()
{
	SpawnPoint::SpawnPoint();
	m_teamSpawner = true;
    m_type = Building::TypeSpawnPointRandom;
}


// ============================================================================

float SpawnPopulationLock::s_overpopulationTimer = 0.0f;
int SpawnPopulationLock::s_overpopulation = 0;


SpawnPopulationLock::SpawnPopulationLock()
:   Building(),
    m_searchRadius(500.0f),
    m_maxPopulation(100),
    m_originalMaxPopulation(100),
    m_recountTeamId(-1)
{
    m_type = Building::TypeSpawnPopulationLock;

    memset( m_teamCount, 0, NUM_TEAMS*sizeof(int) );

    m_recountTimer = syncfrand(10.0f);
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
    // Every once in a while, have a look at the global over-population rate
    // And reduce all local maxPopulations accordingly
    // This only really applies to Green darwinians, as the player can arrange
    // it so an unlimited army gathers on a single island

    if( GetHighResTime() > s_overpopulationTimer )
    {
        s_overpopulationTimer = GetHighResTime() + 1.0f;

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
    if( m_recountTimer <= 0.0f )
    {
        m_recountTimer = 10.0f;
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


void SpawnPopulationLock::Render( float _predictionTime )
{
}


void SpawnPopulationLock::RenderAlphas( float _predictionTime )
{
    if( g_app->m_editing )
    {
        RenderSphere( m_pos, 30.0f, RGBAColour(255,255,255,255) );

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

    m_searchRadius = atof( _in->GetNextToken() );
    m_maxPopulation = atoi( _in->GetNextToken() );
}


void SpawnPopulationLock::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8.2f %-6d", m_searchRadius, m_maxPopulation );
}


bool SpawnPopulationLock::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    return false;
}


bool SpawnPopulationLock::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}

