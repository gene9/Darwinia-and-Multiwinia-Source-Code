#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/text_stream_readers.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/language_table.h"

#include "worldobject/trunkport.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "team.h"
#include "location.h"
#include "global_world.h"
#include "particle_system.h"
#include "main.h"
#include "renderer.h"


TrunkPort::TrunkPort()
:   Building(),
    m_targetLocationId(-1),
    m_openTimer(0.0f),
    m_heightMap(NULL),
    m_heightMapSize(TRUNKPORT_HEIGHTMAP_MAXSIZE)
{
    m_type = TypeTrunkPort;
    SetShape( g_app->m_resource->GetShape( "trunkport.shp" ) );

    m_destination1 = m_shape->m_rootFragment->LookupMarker( "MarkerDestination1" );
    m_destination2 = m_shape->m_rootFragment->LookupMarker( "MarkerDestination2" );
}


void TrunkPort::SetDetail( int _detail )
{
    m_heightMapSize = int( TRUNKPORT_HEIGHTMAP_MAXSIZE / (float) _detail );

    //
    // Pre-Generate our height map

    if( m_heightMap ) delete m_heightMap;
    m_heightMap = new Vector3[ m_heightMapSize * m_heightMapSize ];
    memset( m_heightMap, 0, m_heightMapSize * m_heightMapSize * sizeof(Vector3) );

    ShapeMarker *marker = m_shape->m_rootFragment->LookupMarker( "MarkerSurface" );
    DarwiniaDebugAssert( marker );
    
    Matrix34 transform( m_front, g_upVector, m_pos );
    Vector3 worldPos = marker->GetWorldMatrix( transform ).pos;

    float size = 90.0f;
    Vector3 up = g_upVector * size;
    Vector3 right = ( m_front ^ g_upVector ).Normalise() * size;

    
    for( int x = 0; x < m_heightMapSize; ++x )
    {
        for( int z = 0; z < m_heightMapSize; ++z )
        {
            float fractionX = (float) x / (float) (m_heightMapSize-1);
            float fractionZ = (float) z / (float) (m_heightMapSize-1);

            fractionX -= 0.5f;
            fractionZ -= 0.5f;
            
            Vector3 basePos = worldPos;
            basePos += right * fractionX;
            basePos += up * fractionZ;
    
            //basePos += right * 0.02f;
            //basePos += up * 0.02f;
            
            //basePos += right * 0.05f;
            //basePos += up * 0.05f;
            
            m_heightMap[z*m_heightMapSize+x] = basePos;        
        }
    }
}


bool TrunkPort::Advance()
{
    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );    
    if( gb && gb->m_online && m_openTimer == 0.0f)
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


void TrunkPort::Render( float predictionTime )
{
    Building::Render( predictionTime );

    char caption[256];

    char *locationName = g_app->m_globalWorld->GetLocationNameTranslated( m_targetLocationId );
    if( locationName ) 
    {
        strcpy( caption, locationName );
    }
    else
    {
        sprintf( caption, "[%s]", LANGUAGEPHRASE("location_unknown") );
    }
    
    START_PROFILE( g_app->m_profiler, "RenderDestination" );

    float fontSize = 70.0f / strlen(caption);
    fontSize = min( fontSize, 10.0f );

    Matrix34 portMat( m_front, g_upVector, m_pos );
    
    Matrix34 destMat = m_destination1->GetWorldMatrix(portMat);
    glColor4f( 0.9f, 0.8f, 0.8f, 1.0f );
    g_gameFont.DrawText3D( destMat.pos, destMat.f, destMat.u, fontSize, "%s", caption );
    g_gameFont.SetRenderShadow(true);
    destMat.pos += destMat.f * 0.1f;
    destMat.pos += ( destMat.f ^ destMat.u ) * 0.2f;
    destMat.pos += destMat.u * 0.2f;
    glColor4f( 0.9f, 0.8f, 0.8f, 0.0f );
    g_gameFont.DrawText3D( destMat.pos, destMat.f, destMat.u, fontSize, "%s", caption );

    g_gameFont.SetRenderShadow(false);
    glColor4f( 0.9f, 0.8f, 0.8f, 1.0f );
    destMat = m_destination2->GetWorldMatrix(portMat);
    g_gameFont.DrawText3D( destMat.pos, destMat.f, destMat.u, fontSize, "%s", caption );
    g_gameFont.SetRenderShadow(true);
    destMat.pos += destMat.f * 0.1f;
    destMat.pos += ( destMat.f ^ destMat.u ) * 0.2f;
    destMat.pos += destMat.u * 0.2f;
    glColor4f( 0.9f, 0.8f, 0.8f, 0.0f );
    g_gameFont.DrawText3D( destMat.pos, destMat.f, destMat.u, fontSize, "%s", caption );
    
    g_gameFont.SetRenderShadow(false);

    END_PROFILE( g_app->m_profiler, "RenderDestination" );    
}


bool TrunkPort::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_centrePos;

    return true;
}


void TrunkPort::RenderAlphas( float predictionTime )
{
    Building::RenderAlphas( predictionTime );

    if( m_openTimer > 0.0f )
    {
        ShapeMarker *marker = m_shape->m_rootFragment->LookupMarker( "MarkerSurface" );
        DarwiniaDebugAssert( marker );
    
        Matrix34 transform( m_front, g_upVector, m_pos );
        Vector3 markerPos = marker->GetWorldMatrix( transform ).pos;
        float maxDistance = 40.0f;

        float timeOpen = GetHighResTime() - m_openTimer;
        float timeScale = ( 5 - timeOpen );
        if( timeScale < 1.0f ) timeScale = 1.0f;
        
        //
        // Calculate our Dif map based on some nice sine curves

        START_PROFILE( g_app->m_profiler, "Advance Heightmap" );

        Vector3 difMap[TRUNKPORT_HEIGHTMAP_MAXSIZE][TRUNKPORT_HEIGHTMAP_MAXSIZE];
    
        for( int x = 0; x < m_heightMapSize; ++x )
        {
            for( int z = 0; z < m_heightMapSize; ++z )
            {
                float centreDif = (m_heightMap[z*m_heightMapSize+x] - markerPos).Mag();
                float fractionOut = centreDif / maxDistance;
                if( fractionOut > 1.0f ) fractionOut = 1.0f;

                float wave1 = cosf(centreDif * 0.15f);
                float wave2 = cosf(centreDif * 0.05f);
            
                Vector3 thisDif = m_front * sinf(g_gameTime * 2) * wave1 * (1.0f-fractionOut) * 15 * timeScale;
                thisDif += m_front * sinf(g_gameTime * 2.5) * wave2 * (1.0f-fractionOut) * 15 * timeScale;
                thisDif += g_upVector * cosf(g_gameTime) * wave1 * timeScale * 10 * (1.0f-fractionOut);
                difMap[x][z] = thisDif;
            }        
        }

        END_PROFILE( g_app->m_profiler, "Advance Heightmap" );

        //
        // Render our height map with the dif map added on

        START_PROFILE( g_app->m_profiler, "Render Heightmap" );

        glDisable       ( GL_CULL_FACE );
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDepthMask     ( false );

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laserfence.bmp" ) );

        float alphaValue = timeOpen;
        if( alphaValue > 0.7f ) alphaValue = 0.7f;
        glColor4f       ( 0.8f, 0.8f, 1.0f, alphaValue );

        for( int x = 0; x < m_heightMapSize-1; ++x )
        {
            for( int z = 0; z < m_heightMapSize-1; ++z )
            {
                Vector3 thisPos1 = m_heightMap[z*m_heightMapSize+x] + difMap[x][z];
                Vector3 thisPos2 = m_heightMap[z*m_heightMapSize+x+1] + difMap[x+1][z];
                Vector3 thisPos3 = m_heightMap[(z+1)*m_heightMapSize+x+1] + difMap[x+1][z+1];
                Vector3 thisPos4 = m_heightMap[(z+1)*m_heightMapSize+x] + difMap[x][z+1];
            
                float fractionX = (float) x / (float) m_heightMapSize;
                float fractionZ = (float) z / (float) m_heightMapSize;
                float width = 1.0f / m_heightMapSize;
            
                glBegin( GL_QUADS );
                    glTexCoord2f( fractionX, fractionZ );               glVertex3fv( thisPos1.GetData() );
                    glTexCoord2f( fractionX+width, fractionZ );         glVertex3fv( thisPos2.GetData() );
                    glTexCoord2f( fractionX+width, fractionZ+width );   glVertex3fv( thisPos3.GetData() );
                    glTexCoord2f( fractionX, fractionZ+width );         glVertex3fv( thisPos4.GetData() );
                glEnd();
            }
        }
        
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
        glColor4f       ( 1.0f, 1.0f, 1.0f, 1.0f );

        for( int x = 0; x < m_heightMapSize-1; ++x )
        {
            for( int z = 0; z < m_heightMapSize-1; ++z )
            {
                Vector3 thisPos1 = m_heightMap[z*m_heightMapSize+x] + difMap[x][z];
                Vector3 thisPos2 = m_heightMap[z*m_heightMapSize+x+1] + difMap[x+1][z];
                Vector3 thisPos3 = m_heightMap[(z+1)*m_heightMapSize+x+1] + difMap[x+1][z+1];
                Vector3 thisPos4 = m_heightMap[(z+1)*m_heightMapSize+x] + difMap[x][z+1];
            
                float fractionX = (float) x / (float) m_heightMapSize;
                float fractionZ = (float) z / (float) m_heightMapSize;
                float width = 1.0f / m_heightMapSize;
            
                glBegin( GL_QUADS );
                    glTexCoord2f( fractionX, fractionZ );               glVertex3fv( thisPos1.GetData() );
                    glTexCoord2f( fractionX+width, fractionZ );         glVertex3fv( thisPos2.GetData() );
                    glTexCoord2f( fractionX+width, fractionZ+width );   glVertex3fv( thisPos3.GetData() );
                    glTexCoord2f( fractionX, fractionZ+width );         glVertex3fv( thisPos4.GetData() );
                glEnd();
            }
        }

        glDisable       ( GL_TEXTURE_2D );

        glDepthMask     ( true );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );
        glEnable        ( GL_CULL_FACE );

        END_PROFILE( g_app->m_profiler, "Render Heightmap" );

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

void TrunkPort::Write ( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_targetLocationId );
}

