#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/ogl_extensions.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"

#include "app.h"
#include "camera.h"
#include "entity_grid.h"
#include "globals.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "global_world.h"
#include "unit.h"

#include "sound/soundsystem.h"

#include "worldobject/radardish.h"



RadarDish::RadarDish()
:   Teleport(),
    m_signal(0.0f),
    m_range(0.0f),
    m_receiverId(-1),
    m_horizontallyAligned(true),
    m_verticallyAligned(true),
    m_movementSoundsPlaying(false),
    m_newlyCreated(true)
{
    m_type = Building::TypeRadarDish;
    m_target.Zero();
    m_front.Set(0,0,1);
    m_sendPeriod = RADARDISH_TRANSPORTPERIOD;

	Shape *radarShape = g_app->m_resource->GetShapeCopy("radardish.shp", true);
	SetShape( radarShape );

	m_dish = m_shape->m_rootFragment->LookupFragment("Dish");
	m_upperMount = m_shape->m_rootFragment->LookupFragment("UpperMount");
	m_focusMarker = m_shape->m_rootFragment->LookupMarker("MarkerFocus");
}


RadarDish::~RadarDish()
{
	delete m_shape;
	m_shape = NULL;
}


void RadarDish::SetDetail( int _detail )
{
    Teleport::SetDetail( _detail );

    Matrix34 rootMat(m_front, m_up, m_pos);
    Matrix34 worldMat = m_entrance->GetWorldMatrix(rootMat);
    m_entrancePos = worldMat.pos;
    m_entranceFront = worldMat.f;
}


bool RadarDish::GetEntrance( Vector3 &_pos, Vector3 &_front )
{
    _pos = m_entrancePos;
    _front = m_entranceFront;
    return true;
}


bool RadarDish::Advance ()
{
    //
    // If this is our first advance, align to our dish
    // if we saved a target dish previously
    if( m_newlyCreated )
    {
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb )
        {
            Building *targetBuilding = g_app->m_location->GetBuilding( gb->m_link );
            if( targetBuilding && targetBuilding->m_type == TypeRadarDish )
            {
                RadarDish *dish = (RadarDish *) targetBuilding;
                Aim( dish->GetStartPoint() );
            }
        }
        m_newlyCreated = false;
    }

	if (m_target.MagSquared() < 0.001f)		return Building::Advance();

	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
	Vector3 dishPos = worldMat.pos;
    Vector3 dishFront = worldMat.f;


	//
    // Rotate slowly to face our target (horiz)

    if( !m_horizontallyAligned )
    {
		Vector3 targetFront = ( m_target - dishPos );
		Vector3 currentFront = worldMat.f;
		targetFront.HorizontalAndNormalise();
		currentFront.HorizontalAndNormalise();
		Vector3 rotationAxis = currentFront ^ targetFront;
		rotationAxis.y /= 4.0f;
		if (fabsf(rotationAxis.y) > M_PI / 3000.0f)
		{
			m_upperMount->m_angVel.y = signf(rotationAxis.y) * sqrtf(fabsf(rotationAxis.y));
		}
		else
		{
            m_horizontallyAligned = true;
			m_upperMount->m_angVel.y = 0.0f;
		}
	}


    //
    // Rotate slowly to face our target (vert)

	m_dish->m_angVel.Zero();

    if( m_horizontallyAligned && !m_verticallyAligned )
    {
        Vector3 targetFront = ( m_target - dishPos );
        targetFront.Normalise();
		float const maxAngle = 0.3f;
		float const minAngle = -0.2f;
		if (targetFront.y > maxAngle) targetFront.y = maxAngle;
		if (targetFront.y < minAngle) targetFront.y = minAngle;
        targetFront.Normalise();
		float amount = worldMat.u * targetFront;
		m_dish->m_angVel = m_dish->m_transform.r * amount;

        m_verticallyAligned = ( m_dish->m_angVel.Mag() < 0.001f );
    }

	m_upperMount->m_transform.RotateAround(m_upperMount->m_angVel * SERVER_ADVANCE_PERIOD);
	m_dish->m_transform.RotateAround(m_dish->m_angVel * SERVER_ADVANCE_PERIOD);

//    if( m_movementSoundsPlaying && m_horizontallyAligned && m_dish->m_angVel.Mag() < 0.05f )
    if( m_movementSoundsPlaying && m_horizontallyAligned && m_verticallyAligned )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "RadarDish BeginRotation" );
        g_app->m_soundSystem->TriggerBuildingEvent( this, "EndRotation" );
        m_movementSoundsPlaying = false;
    }


    //
    // Are there any receiving Radar dishes?

    m_range = 99999.9f;
    bool found = false;

    bool previouslyAligned = ( m_receiverId != -1 );

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
		// Skip empty slots
        if( !g_app->m_location->m_buildings.ValidIndex(i) ) continue;

		// Filter out non radar dish buildings
		Building *building = g_app->m_location->m_buildings.GetData(i);
        if( building->m_type != TypeRadarDish ) continue;

		// Don't compare against ourself
		if( building == this ) continue;

		// Does our "ray" hit their dish
        RadarDish *otherDish = (RadarDish *) building;
        bool hit = RaySphereIntersection( dishPos, dishFront, otherDish->m_centrePos, otherDish->m_radius, 1e9 );
        if( hit )
		{
			// Make sure we aren't hitting the back of the receiving dish
	        Vector3 theirFront = otherDish->GetDishFront(0.0f);
			float dotProd = dishFront * theirFront;
			if (dotProd < 0.0f)
			{
				if (g_app->m_location->IsVisible( dishPos, otherDish->GetDishPos(0.0f)))
				{
					float newRange = (otherDish->GetDishPos(0.0f) - dishPos).Mag();
					if( newRange < m_range )
					{
						m_receiverId = otherDish->m_id.GetUniqueId();
						m_range = newRange;
						m_signal = 1.0f;
						found = true;
					}
				}
			}
		}
    }

    if( previouslyAligned && !found )
    {
        m_range = 0.0f;
        m_signal = 0.0f;
        m_receiverId = -1;
        g_app->m_soundSystem->StopAllSounds( m_id, "RadarDish ConnectionEstablished" );
        g_app->m_soundSystem->TriggerBuildingEvent( this, "ConnectionLost" );

        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_link = -1;
    }

    if( !previouslyAligned && found )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "ConnectionEstablished" );

        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_link = m_receiverId;
    }

    return Teleport::Advance();
}


Vector3 RadarDish::GetDishPos( float _predictionTime )
{
	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
    return worldMat.pos;
}


Vector3 RadarDish::GetDishFront( float _predictionTime )
{
    if( m_receiverId != -1 )
    {
        RadarDish *receiver = (RadarDish *) g_app->m_location->GetBuilding( m_receiverId );
        if( receiver )
        {
            Vector3 ourDishPos = GetDishPos( _predictionTime );
            Vector3 receiverDishPos = receiver->GetDishPos( _predictionTime );
            return ( receiverDishPos - ourDishPos ).Normalise();
        }
    }

	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
    return worldMat.f;
}


void RadarDish::Aim( Vector3 _worldPos )
{
    m_target = _worldPos;

    m_horizontallyAligned = false;
    m_verticallyAligned = false;

    if( m_movementSoundsPlaying )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "RadarDish BeginRotation" );
    }

    g_app->m_soundSystem->TriggerBuildingEvent( this, "BeginRotation" );
    m_movementSoundsPlaying = true;
}


void RadarDish::Render( float _predictionTime )
{
	Building::Render(_predictionTime);
}


#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"
#endif

void RadarDish::RenderAlphas ( float _predictionTime )
{
#ifdef USE_DIRECT3D
	// sets default vertex format (somebody changes it and not returns back, where?)
	// without this code, radar beams are not reflected
	extern LPDIRECT3DVERTEXDECLARATION9 s_pCustomVertexDecl;
	OpenGLD3D::g_pd3dDevice->SetVertexDeclaration( s_pCustomVertexDecl );
#endif
    if( m_signal > 0.0f )
    {
        RenderSignal( _predictionTime, 10.0f, 0.4f );
        RenderSignal( _predictionTime, 9.0f, 0.2f );
        RenderSignal( _predictionTime, 8.0f, 0.2f );
        RenderSignal( _predictionTime, 4.0f, 0.5f );
    }

    Teleport::RenderAlphas(_predictionTime);
}


void RadarDish::RenderSignal( float _predictionTime, float _radius, float _alpha )
{
	START_PROFILE(g_app->m_profiler, "Signal");

    Vector3 startPos = GetStartPoint();
    Vector3 endPos = GetEndPoint();
    Vector3 delta = endPos - startPos;
    Vector3 deltaNorm = delta;
    deltaNorm.Normalise();

    float distance = (startPos - endPos).Mag();
    float numRadii = 20.0f;
    int numSteps = int( distance / 200.0f );
    numSteps = max( 1, numSteps );

    glEnable            (GL_TEXTURE_2D);

    gglActiveTextureARB  (GL_TEXTURE0_ARB);
    glBindTexture	    (GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laserfence.bmp", true, true));
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);
    glEnable            (GL_TEXTURE_2D);

    gglActiveTextureARB  (GL_TEXTURE1_ARB);
    glBindTexture	    (GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/radarsignal.bmp", true, true));
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
    glEnable            (GL_TEXTURE_2D);

    glDisable           (GL_CULL_FACE);
    glBlendFunc         (GL_SRC_ALPHA, GL_ONE);
    glEnable            (GL_BLEND);
    glDepthMask         (false);
    glColor4f           (1.0f,1.0f,1.0f,_alpha);

    glMatrixMode        (GL_MODELVIEW);
#ifdef USE_DIRECT3D
	void SwapToViewMatrix();
	void SwapToModelMatrix();

	SwapToViewMatrix();
#endif
    glTranslatef        ( startPos.x, startPos.y, startPos.z );
    Vector3 dishFront   = GetDishFront(_predictionTime);
    double eqn1[4]      = { dishFront.x, dishFront.y, dishFront.z, -1.0f };
    glClipPlane         (GL_CLIP_PLANE0, eqn1 );

    RadarDish *receiver = (RadarDish *) g_app->m_location->GetBuilding( m_receiverId );
    Vector3 receiverPos = receiver->GetDishPos( _predictionTime );
    Vector3 receiverFront = receiver->GetDishFront( _predictionTime );
    glTranslatef        ( -startPos.x, -startPos.y, -startPos.z );
    glTranslatef        ( receiverPos.x, receiverPos.y, receiverPos.z );

    Vector3 diff = receiverPos - startPos;
    float thisDistance = -(receiverFront * diff);

#ifndef USE_DIRECT3D
    thisDistance = -1.0f;
#endif

    double eqn2[4]      = { receiverFront.x, receiverFront.y, receiverFront.z, thisDistance };
    glClipPlane         (GL_CLIP_PLANE1, eqn2 );
    glTranslatef        ( -receiverPos.x, -receiverPos.y, -receiverPos.z );
    glTranslatef        ( startPos.x, startPos.y, startPos.z );

    glEnable            (GL_CLIP_PLANE0);
    glEnable            (GL_CLIP_PLANE1);

    float texXInner = -g_gameTime/_radius;
    float texXOuter = -g_gameTime;

    //float texXInner = -1.0f/_radius;
    //float texXOuter = -1.0f;

    glBegin( GL_QUAD_STRIP );

    for( int s = 0; s < numSteps; ++s )
    {
        Vector3 deltaFrom = 1.2f * delta * (float) s / (float) numSteps;
        Vector3 deltaTo = 1.2f * delta * (float) (s+1) / (float) numSteps;

        Vector3 currentPos = (-delta*0.1f) + Vector3(0,_radius,0);

        for( int r = 0; r <= numRadii; ++r )
        {
            gglMultiTexCoord2fARB    ( GL_TEXTURE0_ARB, texXInner, r/numRadii );
            gglMultiTexCoord2fARB    ( GL_TEXTURE1_ARB, texXOuter, r/numRadii );
            glVertex3fv             ( (currentPos + deltaFrom).GetData() );

            gglMultiTexCoord2fARB    ( GL_TEXTURE0_ARB, texXInner+10.0f/(float)numSteps, (r)/numRadii );
            gglMultiTexCoord2fARB    ( GL_TEXTURE1_ARB, texXOuter+distance/(200.0f *(float)numSteps), (r)/numRadii );
            glVertex3fv             ( (currentPos + deltaTo).GetData() );

            currentPos.RotateAround( deltaNorm * ( 2.0f * M_PI / (float) numRadii ) );
        }

        texXInner += 10.0f / (float) numSteps;
        texXOuter += distance/(200.0f * (float) numSteps);
    }

    glEnd();

    glTranslatef        ( -startPos.x, -startPos.y, -startPos.z );

#ifdef USE_DIRECT3D
	SwapToModelMatrix();
#endif

    glDisable           (GL_CLIP_PLANE0);
    glDisable           (GL_CLIP_PLANE1);
    glDepthMask         (true);
    glDisable           (GL_BLEND);
    glBlendFunc         (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable            (GL_CULL_FACE);

    gglActiveTextureARB  (GL_TEXTURE1_ARB);
    glDisable           (GL_TEXTURE_2D);
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    gglActiveTextureARB  (GL_TEXTURE0_ARB);
    glDisable           (GL_TEXTURE_2D);
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	END_PROFILE(g_app->m_profiler, "Signal");
}


bool RadarDish::Connected()
{
    return( m_receiverId != -1 && m_signal > 0.0f );
}


int RadarDish::GetConnectedDishId()
{
    return m_receiverId;
}


bool RadarDish::ReadyToSend ()
{
    return( Connected() && Teleport::ReadyToSend() );
}


Vector3 RadarDish::GetStartPoint()
{
    return GetDishPos(0.0f);
}


Vector3 RadarDish::GetEndPoint()
{
    return GetDishPos(0.0f) + GetDishFront(0.0f) * m_range;
}


bool RadarDish::GetExit( Vector3 &_pos, Vector3 &_front )
{
    RadarDish *receiver = (RadarDish *) g_app->m_location->GetBuilding( m_receiverId );
    if( receiver )
    {
        Matrix34 rootMat(receiver->m_front, g_upVector, receiver->m_pos);
        Matrix34 worldMat = receiver->m_entrance->GetWorldMatrix(rootMat);
        _pos = worldMat.pos;
        _front = worldMat.f;
        return true;
    }
    return false;
}


bool RadarDish::UpdateEntityInTransit( Entity *_entity )
{
    Vector3 dishPos = GetDishPos(0.0f);
    Vector3 dishFront = GetDishFront(0.0f);
    Vector3 targetPos = dishPos + dishFront * m_range;
    Vector3 ourOffset = ( targetPos - _entity->m_pos );

    WorldObjectId id( _entity->m_id );

    _entity->m_vel = dishFront * RADARDISH_TRANSPORTSPEED;
    _entity->m_pos += _entity->m_vel * SERVER_ADVANCE_PERIOD;
    _entity->m_onGround = false;
    _entity->m_enabled = false;

    if( _entity->m_id.GetUnitId() != -1 )
    {
        Unit *unit = g_app->m_location->GetUnit( _entity->m_id );
        unit->UpdateEntityPosition( _entity->m_pos, _entity->m_radius );
    }

    float distTravelled = ( _entity->m_pos - dishPos ).Mag();

    if( m_signal == 0.0f )
    {
        // Shit - we lost the carrier signal, so we die
        _entity->ChangeHealth( -500 );
        _entity->m_enabled = true;
        _entity->m_vel += Vector3(syncsfrand(10.0f), syncfrand(10.0f), syncsfrand(10.0f) );

        g_app->m_location->m_entityGrid->AddObject( id, _entity->m_pos.x, _entity->m_pos.z, _entity->m_radius );
        return true;
    }
    else if( distTravelled >= m_range )
    {
        // We are there
        Vector3 exitPos, exitFront;
        GetExit( exitPos, exitFront );
        _entity->m_pos = exitPos;
        _entity->m_front = exitFront;
        _entity->m_enabled = true;
        _entity->m_onGround = true;
        _entity->m_vel.Zero();

        g_app->m_location->m_entityGrid->AddObject( id, _entity->m_pos.x, _entity->m_pos.z, _entity->m_radius );

        g_app->m_soundSystem->TriggerEntityEvent( _entity, "ExitTeleport" );
        return true;
    }

    return false;
}


void RadarDish::ListSoundEvents ( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "BeginRotation" );
    _list->PutData( "EndRotation" );
    _list->PutData( "ConnectionEstablished" );
    _list->PutData( "ConnectionLost" );
}


bool RadarDish::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    // This method is overridden for Radar Dish in order to expand the number
    // of cells the Radar Dish is placed into.  We were having problems where
    // entities couldn't get into the radar dish because its door was right on
    // the edge of an obstruction grid cell, so the entity didn't see the
    // building at all

    SpherePackage sphere(_pos, _radius * 1.5f);
    Matrix34 transform(m_front, m_up, m_pos);
    return m_shape->SphereHit(&sphere, transform);
}