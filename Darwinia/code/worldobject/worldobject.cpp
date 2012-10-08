#include "lib/universal_include.h"

#include <math.h>
#include <stdio.h>

#include "lib/debug_utils.h"
#include "lib/math_utils.h"

#include "app.h"
#include "location.h"
#include "worldobject.h"
#include "main.h"


#define COEF_OF_RESTITUTION	0.85f


// ****************************************************************************
//  WorldObjectId
// ****************************************************************************

int WorldObjectId::s_nextUniqueId = 0;

#define ID_MAXTEAMS     256
#define ID_MAXUNITS     65536
#define ID_MAXTROOPS    65536

WorldObjectId::WorldObjectId()
:   m_teamId(255),
	m_oldTeam(255),
    m_unitId(-1),
    m_index(-1),
    m_uniqueId(-1)
{
}


WorldObjectId::WorldObjectId( unsigned char _teamId,
                              int _unitId,
                              int _index,
                              int _uniqueId )
{
    Set( _teamId, _unitId, _index, _uniqueId );
}


void WorldObjectId::Set( unsigned char _teamId,
                         int _unitId,
                         int _index,
                         int _uniqueId )
{
    DarwiniaDebugAssert( _teamId < ID_MAXTEAMS );
    DarwiniaDebugAssert( _unitId < ID_MAXUNITS );
    DarwiniaDebugAssert( _index < ID_MAXTROOPS );

    m_teamId = _teamId;
    m_unitId = _unitId;
    m_index = _index;
    m_uniqueId = _uniqueId;
}


void WorldObjectId::SetInvalid()
{
    m_teamId = 255;
    m_unitId = -1;
    m_index = -1;
    m_uniqueId = -1;
}


bool WorldObjectId::operator != (WorldObjectId const &w) const
{
	return ( m_teamId != w.m_teamId ||
             m_unitId != w.m_unitId ||
             m_index != w.m_index ||
             m_uniqueId != w.m_uniqueId );
}


bool WorldObjectId::operator == (WorldObjectId const &w) const
{
	return ( m_teamId == w.m_teamId &&
             m_unitId == w.m_unitId &&
             m_index == w.m_index &&
             m_uniqueId == w.m_uniqueId );
}


WorldObjectId const &WorldObjectId::operator = (WorldObjectId const &w)
{
    m_teamId = w.m_teamId;
    m_unitId = w.m_unitId;
    m_index = w.m_index;
    m_uniqueId = w.m_uniqueId;
    return *this;
}


void WorldObjectId::GenerateUniqueId()
{
    ++s_nextUniqueId;
    m_uniqueId = s_nextUniqueId;
}


// ****************************************************************************
//  Class WorldObject
// ****************************************************************************

// *** Constructor
WorldObject::WorldObject()
:   m_onGround(false),
    m_enabled(true),
    m_type(0)
{
}


WorldObject::~WorldObject()
{
}


// *** BounceOffLandscape
void WorldObject::BounceOffLandscape()
{
	// Assume that we were above the landscape last frame and that we are not
	// now. We know that we must have impacted the landscape somewhere we
	// are now and where we were last frame. Let's use the midpoint of our last
	// and our current position as the point of impact (it will be correct on
	// average)
	Vector3 lastPos = m_pos;// - m_vel * g_advanceTime;
	Vector3 impactPos = (m_pos + lastPos) * 0.5f;
	m_pos = impactPos;
	m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

    Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
    Vector3 incomingVel = m_vel * -1.0f;
    float dotProd = normal * incomingVel;
    m_vel = 2.0f * dotProd * normal - incomingVel;
    m_vel *= COEF_OF_RESTITUTION;
}


bool WorldObject::Advance()
{
    return false;
}


void WorldObject::Render( float _time )
{
}


bool WorldObject::RenderPixelEffect( float predictionTime )
{
    return false;
}



// ****************************************************************************
//  Class Light
// ****************************************************************************

// *** Constructor
Light::Light()
{
    m_colour[0] = 1.3f;
    m_colour[1] = 1.3f;
    m_colour[2] = 1.3f;
    m_colour[3] = 0.0f;

    SetFront( Vector3(0,0,1) );
}


void Light::SetColour(float colour[4])
{
	m_colour[0] = colour[0];
	m_colour[1] = colour[1];
	m_colour[2] = colour[2];
	m_colour[3] = colour[3];
}


void Light::SetFront(float front[4])
{
	m_front[0] = front[0];
	m_front[1] = front[1];
	m_front[2] = front[2];
	m_front[3] = front[3];
}


void Light::SetFront(Vector3 front)
{
	m_front[0] = front.x;
	m_front[1] = front.y;
	m_front[2] = front.z;
	m_front[3] = 0.0f;
}


void Light::Normalise()
{
	float mag = sqrtf(m_front[0] * m_front[0] +
					  m_front[1] * m_front[1] +
					  m_front[2] * m_front[2]);
	m_front[0] /= mag;
	m_front[1] /= mag;
	m_front[2] /= mag;
}