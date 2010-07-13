#include "lib/universal_include.h"

#include "lib/filesys/text_file_writer.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "globals.h"
#include "location.h"
#include "particle_system.h"
#include "global_world.h"
#include "camera.h"
#include "team.h"
#include "gametimer.h"

#include "sound/soundsystem.h"

#include "worldobject/incubator.h"
#include "worldobject/darwinian.h"



Incubator::Incubator()
:   Building(),
    m_spiritCentre(NULL),
    m_troopType(Entity::TypeDarwinian),
    m_timer(INCUBATOR_PROCESSTIME),
    m_numStartingSpirits(0)
{
    m_type = TypeIncubator;

    SetShape( g_app->m_resource->GetShape( "incubator.shp" ) );

    const char spiritCentreName[] = "MarkerSpirits";
    m_spiritCentre  = m_shape->m_rootFragment->LookupMarker( spiritCentreName );
    AppReleaseAssert( m_spiritCentre, "Incubator: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", spiritCentreName, m_shape->m_name );

    const char exitName[] = "MarkerExit";
    m_exit          = m_shape->m_rootFragment->LookupMarker( exitName );
    AppReleaseAssert( m_exit, "Incubator: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", exitName, m_shape->m_name );

    const char dockName[] = "MarkerDock";
    m_dock          = m_shape->m_rootFragment->LookupMarker( dockName );
    AppReleaseAssert( m_dock, "Incubator: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", dockName, m_shape->m_name );

    const char spiritEntrance0Name[] = "MarkerSpiritIncoming0";
    m_spiritEntrance[0] = m_shape->m_rootFragment->LookupMarker( spiritEntrance0Name );
    AppReleaseAssert( m_spiritEntrance[0], "Incubator: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", spiritEntrance0Name, m_shape->m_name );

    const char spiritEntrance1Name[] = "MarkerSpiritIncoming1";
    m_spiritEntrance[1] = m_shape->m_rootFragment->LookupMarker( spiritEntrance1Name );
    AppReleaseAssert( m_spiritEntrance[1], "Incubator: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", spiritEntrance1Name, m_shape->m_name );

    const char spiritEntrance2Name[] = "MarkerSpiritIncoming2";
    m_spiritEntrance[2] = m_shape->m_rootFragment->LookupMarker( spiritEntrance2Name );
    AppReleaseAssert( m_spiritEntrance[2], "Incubator: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", spiritEntrance2Name, m_shape->m_name );

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
        s->m_pos = spiritCentre + Vector3(SFRAND(20.0), SFRAND(20.0), SFRAND(20.0) );
        if( m_id.GetTeamId() == 255 )
        {
            s->m_teamId = syncfrand(NUM_TEAMS);
        }
        else
        {
            s->m_teamId = m_id.GetTeamId();  
        }
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
            if( m_timer <= 0.0 )
            {
                SpawnEntity();

                if( g_app->Multiplayer() ) 
                {
                    m_timer = INCUBATOR_PROCESSTIME_MULTIPLAYER;
                }
                else
                {
                    m_timer = INCUBATOR_PROCESSTIME;
                    double overCrowded = (double) m_spirits.NumUsed() / 10.0;
                    m_timer -= overCrowded;
                    if( m_timer < 0.5 ) m_timer = 0.5;
                }
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
        if( ii->m_alpha <= 0.0 )
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
    int teamId = m_id.GetTeamId();

	if( teamId == 2 && g_app->IsSinglePlayer())//!g_app->m_multiwinia )
    {
        // Yellow owned incubators spawn green Darwinians
        // when in Single Player mode
        teamId = 0;               
    }


    // 
    // Check to be sure we have a team

    if( teamId < 0 || 
        teamId >= NUM_TEAMS ||
        g_app->m_location->m_teams[teamId]->m_teamType == TeamTypeUnused )
    {
        return;
    }

    
    //
    // Spawn the entity
     
    Matrix34 mat( m_front, g_upVector, m_pos );
    Matrix34 exit = m_exit->GetWorldMatrix(mat);
    
    Vector3 exitPos = exit.pos;
    Vector3 exitVel = exit.f;


    if( g_app->Multiplayer() )
    {
        exitPos.y += 20;

        exitVel.x += syncsfrand(0.2);
        exitVel.y += syncsfrand(0.2);
        exitVel.z += syncsfrand(0.2);
        exitVel *= 10;
    }

    WorldObjectId spawned = g_app->m_location->SpawnEntities( exitPos, teamId, -1, m_troopType, 1, exitVel, 0.0 );
    Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntitySafe( spawned, Entity::TypeDarwinian );
    darwinian->m_onGround = true;


    
    //
    // Remove a spirit

    Vector3 spiritPos;
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            spiritPos = m_spirits[i].m_pos;
            m_spirits.RemoveData( i );
            break;
        }
    }


    //
    // Create Particle + outgoing affects
    
    IncubatorIncoming *ii = new IncubatorIncoming();
    ii->m_pos = spiritPos;
    ii->m_entrance = 2;
    ii->m_alpha = 1.0;
    m_incoming.PutData( ii );

    int numFlashes = 4 + AppRandom() % 4;
    for( int i = 0; i < numFlashes; ++i )
    {
        Vector3 vel( sfrand(15.0), frand(35.0), sfrand(15.0) );
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
    s->m_pos = spiritCentre + Vector3(SFRAND(20.0), SFRAND(20.0), SFRAND(20.0) );
    s->m_teamId = _spirit->m_teamId;        
    s->m_state = Spirit::StateInStore;   
    
    IncubatorIncoming *ii = new IncubatorIncoming();
    ii->m_pos = s->m_pos;
    ii->m_entrance = syncrand() % 2;
    ii->m_alpha = 1.0;
    m_incoming.PutData( ii );

    g_app->m_soundSystem->TriggerBuildingEvent( this, "AddSpirit" );
}


void Incubator::GetDockPoint( Vector3 &_pos, Vector3 &_front )
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    Matrix34 dock = m_dock->GetWorldMatrix( mat );
    _pos = dock.pos;
    _pos = PushFromBuilding( _pos, 5.0 );
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


void Incubator::Write( TextWriter *_out )
{   
    Building::Write( _out );

    _out->printf( "%6d", m_numStartingSpirits );
}


void Incubator::Render( double _predictionTime )
{
    Building::Render( _predictionTime );    

//    Vector3 dockPos, dockFront;
//    GetDockPoint( dockPos, dockFront );
//    RenderSphere( dockPos, 5.0 );
}


void Incubator::RenderAlphas( double _predictionTime )
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
    
        Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0;
        Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
        Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();
    
        rightAngle *= 1.5;
    
        glColor4f( 1.0, 1.0, 0.2, ii->m_alpha );

        glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3dv( (fromPos - rightAngle).GetData() );
                glTexCoord2i(0,1);      glVertex3dv( (fromPos + rightAngle).GetData() );
                glTexCoord2i(1,1);      glVertex3dv( (toPos + rightAngle).GetData() );                
                glTexCoord2i(1,0);      glVertex3dv( (toPos - rightAngle).GetData() );                     
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

