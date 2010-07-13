#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/debug_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/language_table.h"

#include "worldobject/trunkport.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/virii.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "team.h"
#include "location.h"
#include "global_world.h"
#include "particle_system.h"
#include "main.h"
#include "renderer.h"
#include "level_file.h"
#include "multiwinia.h"


TrunkPort::TrunkPort()
:   Building(),
    m_targetLocationId(-1),
    m_openTimer(0.0),
    m_heightMap(NULL),
    m_heightMapSize(TRUNKPORT_HEIGHTMAP_MAXSIZE),
    m_populationLock(-1)
{
    m_type = TypeTrunkPort;
    if( g_app->IsSinglePlayer() )
    {
        SetShape( g_app->m_resource->GetShape( "trunkport.shp" ) );
    }
    else
    {
        SetShape( g_app->m_resource->GetShape( "trunkport_mp.shp" ) );
    }

    const char destination1Name[] = "MarkerDestination1";
    m_destination1 = m_shape->m_rootFragment->LookupMarker( destination1Name );
    AppReleaseAssert( m_destination1, "TrunkPort: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", destination1Name, m_shape->m_name );

    const char destination2Name[] = "MarkerDestination2";
    m_destination2 = m_shape->m_rootFragment->LookupMarker( destination2Name );
    AppReleaseAssert( m_destination2, "TrunkPort: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", destination2Name, m_shape->m_name );
}

TrunkPort::~TrunkPort()
{
	delete [] m_heightMap; m_heightMap = NULL;
}

void TrunkPort::SetDetail( int _detail )
{
    m_heightMapSize = int( TRUNKPORT_HEIGHTMAP_MAXSIZE / (double) _detail );

    //
    // Pre-Generate our height map

    if( m_heightMap ) delete [] m_heightMap;
    m_heightMap = new Vector3[ m_heightMapSize * m_heightMapSize ];
    memset( m_heightMap, 0, m_heightMapSize * m_heightMapSize * sizeof(Vector3) );

    const char markerName[] = "MarkerSurface";
    ShapeMarker *marker = m_shape->m_rootFragment->LookupMarker( markerName );
    AppReleaseAssert( marker, "TrunkPort: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", markerName, m_shape->m_name );
    
    Matrix34 transform( m_front, g_upVector, m_pos );
    Vector3 worldPos = marker->GetWorldMatrix( transform ).pos;

    double size = 90.0;
    Vector3 up = g_upVector * size;
    Vector3 right = ( m_front ^ g_upVector ).Normalise() * size;

    
    for( int x = 0; x < m_heightMapSize; ++x )
    {
        for( int z = 0; z < m_heightMapSize; ++z )
        {
            double fractionX = (double) x / (double) (m_heightMapSize-1);
            double fractionZ = (double) z / (double) (m_heightMapSize-1);

            fractionX -= 0.5;
            fractionZ -= 0.5;
            
            Vector3 basePos = worldPos;
            basePos += right * fractionX;
            basePos += up * fractionZ;
    
            //basePos += right * 0.02;
            //basePos += up * 0.02;
            
            //basePos += right * 0.05;
            //basePos += up * 0.05;
            
            m_heightMap[z*m_heightMapSize+x] = basePos;        
        }
    }
}


bool TrunkPort::Advance()
{
    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );    
    if( gb && gb->m_online && m_openTimer == 0.0)
    {
        m_openTimer = GetHighResTime();
        g_app->m_soundSystem->TriggerBuildingEvent( this, "PowerUp" );
    }

    return Building::Advance();
}

void TrunkPort::Initialise ( Building *_template )
{
    Building::Initialise( _template );

    m_targetLocationId = ((TrunkPort *) _template)->m_targetLocationId;
}


void TrunkPort::Render( double predictionTime )
{
    Building::Render( predictionTime );

	if( g_app->IsSinglePlayer() )
	{
		UnicodeString locationName;
		g_app->m_globalWorld->GetLocationNameTranslated( m_targetLocationId, locationName );
		if( locationName.Length() <= 0 ) 
		{
			locationName = LANGUAGEPHRASE("location_unknown");
		}
	    
		START_PROFILE( "RenderDestination" );

		float fontSize = 70.0f / locationName.Length();
		fontSize = min( fontSize, 10.0f );

		Matrix34 portMat( m_front, g_upVector, m_pos );
	    
		Matrix34 destMat = m_destination1->GetWorldMatrix(portMat);
		glColor4f( 0.9f, 0.8f, 0.8f, 1.0f );
		g_gameFont.DrawText3D( destMat.pos, destMat.f, destMat.u, fontSize, locationName );
		g_gameFont.SetRenderShadow(true);
		destMat.pos += destMat.f * 0.1f;
		destMat.pos += ( destMat.f ^ destMat.u ) * 0.2f;
		destMat.pos += destMat.u * 0.2f;
		glColor4f( 0.9f, 0.8f, 0.8f, 0.0f );
		g_gameFont.DrawText3D( destMat.pos, destMat.f, destMat.u, fontSize, locationName );

		g_gameFont.SetRenderShadow(false);
		glColor4f( 0.9f, 0.8f, 0.8f, 1.0f );
		destMat = m_destination2->GetWorldMatrix(portMat);
		g_gameFont.DrawText3D( destMat.pos, destMat.f, destMat.u, fontSize, locationName );
		g_gameFont.SetRenderShadow(true);
		destMat.pos += destMat.f * 0.1f;
		destMat.pos += ( destMat.f ^ destMat.u ) * 0.2f;
		destMat.pos += destMat.u * 0.2f;
		glColor4f( 0.9f, 0.8f, 0.8f, 0.0f );
		g_gameFont.DrawText3D( destMat.pos, destMat.f, destMat.u, fontSize, locationName );
	    
		g_gameFont.SetRenderShadow(false);

		END_PROFILE(  "RenderDestination" );    
	}
}


bool TrunkPort::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_centrePos;

    return true;
}


void TrunkPort::RenderAlphas( double predictionTime )
{
    Building::RenderAlphas( predictionTime );

    if( m_openTimer > 0.0 || g_app->Multiplayer() )
    {
        const char markerName[] = "MarkerSurface";
        ShapeMarker *marker = m_shape->m_rootFragment->LookupMarker( markerName );
        AppReleaseAssert( marker, "TrunkPort: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", markerName, m_shape->m_name );
    
        Matrix34 transform( m_front, g_upVector, m_pos );
        Vector3 markerPos = marker->GetWorldMatrix( transform ).pos;
        double maxDistance = 40.0;

        double timeOpen = GetHighResTime() - m_openTimer;
        double timeScale = ( 5 - timeOpen );
        if( timeScale < 1.0 ) timeScale = 1.0;
        
        //
        // Calculate our Dif map based on some nice sine curves

        START_PROFILE( "Advance Heightmap" );

        Vector3 difMap[TRUNKPORT_HEIGHTMAP_MAXSIZE][TRUNKPORT_HEIGHTMAP_MAXSIZE];
    
        for( int x = 0; x < m_heightMapSize; ++x )
        {
            for( int z = 0; z < m_heightMapSize; ++z )
            {
                double centreDif = (m_heightMap[z*m_heightMapSize+x] - markerPos).Mag();
                double fractionOut = centreDif / maxDistance;
                if( fractionOut > 1.0 ) fractionOut = 1.0;

                double wave1 = iv_cos(centreDif * 0.15);
                double wave2 = iv_cos(centreDif * 0.05);
            
                Vector3 thisDif = m_front * iv_sin(g_gameTime * 2) * wave1 * (1.0-fractionOut) * 15 * timeScale;
                thisDif += m_front * iv_sin(g_gameTime * 2.5) * wave2 * (1.0-fractionOut) * 15 * timeScale;
                thisDif += g_upVector * iv_cos(g_gameTime) * wave1 * timeScale * 10 * (1.0-fractionOut);
                difMap[x][z] = thisDif;
            }        
        }

        END_PROFILE(  "Advance Heightmap" );

        //
        // Render our height map with the dif map added on

        START_PROFILE( "Render Heightmap" );

        glDisable       ( GL_CULL_FACE );
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDepthMask     ( false );

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laserfence.bmp" ) );

        double alphaValue = timeOpen;
        if( alphaValue > 0.7 ) alphaValue = 0.7;
        
        if( g_app->Multiplayer() && m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS )
        {
            Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
            RGBAColour colour = team->m_colour;
            colour.a *= 0.7;

            glColor4ubv     ( colour.GetData() );
        }
        else
        {
            glColor4f       ( 0.8, 0.8, 1.0, alphaValue );
        }

        glBegin( GL_QUADS );

        for( int x = 0; x < m_heightMapSize-1; ++x )
        {
            for( int z = 0; z < m_heightMapSize-1; ++z )
            {
                Vector3 thisPos1 = m_heightMap[z*m_heightMapSize+x] + difMap[x][z];
                Vector3 thisPos2 = m_heightMap[z*m_heightMapSize+x+1] + difMap[x+1][z];
                Vector3 thisPos3 = m_heightMap[(z+1)*m_heightMapSize+x+1] + difMap[x+1][z+1];
                Vector3 thisPos4 = m_heightMap[(z+1)*m_heightMapSize+x] + difMap[x][z+1];
            
                double fractionX = (double) x / (double) m_heightMapSize;
                double fractionZ = (double) z / (double) m_heightMapSize;
                double width = 1.0 / m_heightMapSize;
            
                glTexCoord2f( fractionX, fractionZ );               glVertex3dv( thisPos1.GetData() );
                glTexCoord2f( fractionX+width, fractionZ );         glVertex3dv( thisPos2.GetData() );
                glTexCoord2f( fractionX+width, fractionZ+width );   glVertex3dv( thisPos3.GetData() );
                glTexCoord2f( fractionX, fractionZ+width );         glVertex3dv( thisPos4.GetData() );
            }
        }

        glEnd();
        
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
        
        if( !g_app->Multiplayer() )
        {
            glColor4f       ( 1.0, 1.0, 1.0, 1.0 );
        }

        glBegin( GL_QUADS );

        for( int x = 0; x < m_heightMapSize-1; ++x )
        {
            for( int z = 0; z < m_heightMapSize-1; ++z )
            {
                Vector3 thisPos1 = m_heightMap[z*m_heightMapSize+x] + difMap[x][z];
                Vector3 thisPos2 = m_heightMap[z*m_heightMapSize+x+1] + difMap[x+1][z];
                Vector3 thisPos3 = m_heightMap[(z+1)*m_heightMapSize+x+1] + difMap[x+1][z+1];
                Vector3 thisPos4 = m_heightMap[(z+1)*m_heightMapSize+x] + difMap[x][z+1];
            
                double fractionX = (double) x / (double) m_heightMapSize;
                double fractionZ = (double) z / (double) m_heightMapSize;
                double width = 1.0 / m_heightMapSize;
            
                glTexCoord2f( fractionX, fractionZ );               glVertex3dv( thisPos1.GetData() );
                glTexCoord2f( fractionX+width, fractionZ );         glVertex3dv( thisPos2.GetData() );
                glTexCoord2f( fractionX+width, fractionZ+width );   glVertex3dv( thisPos3.GetData() );
                glTexCoord2f( fractionX, fractionZ+width );         glVertex3dv( thisPos4.GetData() );
            }
        }

        glEnd();

        glDisable       ( GL_TEXTURE_2D );

        glDepthMask     ( true );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );
        glEnable        ( GL_CULL_FACE );

        END_PROFILE(  "Render Heightmap" );

    }
}

void TrunkPort::ReprogramComplete()
{
    GlobalLocation *location = g_app->m_globalWorld->GetLocation( m_targetLocationId );
    if( location )
    {
        location->m_available = true;

        // Look for a "receiver" trunk port and set that to the same state
        for( int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i )
        {
            GlobalBuilding *building = g_app->m_globalWorld->m_buildings[i];
            if( building->m_type == Building::TypeTrunkPort &&
                building->m_locationId == m_targetLocationId &&
                building->m_link == g_app->m_locationId )
            {
                building->m_online = true;         
            }
        }
    }
    
    Building::ReprogramComplete();
}


bool TrunkPort::PopulationLocked()
{
    //
    // If a population lock has been specified for the map, use that.
    // Otherwise, fall back on pop-lock buildings.

    if( g_app->Multiplayer() )
    {
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


void TrunkPort::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "PowerUp" );
}


void TrunkPort::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    if( _in->TokenAvailable() )
    {
        m_targetLocationId = atoi( _in->GetNextToken() );
    }
}

void TrunkPort::Write ( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_targetLocationId );
}


