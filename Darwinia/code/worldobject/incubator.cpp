#include "lib/universal_include.h"

#include "lib/file_writer.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"

#include "app.h"
#include "globals.h"
#include "location.h"
#include "particle_system.h"
#include "global_world.h"
#include "camera.h"

#include "sound/soundsystem.h"

#include "worldobject/incubator.h"



Incubator::Incubator()
:   Building(),
    m_spiritCentre(NULL),
    m_troopType(Entity::TypeDarwinian),
    m_timer(INCUBATOR_PROCESSTIME),
    m_numStartingSpirits(0)
{
    m_type = TypeIncubator;

    SetShape( g_app->m_resource->GetShape( "incubator.shp" ) );

    m_spiritCentre  = m_shape->m_rootFragment->LookupMarker( "MarkerSpirits" );
    m_exit          = m_shape->m_rootFragment->LookupMarker( "MarkerExit" );
    m_dock          = m_shape->m_rootFragment->LookupMarker( "MarkerDock" );

    m_spiritEntrance[0] = m_shape->m_rootFragment->LookupMarker( "MarkerSpiritIncoming0" );
    m_spiritEntrance[1] = m_shape->m_rootFragment->LookupMarker( "MarkerSpiritIncoming1" );
    m_spiritEntrance[2] = m_shape->m_rootFragment->LookupMarker( "MarkerSpiritIncoming2" );


    DarwiniaDebugAssert( m_spiritCentre );
    DarwiniaDebugAssert( m_exit );
    DarwiniaDebugAssert( m_dock );
    DarwiniaDebugAssert( m_spiritEntrance[0] );
    DarwiniaDebugAssert( m_spiritEntrance[1] );
    DarwiniaDebugAssert( m_spiritEntrance[2] );

    m_spirits.SetStepSize( 20 );
}

Incubator::~Incubator()
{
}


void Incubator::Initialise( Building *_template )
{
    Building::Initialise( _template );

    Matrix34 mat( m_front, g_upVector, m_pos );
    Vector3 spiritCentre = m_spiritCentre->GetWorldMatrix(mat).pos;

    m_numStartingSpirits = ((Incubator *) _template)->m_numStartingSpirits;

    for( int i = 0; i < m_numStartingSpirits; ++i )
    {
        Spirit *s = m_spirits.GetPointer();
        s->m_pos = spiritCentre + Vector3(sfrand(20.0f), sfrand(20.0f), sfrand(20.0f) );
        s->m_teamId = m_id.GetTeamId();
        s->Begin();
        s->m_state = Spirit::StateInStore;
    }
}


bool Incubator::Advance()
{
    //
    // If there are spirits in the building
    // Spawn entities at the door

    if( m_id.GetTeamId() != 255 )
    {
        if( m_spirits.NumUsed() > 0 )
        {
            m_timer -= SERVER_ADVANCE_PERIOD;
            if( m_timer <= 0.0f )
            {
                SpawnEntity();
                m_timer = INCUBATOR_PROCESSTIME;

                float overCrowded = (float) m_spirits.NumUsed() / 10.0f;
                m_timer -= overCrowded;

                if( m_timer < 0.5f ) m_timer = 0.5f;
            }
        }
    }

    //
    // Advance all the spirits in our store

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *spirit = m_spirits.GetPointer(i);
            spirit->Advance();
        }
    }


    //
    // Reduce incoming and outgoing logs

    for( int i = 0; i < m_incoming.Size(); ++i )
    {
        IncubatorIncoming *ii = m_incoming[i];
        ii->m_alpha -= SERVER_ADVANCE_PERIOD;
        if( ii->m_alpha <= 0.0f )
        {
            m_incoming.RemoveData(i);
            delete ii;
            --i;
        }
    }

    return Building::Advance();
}


void Incubator::SpawnEntity()
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    Matrix34 exit = m_exit->GetWorldMatrix(mat);

    //
    // Spawn the entity
    int teamId = m_id.GetTeamId();
    if( teamId == 2 ) teamId = 0;               // Green rather than yellow

    g_app->m_location->SpawnEntities( exit.pos, teamId, -1, m_troopType, 1, exit.f, 0.0f );


    //
    // Remove a spirit
    Vector3 spiritPos;
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            spiritPos = m_spirits[i].m_pos;
            m_spirits.MarkNotUsed( i );
            break;
        }
    }


    //
    // Create Particle + outgoing affects

    IncubatorIncoming *ii = new IncubatorIncoming();
    ii->m_pos = spiritPos;
    ii->m_entrance = 2;
    ii->m_alpha = 1.0f;
    m_incoming.PutData( ii );

    int numFlashes = 4 + darwiniaRandom() % 4;
    for( int i = 0; i < numFlashes; ++i )
    {
        Vector3 vel( sfrand(15.0f), frand(35.0f), sfrand(15.0f) );
        g_app->m_particleSystem->CreateParticle( exit.pos, vel, Particle::TypeControlFlash );
        //g_app->m_particleSystem->CreateParticle( spiritPos, vel, Particle::TypeControlFlash );
    }


    //
    // Sound effect

    g_app->m_soundSystem->TriggerBuildingEvent( this, "SpawnEntity" );
}


void Incubator::AddSpirit( Spirit *_spirit )
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    Vector3 spiritCentre = m_spiritCentre->GetWorldMatrix(mat).pos;

    Spirit *s = m_spirits.GetPointer();
    s->m_pos = spiritCentre + Vector3(sfrand(20.0f), sfrand(20.0f), sfrand(20.0f) );
    s->m_teamId = _spirit->m_teamId;
    s->m_state = Spirit::StateInStore;

    IncubatorIncoming *ii = new IncubatorIncoming();
    ii->m_pos = s->m_pos;
    ii->m_entrance = syncrand() % 2;
    ii->m_alpha = 1.0f;
    m_incoming.PutData( ii );

    g_app->m_soundSystem->TriggerBuildingEvent( this, "AddSpirit" );
}


void Incubator::GetDockPoint( Vector3 &_pos, Vector3 &_front )
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    Matrix34 dock = m_dock->GetWorldMatrix( mat );
    _pos = dock.pos;
    _pos = PushFromBuilding( _pos, 5.0f );
    _front = dock.f;
}


int Incubator::NumSpiritsInside()
{
    return m_spirits.NumUsed();
}


void Incubator::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    if( _in->TokenAvailable() )
    {
        m_numStartingSpirits = atoi( _in->GetNextToken() );
    }
}


void Incubator::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%6d", m_numStartingSpirits );
}


void Incubator::Render( float _predictionTime )
{
    Building::Render( _predictionTime );

//    Vector3 dockPos, dockFront;
//    GetDockPoint( dockPos, dockFront );
//    RenderSphere( dockPos, 5.0f );
}


void Incubator::RenderAlphas( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );

    glDisable       ( GL_CULL_FACE );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_BLEND );
    glDepthMask     ( false );

    //
    // Render all spirits

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *spirit = m_spirits.GetPointer(i);
            spirit->Render( _predictionTime );
        }
    }


    //
    // Render incoming and outgoing effects

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );

    Matrix34 mat( m_front, g_upVector, m_pos );
    Vector3 entrances[3];
    entrances[0] = m_spiritEntrance[0]->GetWorldMatrix( mat ).pos;
    entrances[1] = m_spiritEntrance[1]->GetWorldMatrix( mat ).pos;
    entrances[2] = m_spiritEntrance[2]->GetWorldMatrix( mat ).pos;

    for( int i = 0; i < m_incoming.Size(); ++i )
    {
        IncubatorIncoming *ii = m_incoming[i];
        Vector3 fromPos = ii->m_pos;
        Vector3 toPos = entrances[ii->m_entrance];

        Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0f;
        Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
        Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();

        rightAngle *= 1.5f;

        glColor4f( 1.0f, 1.0f, 0.2f, ii->m_alpha );

        glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3fv( (fromPos - rightAngle).GetData() );
                glTexCoord2i(0,1);      glVertex3fv( (fromPos + rightAngle).GetData() );
                glTexCoord2i(1,1);      glVertex3fv( (toPos + rightAngle).GetData() );
                glTexCoord2i(1,0);      glVertex3fv( (toPos - rightAngle).GetData() );
        glEnd();

    }

    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );
    glDisable       ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_CULL_FACE );
}


void Incubator::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "AddSpirit" );
    _list->PutData( "SpawnEntity" );
}

