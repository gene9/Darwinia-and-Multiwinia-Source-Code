#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/text_stream_readers.h"
#include "lib/debug_render.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#include "worldobject/spiritreceiver.h"
#include "worldobject/darwinian.h"

#include "app.h"
#include "location.h"
#include "camera.h"
#include "global_world.h"
#include "particle_system.h"
#include "main.h"
#include "team.h"
#include "renderer.h"
#include "entity_grid.h"
#include "water_reflection.h"

#include "sound/soundsystem.h"


// ****************************************************************************
// Class ReceiverBuilding
// ****************************************************************************

ReceiverBuilding::ReceiverBuilding()
:   Building(),
    m_spiritLink(-1),
    m_spiritLocation(NULL)
{
}

void ReceiverBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_spiritLink = ((ReceiverBuilding *) _template)->m_spiritLink;
}

Vector3 ReceiverBuilding::GetSpiritLocation()
{
    if( !m_spiritLocation )
    {
        m_spiritLocation = m_shape->m_rootFragment->LookupMarker( "MarkerSpiritLink" );
        DarwiniaDebugAssert( m_spiritLocation );
    }

    Matrix34 rootMat( m_front, m_up, m_pos );
    Matrix34 worldPos = m_spiritLocation->GetWorldMatrix( rootMat );
    return worldPos.pos;
}


bool ReceiverBuilding::IsInView()
{
    Building *spiritLink = g_app->m_location->GetBuilding( m_spiritLink);

    if( spiritLink )
    {
        Vector3 midPoint = ( spiritLink->m_centrePos + m_centrePos ) / 2.0f;
        float radius = ( spiritLink->m_centrePos - m_centrePos ).Mag() / 2.0f;
        radius += m_radius;
        return( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) );
    }
    else
    {
        return Building::IsInView();
    }
}


void ReceiverBuilding::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "TriggerSpirit" );
}


void ReceiverBuilding::Render( float _predictionTime )
{
	Matrix34 mat(m_front, m_up, m_pos);
	m_shape->Render(_predictionTime, mat);
}


void ReceiverBuilding::RenderAlphas ( float _predictionTime )
{
#ifdef USE_DIRECT3D
	// don't reflect us in water
	if(g_waterReflectionEffect && g_waterReflectionEffect->IsPrerendering()) return;
#endif

	Building::RenderAlphas( _predictionTime );

    _predictionTime -= 0.1f;

    Building *spiritLink = g_app->m_location->GetBuilding( m_spiritLink );

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );

    if( spiritLink )
    {
        //
        // Render the spirit line itself

        ReceiverBuilding *receiverBuilding = (ReceiverBuilding *) spiritLink;

        Vector3 ourPos = GetSpiritLocation();
        Vector3 theirPos = receiverBuilding->GetSpiritLocation();

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
            glEnable        ( GL_TEXTURE_2D );
            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

            glEnable    ( GL_BLEND );
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

            size = 1.0f;
        }

        ourPosRight.SetLength( size );
        theirPosRight.SetLength( size );

        glBegin( GL_QUADS );
            glTexCoord2f(0.1f, 0);      glVertex3fv( (ourPos - ourPosRight).GetData() );
            glTexCoord2f(0.1f, 1);      glVertex3fv( (ourPos + ourPosRight).GetData() );
            glTexCoord2f(0.9f, 1);      glVertex3fv( (theirPos + theirPosRight).GetData() );
            glTexCoord2f(0.9f, 0);      glVertex3fv( (theirPos - theirPosRight).GetData() );
        glEnd();

        glDisable       ( GL_TEXTURE_2D );


        //
        // Render any surges

        BeginRenderUnprocessedSpirits();
#ifndef USE_DIRECT3D
		for( int i = 0; i < m_spirits.Size(); ++i )
		{
			float thisSpirit = m_spirits[i];
			thisSpirit += _predictionTime * 0.8f;
			if( thisSpirit < 0.0f ) thisSpirit = 0.0f;
			if( thisSpirit > 1.0f ) thisSpirit = 1.0f;
			Vector3 thisSpiritPos = ourPos + (theirPos-ourPos) * thisSpirit;
			RenderUnprocessedSpirit( thisSpiritPos, 1.0f );
		}
#else
		glBegin( GL_QUADS );
			for( int i = 0; i < m_spirits.Size(); ++i )
			{
				float thisSpirit = m_spirits[i];
				thisSpirit += _predictionTime * 0.8f;
				if( thisSpirit < 0.0f ) thisSpirit = 0.0f;
				if( thisSpirit > 1.0f ) thisSpirit = 1.0f;
				Vector3 thisSpiritPos = ourPos + (theirPos-ourPos) * thisSpirit;
				RenderUnprocessedSpirit_basic( thisSpiritPos, 1.0f );
			}
		glEnd();
		int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
		if( buildingDetail == 1 )
		{
			glEnable( GL_TEXTURE_2D );
			glBegin( GL_QUADS );
				for( int i = 0; i < m_spirits.Size(); ++i )
				{
					float thisSpirit = m_spirits[i];
					thisSpirit += _predictionTime * 0.8f;
					if( thisSpirit < 0.0f ) thisSpirit = 0.0f;
					if( thisSpirit > 1.0f ) thisSpirit = 1.0f;
					Vector3 thisSpiritPos = ourPos + (theirPos-ourPos) * thisSpirit;
					RenderUnprocessedSpirit_detail( thisSpiritPos, 1.0f );
				}
			glEnd();
			glDisable( GL_TEXTURE_2D );
		}
#endif
        EndRenderUnprocessedSpirits();
    }
}

bool ReceiverBuilding::Advance()
{
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        float *thisSpirit = m_spirits.GetPointer(i);
        *thisSpirit += SERVER_ADVANCE_PERIOD * 0.8f;
        if( *thisSpirit >= 1.0f )
        {
            m_spirits.RemoveData(i);
            --i;

            Building *spiritLink = g_app->m_location->GetBuilding( m_spiritLink );
            if( spiritLink )
            {
                ReceiverBuilding *receiverBuilding = (ReceiverBuilding *) spiritLink;
                receiverBuilding->TriggerSpirit( 0.0f );
            }
        }
    }
    return Building::Advance();
}

void ReceiverBuilding::TriggerSpirit ( float _initValue )
{
    m_spirits.PutDataAtStart( _initValue );
    g_app->m_soundSystem->TriggerBuildingEvent( this, "TriggerSpirit" );
}

void ReceiverBuilding::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    m_spiritLink = atoi( _in->GetNextToken() );
}

void ReceiverBuilding::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_spiritLink );
}

int ReceiverBuilding::GetBuildingLink()
{
    return m_spiritLink;
}

void ReceiverBuilding::SetBuildingLink( int _buildingId )
{
    m_spiritLink = _buildingId;
}


SpiritProcessor *ReceiverBuilding::GetSpiritProcessor()
{
    static int processorId = -1;

    SpiritProcessor *processor = (SpiritProcessor *) g_app->m_location->GetBuilding( processorId );

    if( !processor || processor->m_type != Building::TypeSpiritProcessor )
    {
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building->m_type == TypeSpiritProcessor )
                {
                    processor = (SpiritProcessor *) building;
                    processorId = processor->m_id.GetUniqueId();
                    break;
                }
            }
        }
    }

    return processor;
}


static float s_nearPlaneStart;

void ReceiverBuilding::BeginRenderUnprocessedSpirits()
{
    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    if( buildingDetail == 1 )
    {
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
    }

	s_nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart * 1.1f,
							 			   g_app->m_renderer->GetFarPlane());

}


void ReceiverBuilding::RenderUnprocessedSpirit( Vector3 const &_pos, float _life )
{
    Vector3 position = _pos;
    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
    float scale = 2.0f * _life;
    float alphaValue = _life;

    glColor4f( 0.6f, 0.2f, 0.1f, alphaValue);
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 3 * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 3 * scale).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 3 * scale).GetData() );
        glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 3 * scale).GetData() );
        glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 3 * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 3 * scale).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 3 * scale).GetData() );
        glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 3 * scale).GetData() );
    glEnd();

    glColor4f( 0.6f, 0.2f, 0.1f, alphaValue);
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 1 * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 1 * scale).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 1 * scale).GetData() );
        glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 1 * scale).GetData() );
        glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 1 * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 1 * scale).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 1 * scale).GetData() );
        glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 1 * scale).GetData() );
    glEnd();

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    if( buildingDetail == 1 )
    {
        glEnable        ( GL_TEXTURE_2D );
        glColor4f( 0.6f, 0.2f, 0.1f, alphaValue/1.5f);
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 30 * scale).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 30 * scale).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 30 * scale).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 30 * scale).GetData() );
        glEnd();
        glDisable       ( GL_TEXTURE_2D );
    }
}

void ReceiverBuilding::RenderUnprocessedSpirit_basic( Vector3 const &_pos, float _life )
{
	Vector3 position = _pos;
	Vector3 camUp = g_app->m_camera->GetUp();
	Vector3 camRight = g_app->m_camera->GetRight();
	float scale = 2.0f * _life;
	float alphaValue = _life;

	glColor4f( 0.6f, 0.2f, 0.1f, alphaValue);
	glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 3 * scale).GetData() );
	glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 3 * scale).GetData() );
	glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 3 * scale).GetData() );
	glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 3 * scale).GetData() );
	glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 3 * scale).GetData() );
	glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 3 * scale).GetData() );
	glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 3 * scale).GetData() );
	glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 3 * scale).GetData() );

	glColor4f( 0.6f, 0.2f, 0.1f, alphaValue);
	glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 1 * scale).GetData() );
	glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 1 * scale).GetData() );
	glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 1 * scale).GetData() );
	glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 1 * scale).GetData() );
	glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 1 * scale).GetData() );
	glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 1 * scale).GetData() );
	glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 1 * scale).GetData() );
	glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 1 * scale).GetData() );
}

void ReceiverBuilding::RenderUnprocessedSpirit_detail( Vector3 const &_pos, float _life )
{
	Vector3 position = _pos;
	Vector3 camUp = g_app->m_camera->GetUp();
	Vector3 camRight = g_app->m_camera->GetRight();
	float scale = 2.0f * _life;
	float alphaValue = _life;

	glColor4f( 0.6f, 0.2f, 0.1f, alphaValue/1.5f);
	glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 30 * scale).GetData() );
	glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 30 * scale).GetData() );
	glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 30 * scale).GetData() );
	glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 30 * scale).GetData() );
}


void ReceiverBuilding::EndRenderUnprocessedSpirits()
{
	g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart,
								 		   g_app->m_renderer->GetFarPlane());

    glDisable   ( GL_TEXTURE_2D );
    glDepthMask ( true );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable   ( GL_BLEND );
    glEnable    ( GL_CULL_FACE );
}


// ****************************************************************************
// Class SpiritProcessor
// ****************************************************************************

SpiritProcessor::SpiritProcessor()
:   ReceiverBuilding(),
    m_throughput(0.0f),
    m_timerSync(0.0f),
    m_numThisSecond(0),
    m_spawnSync(0.0f)
{
    m_type = TypeSpiritProcessor;
    SetShape( g_app->m_resource->GetShape( "spiritprocessor.shp" ) );
}


void SpiritProcessor::Initialise( Building *_building )
{
    ReceiverBuilding::Initialise( _building );

    //
    // Spawn some unprocessed spirits

    for( int i = 0; i < 150; ++i )
    {
        float sizeX = g_app->m_location->m_landscape.GetWorldSizeX();
        float sizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
        float posY = syncfrand(1000.0f);
        Vector3 spawnPos = Vector3( syncfrand(sizeX), posY, syncfrand(sizeZ) ) ;

        UnprocessedSpirit *spirit = new UnprocessedSpirit();
        spirit->m_pos = spawnPos;
        m_floatingSpirits.PutData( spirit );
    }
}


char *SpiritProcessor::GetObjectiveCounter()
{
    static char result[256];
    sprintf( result, "%s : %2.2f", LANGUAGEPHRASE("objective_throughput"), m_throughput );
    return result;
}


void SpiritProcessor::TriggerSpirit( float _initValue )
{
    ReceiverBuilding::TriggerSpirit( _initValue );

    ++m_numThisSecond;
}


bool SpiritProcessor::IsInView()
{
    return true;
}


bool SpiritProcessor::Advance()
{
    //
    // Calculate our throughput

    m_timerSync -= SERVER_ADVANCE_PERIOD;

    if( m_timerSync <= 0.0f )
    {
        float newAverage = m_numThisSecond;
        newAverage *= 7.0f;
        m_numThisSecond = 0;
        m_timerSync = 10.0f;
        if( newAverage > m_throughput )
        {
            m_throughput = newAverage;
        }
        else
        {
            m_throughput = m_throughput * 0.8f + newAverage * 0.2f;
        }
    }

    if( m_throughput > 50.0f )
    {
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        gb->m_online = true;
    }


    //
    // Advance all unprocessed spirits

    for( int i = 0; i < m_floatingSpirits.Size(); ++i )
    {
        UnprocessedSpirit *spirit = m_floatingSpirits[i];
        bool finished = spirit->Advance();
        if( finished )
        {
            m_floatingSpirits.RemoveData(i);
            delete spirit;
            --i;
        }
    }


    //
    // Spawn more unprocessed spirits

    m_spawnSync -= SERVER_ADVANCE_PERIOD;
    if( m_spawnSync <= 0.0f )
    {
        m_spawnSync = 0.2f;

        float sizeX = g_app->m_location->m_landscape.GetWorldSizeX();
        float sizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
        float posY = 700.0f + syncfrand(300.0f);
        Vector3 spawnPos = Vector3( syncfrand(sizeX), posY, syncfrand(sizeZ) ) ;
        UnprocessedSpirit *spirit = new UnprocessedSpirit();
        spirit->m_pos = spawnPos;
        m_floatingSpirits.PutData( spirit );
    }

    return ReceiverBuilding::Advance();
}


void SpiritProcessor::Render( float _predictionTime )
{
    ReceiverBuilding::Render( _predictionTime );

    //g_gameFont.DrawText3DCentre( m_pos + Vector3(0,215,0), 10.0f, "NumThisSecond : %d", m_numThisSecond );
    //g_gameFont.DrawText3DCentre( m_pos + Vector3(0,200,0), 10.0f, "Throughput    : %2.2f", m_throughput );
}


void SpiritProcessor::RenderAlphas( float _predictionTime )
{
#ifdef USE_DIRECT3D
	// don't reflect us in water
	if(g_waterReflectionEffect && g_waterReflectionEffect->IsPrerendering()) return;
#endif

    ReceiverBuilding::RenderAlphas( _predictionTime );

    //
    // Render all floating spirits

    BeginRenderUnprocessedSpirits();

    _predictionTime -= SERVER_ADVANCE_PERIOD;

#ifndef USE_DIRECT3D
    for( int i = 0; i < m_floatingSpirits.Size(); ++i )
    {
        UnprocessedSpirit *spirit = m_floatingSpirits[i];
        Vector3 pos = spirit->m_pos;
        pos += spirit->m_vel * _predictionTime;
        pos += spirit->m_hover * _predictionTime;
        float life = spirit->GetLife();
        RenderUnprocessedSpirit( pos, life );
    }
#else
	glBegin( GL_QUADS );
		for( int i = 0; i < m_floatingSpirits.Size(); ++i )
		{
			UnprocessedSpirit *spirit = m_floatingSpirits[i];
			Vector3 pos = spirit->m_pos;
			pos += spirit->m_vel * _predictionTime;
			pos += spirit->m_hover * _predictionTime;
			float life = spirit->GetLife();
			RenderUnprocessedSpirit_basic( pos, life );
		}
	glEnd();
	int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
	if( buildingDetail == 1 )
	{
		glEnable( GL_TEXTURE_2D );
		glBegin( GL_QUADS );
			for( int i = 0; i < m_floatingSpirits.Size(); ++i )
			{
				UnprocessedSpirit *spirit = m_floatingSpirits[i];
				Vector3 pos = spirit->m_pos;
				pos += spirit->m_vel * _predictionTime;
				pos += spirit->m_hover * _predictionTime;
				float life = spirit->GetLife();
				RenderUnprocessedSpirit_detail( pos, life );
			}
		glEnd();
		glDisable( GL_TEXTURE_2D );
	}
#endif
    EndRenderUnprocessedSpirits();
}


// ****************************************************************************
// Class ReceiverLink
// ****************************************************************************

ReceiverLink::ReceiverLink()
:   ReceiverBuilding()
{
    m_type = TypeReceiverLink;
    SetShape( g_app->m_resource->GetShape( "receiverlink.shp" ) );
}


bool ReceiverLink::Advance()
{
    return ReceiverBuilding::Advance();
}


// ****************************************************************************
// Class ReceiverSpiritSpawner
// ****************************************************************************

ReceiverSpiritSpawner::ReceiverSpiritSpawner()
:   ReceiverBuilding()
{
    m_type = TypeReceiverSpiritSpawner;
    SetShape( g_app->m_resource->GetShape( "receiverlink.shp" ) );
}


bool ReceiverSpiritSpawner::Advance()
{
    if( syncfrand(10.0f) < 1.0f )
    {
        TriggerSpirit( 0.0f );
    }

    return ReceiverBuilding::Advance();
}


// ****************************************************************************
// Class SpiritReceiver
// ****************************************************************************

SpiritReceiver::SpiritReceiver()
:   ReceiverBuilding(),
    m_headMarker(NULL),
    m_headShape(NULL),
    m_spiritLink(NULL)
{
    m_type = TypeSpiritReceiver;
    SetShape( g_app->m_resource->GetShape( "spiritreceiver.shp" ) );
    m_headMarker = m_shape->m_rootFragment->LookupMarker( "MarkerHead" );

    for( int i = 0; i < SPIRITRECEIVER_NUMSTATUSMARKERS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerStatus0%d", i+1 );
        m_statusMarkers[i] = m_shape->m_rootFragment->LookupMarker( name );
    }

    m_headShape = g_app->m_resource->GetShape( "spiritreceiverhead.shp" );
    m_spiritLink = m_headShape->m_rootFragment->LookupMarker( "MarkerSpiritLink" );
}


void SpiritReceiver::Initialise( Building *_template )
{
    _template->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( _template->m_pos.x, _template->m_pos.z );
    Vector3 right = Vector3( 1, 0, 0 );
    _template->m_front = right ^ _template->m_up;

    ReceiverBuilding::Initialise( _template );
}


bool SpiritReceiver::Advance()
{
    float fractionOccupied = (float) GetNumPortsOccupied() / (float) GetNumPorts();

    //
    // Search for spirits nearby

    SpiritProcessor *processor = GetSpiritProcessor();
    if( processor && fractionOccupied > 0.0f )
    {
        for( int i = 0; i < processor->m_floatingSpirits.Size(); ++i )
        {
            UnprocessedSpirit *spirit = processor->m_floatingSpirits[i];
            if( spirit->m_state == UnprocessedSpirit::StateUnprocessedFloating )
            {
                Vector3 themToUs = ( m_pos - spirit->m_pos );
                float distance = themToUs.Mag();
                Vector3 targetPos = m_pos;
                if( distance < 100.0f )
                {
                    float fraction = 1.0f - distance / 100.0f;
                    targetPos += Vector3(0,100.0f*fraction,0);
                    themToUs = targetPos - spirit->m_pos;
                    distance = themToUs.Mag();
                }

                if( distance < 10.0f )
                {
                    processor->m_floatingSpirits.RemoveData(i);
                    delete spirit;
                    --i;
                    TriggerSpirit(0.0f);
                }
                else if( distance < 200.0f )
                {
                    float fraction = 1.0f - distance / 200.0f;
                    fraction *= fractionOccupied;
                    themToUs.SetLength( 20.0f * fraction );
                    spirit->m_vel += themToUs * SERVER_ADVANCE_PERIOD;
                }
            }
        }
    }

    return ReceiverBuilding::Advance();
}


void SpiritReceiver::Render( float _predictionTime )
{
    if( g_app->m_editing )
    {
        m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
        Vector3 right( 1, 0, 0 );
        m_front = right ^ m_up;
    }

    ReceiverBuilding::Render( _predictionTime );

    Matrix34 mat( m_front, m_up, m_pos );
    Vector3 headPos = m_headMarker->GetWorldMatrix(mat).pos;
    Vector3 up = g_upVector;
    Vector3 right( 1, 0, 0 );
    Vector3 front = up ^ right;
    Matrix34 headMat( front, up, headPos );
    m_headShape->Render( _predictionTime, headMat );
}


Vector3 SpiritReceiver::GetSpiritLocation()
{
    Matrix34 mat( m_front, m_up, m_pos );
    Vector3 headPos = m_headMarker->GetWorldMatrix(mat).pos;
    Vector3 up = g_upVector;
    Vector3 right( 1, 0, 0 );
    Vector3 front = up ^ right;
    Matrix34 headMat( front, up, headPos );
    Vector3 spiritLinkPos = m_spiritLink->GetWorldMatrix(headMat).pos;
    return spiritLinkPos;
}


void SpiritReceiver::RenderPorts()
{
    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        Matrix34 rootMat(m_front, m_up, m_pos);
        Matrix34 worldMat = m_statusMarkers[i]->GetWorldMatrix(rootMat);

        //
        // Render the status light

        float size = 6.0f;
        Vector3 camR = g_app->m_camera->GetRight() * size;
        Vector3 camU = g_app->m_camera->GetUp() * size;

        Vector3 statusPos = worldMat.pos;

        if( GetPortOccupant(i).IsValid() )      glColor4f( 0.3f, 1.0f, 0.3f, 1.0f );
        else                                    glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );           glVertex3fv( (statusPos - camR - camU).GetData() );
            glTexCoord2i( 1, 0 );           glVertex3fv( (statusPos + camR - camU).GetData() );
            glTexCoord2i( 1, 1 );           glVertex3fv( (statusPos + camR + camU).GetData() );
            glTexCoord2i( 0, 1 );           glVertex3fv( (statusPos - camR + camU).GetData() );
        glEnd();
    }

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}


void SpiritReceiver::RenderAlphas( float _predictionTime )
{
    ReceiverBuilding::RenderAlphas( _predictionTime );

    //RenderHitCheck();

    float fractionOccupied = (float) GetNumPortsOccupied() / (float) GetNumPorts();
}


// ****************************************************************************
// Class UnprocessedSpirit
// ****************************************************************************

UnprocessedSpirit::UnprocessedSpirit()
:   WorldObject()
{
    m_state = StateUnprocessedFalling;

    m_positionOffset = syncfrand(10.0f);
    m_xaxisRate = syncfrand(2.0f);
    m_yaxisRate = syncfrand(2.0f);
    m_zaxisRate = syncfrand(2.0f);

    m_timeSync = GetHighResTime();
}


bool UnprocessedSpirit::Advance()
{
    m_vel *= 0.9f;

    //
    // Make me float around slowly

    m_positionOffset += SERVER_ADVANCE_PERIOD;
    m_xaxisRate += syncsfrand(1.0f);
    m_yaxisRate += syncsfrand(1.0f);
    m_zaxisRate += syncsfrand(1.0f);
    if( m_xaxisRate > 2.0f ) m_xaxisRate = 2.0f;
    if( m_xaxisRate < 0.0f ) m_xaxisRate = 0.0f;
    if( m_yaxisRate > 2.0f ) m_yaxisRate = 2.0f;
    if( m_yaxisRate < 0.0f ) m_yaxisRate = 0.0f;
    if( m_zaxisRate > 2.0f ) m_zaxisRate = 2.0f;
    if( m_zaxisRate < 0.0f ) m_zaxisRate = 0.0f;
    m_hover.x = sinf( m_positionOffset ) * m_xaxisRate;
    m_hover.y = sinf( m_positionOffset ) * m_yaxisRate;
    m_hover.z = sinf( m_positionOffset ) * m_zaxisRate;

    switch( m_state )
    {
        case StateUnprocessedFalling:
        {
            float heightAboveGround = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
            if( heightAboveGround > 15.0f )
            {
                float fractionAboveGround = heightAboveGround / 100.0f;
                fractionAboveGround = min( fractionAboveGround, 1.0f );
                m_hover.y = (-10.0f - syncfrand(10.0f)) * fractionAboveGround;
            }
            else
            {
                m_state = StateUnprocessedFloating;
                m_timeSync = 30.0f + syncsfrand(30.0f);
            }
            break;
        }

        case StateUnprocessedFloating:
            m_timeSync -= SERVER_ADVANCE_PERIOD;
            if( m_timeSync <= 0.0f )
            {
                m_state = StateUnprocessedDeath;
                m_timeSync = 10.0f;
            }
            break;

        case StateUnprocessedDeath:
            m_timeSync -= SERVER_ADVANCE_PERIOD;
            if( m_timeSync <= 0.0f ) return true;
            break;
    }

    Vector3 oldPos = m_pos;

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_pos += m_hover * SERVER_ADVANCE_PERIOD;
    float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0f ) m_pos.x = 0.0f;
    if( m_pos.z < 0.0f ) m_pos.z = 0.0f;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;

    return false;
}


float UnprocessedSpirit::GetLife()
{
    switch( m_state )
    {
        case StateUnprocessedFalling:
        {
            float timePassed = GetHighResTime() - m_timeSync;
            float life = timePassed / 10.0f;
            life = min( life, 1.0f );
            return life;
        }

        case StateUnprocessedFloating:
            return 1.0f;

        case StateUnprocessedDeath:
        {
            float timeLeft = m_timeSync;
            float life = timeLeft / 10.0f;
            life = min( life, 1.0f );
            life = max( life, 0.0f );
            return life;
        }
    }

    return 0.0f;
}
