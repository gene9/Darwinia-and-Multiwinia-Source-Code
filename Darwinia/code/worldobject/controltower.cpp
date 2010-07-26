#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/matrix34.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_stream_readers.h"
#include "lib/math_utils.h"
#include "lib/hi_res_time.h"

#include "network/clienttoserver.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "location.h"
#include "renderer.h"
#include "team.h"
#include "sepulveda.h"
#include "main.h"
#include "particle_system.h"
#include "global_world.h"

#include "worldobject/controltower.h"
#include "worldobject/trunkport.h"

Shape *ControlTower::s_dishShape = NULL;


ControlTower::ControlTower()
:   Building(),
    m_ownership(100.0f),
    m_controlBuildingId(-1),
    m_checkTargetTimer(0.0f)
{
    m_radius = 4.0f;
    m_type = TypeControlTower;

    SetShape( g_app->m_resource->GetShape( "controltower.shp" ) );
    m_lightPos = m_shape->m_rootFragment->LookupMarker("MarkerLightPos");
    m_dishPos = m_shape->m_rootFragment->LookupMarker("MarkerDishPos" );

    for( int i = 0; i < 3; ++i )
    {
        m_beingReprogrammed[i] = false;
        char markerName[64];

        sprintf( markerName, "MarkerReprogrammer%d", i );
        m_reprogrammer[i] = m_shape->m_rootFragment->LookupMarker(markerName);

        sprintf( markerName, "MarkerConsole%d", i );
        m_console[i] = m_shape->m_rootFragment->LookupMarker(markerName);
    }

    if( !s_dishShape )
    {
        s_dishShape = g_app->m_resource->GetShape( "controltowerdish.shp" );
    }
}

void ControlTower::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_controlBuildingId = ((ControlTower *) _template)->m_controlBuildingId;
}

bool ControlTower::Advance()
{
    //
    // Calculate our Dish matrix (once only, during first Advance)
    // Also look to see if our building is already captured

    if( m_dishMatrix == Matrix34() )
    {
        Building *targetBuilding = g_app->m_location->GetBuilding( m_controlBuildingId );
        if( targetBuilding)
        {
            Matrix34 mat(m_front, g_upVector, m_pos);
            Vector3 dishPos = m_dishPos->GetWorldMatrix( mat ).pos;
            Vector3 dishFront = (dishPos - targetBuilding->m_centrePos).Normalise();
            Vector3 dishRight = dishFront ^ g_upVector;
            Vector3 dishUp = dishRight ^ dishFront;
            m_dishMatrix = Matrix34( dishFront, dishUp, dishPos );

            if( targetBuilding->m_id.GetTeamId() == 255 )
            {
                m_ownership = 0.0f;
            }
            else
            {
                m_ownership = 100.0f;
            }

        }
    }


    //
    // If we are connected to a TrunkPort, somebody else may have opened that port from another map.
    // Every once in a while, check and turn on if it has happened

    m_checkTargetTimer -= SERVER_ADVANCE_PERIOD;

    if( m_checkTargetTimer <= 0.0f )
    {
        Building *building = g_app->m_location->GetBuilding( m_controlBuildingId );
        if( building &&
            building->m_type == TypeTrunkPort &&
            m_id.GetTeamId() != 2 )
        {
            TrunkPort *tp = (TrunkPort *) building;
            if( tp->m_openTimer > 0.0f )
            {
                 m_id.SetTeamId( 2 );
                 m_ownership = 100.0f;
            }
        }
        m_checkTargetTimer = 2.0f;
    }


    //
    // Spawn particles if we are being reprogrammed

    for( int i = 0; i < 3; ++i )
    {
        if( m_beingReprogrammed[i] )
        {
            Matrix34 rootMat(m_front, g_upVector, m_pos);
            Matrix34 worldMat = m_console[i]->GetWorldMatrix(rootMat);
            Vector3 particleVel = worldMat.pos - m_pos;
            particleVel += Vector3( sfrand() * 10.0f, sfrand() * 5.0f, sfrand() * 10.0f );
            g_app->m_particleSystem->CreateParticle( worldMat.pos, particleVel, Particle::TypeBlueSpark );
        }
    }

    return Building::Advance();
}

int ControlTower::GetAvailablePosition( Vector3 &_pos, Vector3 &_front )
{
    for( int i = 0; i < 3; ++i )
    {
        if( !m_beingReprogrammed[i] )
        {
            Matrix34 rootMat(m_front, g_upVector, m_pos);
            Matrix34 worldMat = m_reprogrammer[i]->GetWorldMatrix(rootMat);

            _pos = worldMat.pos;
            _front = worldMat.f;

            return i;
        }
    }

    return -1;
}


void ControlTower::GetConsolePosition( int _position, Vector3 &_pos )
{
    DarwiniaDebugAssert( _position >= 0 && _position < 3 );

    Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_console[_position]->GetWorldMatrix(rootMat);

    _pos = worldMat.pos;
}


void ControlTower::BeginReprogram( int _position )
{
    DarwiniaDebugAssert( !m_beingReprogrammed[_position] );
    m_beingReprogrammed[_position] = true;
}


bool ControlTower::Reprogram( int _teamId )
{
    //if( _teamId != m_id.GetTeamId() )
	if ( !g_app->m_location->IsFriend(_teamId, m_id.GetTeamId()) )
    {
        // Removing someone elses control
        m_ownership -= 0.1f;
        if( m_ownership <= 0.0f )
        {
            m_id.SetTeamId( _teamId );
            Building *targetBuilding = g_app->m_location->GetBuilding( m_controlBuildingId );
            if( targetBuilding && targetBuilding->m_id.GetTeamId() != m_id.GetTeamId() )
            {
                targetBuilding->SetTeamId( m_id.GetTeamId() );
            }
        }
    }
    else
    {
        // Increasing our own control
        if( m_ownership < 100.0f )
        {
            m_ownership += 0.1f;
            if( m_ownership > 100.0f ) m_ownership = 100.0f;

            Building *targetBuilding = g_app->m_location->GetBuilding( m_controlBuildingId );
            if( targetBuilding )
            {
                targetBuilding->Reprogram( m_ownership );

                if( m_ownership == 100.0f )
                {
                    g_app->m_soundSystem->TriggerBuildingEvent( this, "ReprogramComplete" );
			        //g_app->m_sepulveda->Say("building_captured");
                    targetBuilding->ReprogramComplete();
                    SetTeamId( _teamId );
				    g_app->m_globalWorld->m_research->GiveResearchPoints( GLOBALRESEARCH_POINTS_CONTROLTOWER );

                    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
                    if( gb )
                    {
                        gb->m_online = true;
                        g_app->m_globalWorld->EvaluateEvents();
                    }


                    g_app->SaveProfile( true, true );
                    return true;
                }
            }
        }
        else
        {
            return true;
        }
    }

    return false;
}


void ControlTower::EndReprogram( int _position )
{
    DarwiniaDebugAssert( m_beingReprogrammed[_position] );
    m_beingReprogrammed[_position] = false;
}


void ControlTower::Render( float _predictionTime )
{
    Building::Render( _predictionTime );

    //
    // Render our dish

    s_dishShape->Render( _predictionTime, m_dishMatrix );
}


bool ControlTower::IsInView()
{
    if( Building::IsInView() ) return true;

    //
    // Check against the tall thin control line to heaven

    Vector3 towerPos = m_pos;
    towerPos.y = g_app->m_camera->GetPos().y;
    return g_app->m_camera->PosInViewFrustum( towerPos );
}


void ControlTower::RenderAlphas ( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );


    //
    // Control lines will be bright when we are near a control tower
    // And dim when we are not
    // Recalculate our distance to the nearest control tower once per second

    static int s_lastRecalculation = 0.0f;
    static float s_distanceScale = 0.0f;
    static float s_desiredDistanceScale = 0.0f;

    if( (int) GetHighResTime() > s_lastRecalculation )
    {
        s_lastRecalculation = (int) GetHighResTime();

        float nearest = 99999.9f;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && (building->m_type == TypeControlTower || building->m_type == TypeControlStation) )
                {
                    float camDist = (building->m_pos - g_app->m_camera->GetPos()).Mag();
                    if( camDist < nearest ) nearest = camDist;
                }
            }
        }

        if   ( nearest < 200.0f )      s_desiredDistanceScale = 1.0f;
        else                           s_desiredDistanceScale = 0.1f;
    }

    if( s_desiredDistanceScale > s_distanceScale )
    {
        s_distanceScale = ( s_desiredDistanceScale * SERVER_ADVANCE_PERIOD * 0.1f ) +
                          ( s_distanceScale * (1.0f - SERVER_ADVANCE_PERIOD * 0.1f) );
    }
    else
    {
        s_distanceScale = ( s_desiredDistanceScale * SERVER_ADVANCE_PERIOD * 0.03f ) +
                          ( s_distanceScale * (1.0f - SERVER_ADVANCE_PERIOD * 0.03f) );
    }


    //
    // Pre-compute some positions and shit

	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_lightPos->GetWorldMatrix(rootMat);
	Vector3 lightPos = worldMat.pos;

    Vector3 camR = g_app->m_camera->GetRight();
    Vector3 camU = g_app->m_camera->GetUp();

    RGBAColour colour;
    if( m_id.GetTeamId() == 255 )
	{
		colour.Set( 128, 128, 128, 255 );
	}
	else
	{
		colour = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour;
	}


    //
    // Draw control line to heaven

    if( !g_app->m_editing )
    {
        Vector3 controlUp( 0, 50.0f + (m_id.GetUniqueId() % 50), 0 );

        glDisable       ( GL_CULL_FACE );
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glShadeModel    ( GL_SMOOTH );
        glDepthMask     ( false );

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
	    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

        float w = (lightPos  - g_app->m_camera->GetPos()).Mag() * 0.002f;
        w = max( 0.5f, w );

        for( int i = 0; i < 10; ++i )
        {
            Vector3 thisUp1 = Vector3(0,-20,0) + controlUp * (float) i;
            Vector3 thisUp2 = Vector3(0,-20,0) + controlUp * (float) (i+1);

            int alpha = 255 - 255 * (float) i / 10.0f;
            int alpha2 = 255 - 255 * (float) (i+1) / 10.0f;

            alpha *= fabs( sinf( g_gameTime * 2 + (float) i / 5.0f ) );
            alpha2 *= fabs( sinf( g_gameTime * 2 + (float) (i+1) / 5.0f ) );

            alpha *= s_distanceScale;
            alpha2 *= s_distanceScale;

            float y = (float) i / 10.0f;
            float h = 1.0f / 10.0f;

            glBegin( GL_QUADS );
                glColor4ub( colour.r, colour.g, colour.b, alpha );

                glTexCoord2f(y,0);      glVertex3fv( (lightPos - camR * w + thisUp1).GetData() );
                glTexCoord2f(y,1);      glVertex3fv( (lightPos + camR * w + thisUp1).GetData() );

                glColor4ub( colour.r, colour.g, colour.b, alpha2 );

                glTexCoord2f(y+h,1);    glVertex3fv( (lightPos + camR * w + thisUp2).GetData() );
                glTexCoord2f(y+h,0);    glVertex3fv( (lightPos - camR * w + thisUp2).GetData() );
            glEnd();
        }

        glDisable       ( GL_TEXTURE_2D );

        glDepthMask     ( true );
        glShadeModel    ( GL_FLAT );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );
        glEnable        ( GL_CULL_FACE );
    }


    //
    // Draw our signal flash

    int lastSeqId = g_app->m_clientToServer->m_lastValidSequenceIdFromServer;

    if( (m_id.GetTeamId() != 255 && (lastSeqId % 10)/2 == m_id.GetTeamId() ) ||
        m_beingReprogrammed[ lastSeqId % 3 ] ||
        g_app->m_editing )
    {

	    Matrix34 rootMat(m_front, g_upVector, m_pos);
        Matrix34 worldMat = m_lightPos->GetWorldMatrix(rootMat);
	    Vector3 lightPos = worldMat.pos;

        float signalSize = m_ownership / 5.0f;

        glColor4ub( colour.r, colour.g, colour.b, 255 );

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
	    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glDisable       ( GL_CULL_FACE );
        glDepthMask     ( false );

        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

        for( int i = 0; i < 10; ++i )
        {
            float size = signalSize * (float) i / 10.0f;
            glBegin( GL_QUADS );
                glTexCoord2f    ( 0.0f, 0.0f );             glVertex3fv     ( (lightPos - camR * size - camU * size).GetData() );
                glTexCoord2f    ( 1.0f, 0.0f );             glVertex3fv     ( (lightPos + camR * size - camU * size).GetData() );
                glTexCoord2f    ( 1.0f, 1.0f );             glVertex3fv     ( (lightPos + camR * size + camU * size).GetData() );
                glTexCoord2f    ( 0.0f, 1.0f );             glVertex3fv     ( (lightPos - camR * size + camU * size).GetData() );
            glEnd();
        }

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );

        glDepthMask     ( true );
        glEnable        ( GL_CULL_FACE );
        glDisable       ( GL_TEXTURE_2D );
    }
}


void ControlTower::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );
}


void ControlTower::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    char *word = _in->GetNextToken();
    m_controlBuildingId = atoi(word);
}

void ControlTower::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%6d", m_controlBuildingId);
}

int  ControlTower::GetBuildingLink()
{
    return m_controlBuildingId;
}

void ControlTower::SetBuildingLink( int _buildingId )
{
    m_controlBuildingId = _buildingId;
}
