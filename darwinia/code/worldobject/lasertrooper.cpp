#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"

#include "app.h"
#include "location.h"
#include "team.h"
#include "unit.h"
#include "user_input.h"
#include "entity_grid.h"
#include "camera.h"

#include "worldobject/lasertrooper.h"


// *** Advance

/*
bool LaserTrooper::Advance(Unit *_unit)
{    
    Vector3 oldPos = m_pos;
    bool aiControlled = (g_app->m_location->m_teams[ m_teamId ].m_teamType == Team::TeamTypeAI);
    
    if( aiControlled )
    {
        m_obeyUnit -= syncfrand(100.0f) * SERVER_ADVANCE_PERIOD;
        if( m_obeyUnit < -100.0f )
        {
            m_obeyUnit = 100.0f;
        }
    }

    if( !m_dead && m_onGround && m_inWater < 0.0f )
    {        
        Vector3 targetPos = _unit->GetWayPoint();
        targetPos.y = 0.0f;
        Vector3 offset = _unit->GetOffset(Unit::FormationRectangle, m_formationIndex );
        targetPos += offset;
        targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z );
        targetPos = PushFromObstructions( targetPos );
        //targetPos = PushFromEachOther( targetPos );
            
        m_targetPos = targetPos;

//		Vector3 targetPos = _unit->GetLeadVector(&m_wayPointId, 0.0f, m_pos);
        Vector3 targetVector = targetPos - m_pos;
        //targetVector = PushFromEachOther( targetVector );
        targetVector.y = 0.0f;
        
        if( !aiControlled || m_obeyUnit != 0.0f )
        {
			float distance = targetVector.Mag();
			
			// If moving towards way point...
			if (distance > 0.01f)
			{
				m_wobble += SERVER_ADVANCE_PERIOD * 15.0f;
				if (m_wobble > M_PI * 2.0f) m_wobble -= M_PI * 2.0f;

				m_vel = targetVector / distance;
				m_vel = PushFromEachOther( m_vel );
                m_vel.Normalise();
                m_vel *= m_stats[StatSpeed];
                
                //
                // Slow us down if we're going up hill
                // Speed up if going down hill

                Vector3 nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
                nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);
                float currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
                float nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( nextPos.x, nextPos.z );
                float factor = 1.0f - (currentHeight - nextHeight) / -3.0f;
                if( factor < 0.01f ) factor = 0.01f;
                if( factor > 2.0f ) factor = 2.0f;
                m_vel *= factor;

				if (distance < m_stats[StatSpeed] * SERVER_ADVANCE_PERIOD)
				{                    
					m_vel.SetLength(distance / SERVER_ADVANCE_PERIOD);
				}
				m_pos += m_vel * SERVER_ADVANCE_PERIOD;
				m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) + 0.1f;

                if( targetVector.Mag() < 1.0f )
                {
                    targetVector = _unit->GetWayPoint() - m_pos;
                }

				targetVector.HorizontalAndNormalise();                
				m_angVel = (targetVector ^ m_front) * 8.0f;
				float const maxTurnRate = 10.0f; // radians per second
				if (m_angVel.Mag() > maxTurnRate) 
				{
					m_angVel.SetLength(maxTurnRate);
				}
				m_front.RotateAround(m_angVel * SERVER_ADVANCE_PERIOD);
			}
			else // otherwise we are standing on the spot
			{
				Team *team = &g_app->m_location->m_teams[m_teamId];

				// Does this entity belong to the currently selectedUnit
				if (m_unitId == team->m_currentUnitId)
				{
					Vector3 toMouse = g_app->m_userInput->GetMousePos3d() - m_pos;
					toMouse.HorizontalAndNormalise();
					m_angVel = (toMouse ^ m_front) * 4.0f;
				}
				else
				{
					m_angVel = (Vector3(1,0,0) ^ m_front) * 4.0f;
				}

				float const maxTurnRate = 10.0f; // radians per second
				if (m_angVel.Mag() > maxTurnRate) 
				{
					m_angVel.SetLength(maxTurnRate);
				}
				m_front.RotateAround(m_angVel * SERVER_ADVANCE_PERIOD);
				m_front.Normalise();
                m_vel.Zero();
			}
        }
        else
        {
            m_vel.Zero();
        }

        if( m_pos.y <= 0.0f )
        {
            m_inWater = syncfrand(3.0f);
        }
    }
    
	if( !m_onGround ) AdvanceInAir(_unit);
    
    bool enteredTeleport = EnterTeleports();
    if( enteredTeleport )
    {
        return true;
    }

    return Entity::Advance( _unit );
}
*/

void LaserTrooper::Begin()
{
    Entity::Begin();

    m_victoryDance = -1.0f;
}


bool LaserTrooper::Advance( Unit *_unit )
{
    if( m_targetPos == g_zeroVector ) m_targetPos = m_pos;

    if( m_enabled && m_onGround && !m_dead )
    {
        Vector3 movementDir = ( m_targetPos - m_pos ).Normalise();
        float distance = ( m_targetPos - m_pos ).Mag();
        float speed = m_stats[StatSpeed];
        if( speed * SERVER_ADVANCE_PERIOD > distance ) speed = distance/SERVER_ADVANCE_PERIOD;
        m_vel = movementDir * speed;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
        m_front = m_vel;
        m_front.Normalise();
    
        if( EnterTeleports() )  return true;    
    }

	if( !m_onGround )       AdvanceInAir(_unit);
    if( m_inWater != -1 )   AdvanceInWater(_unit);

    if( m_reloading > 0.0f )
    {
        m_reloading -= SERVER_ADVANCE_PERIOD;
        if( m_reloading < 0.0f ) m_reloading = 0.0f;
    }

    if( m_dead ) 
    {
        bool amIDead = AdvanceDead( _unit );
        if( amIDead ) return true;
    }

    if( m_victoryDance != -1.0f )
    {
        AdvanceVictoryDance();
    }

    _unit->UpdateEntityPosition( m_pos, m_radius );    

    return false;
}


void LaserTrooper::AdvanceVictoryDance()
{
    if( syncfrand(100.0f) < 1.0f )
    {
        m_vel.Zero();
        m_vel.y += 10.0f + syncfrand(10.0f);
        m_onGround = false;
    }
}


void LaserTrooper::Render( float predictionTime, int teamId )
{         
    if( !m_enabled ) return;
    
    //
    // Work out our predicted position and orientation

    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    if( m_onGround && m_inWater==-1 )
    {
        predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
    }

    float size = 2.0f;
        
    Vector3 entityUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
//	float wobble = m_wobble;
//    if( !m_onGround ) wobble = 0.0f;
//	if ( m_vel.Mag() > 0.01f )
//	{
//		wobble += predictionTime * 15.0f;
//	}
//	entityUp.FastRotateAround(m_front, sinf(wobble) * 0.1f);
    entityUp.Normalise();
    Vector3 entityFront = m_front;
	entityFront.RotateAround(m_angVel * predictionTime);

//	Matrix34 transform(entityFront, entityUp, predictedPos);
//	Shape *aShape = g_app->m_resource->GetShape("laser_troop.shp");
//	aShape->Render(predictionTime, transform);

    Vector3 entityRight = entityFront ^ entityUp;
	entityFront *= 0.2f;
	entityUp *= size * 2.0f;
    entityRight.SetLength(size);

    if( m_justFired )
    {
        m_justFired = false;
    }

    //glDisable( GL_TEXTURE_2D );
    //RenderSphere( m_targetPos, 1.0f, RGBAColour(255,255,255,100) );
    //RenderSphere( m_unitTargetPos, 1.1f, RGBAColour(255,155,155,200) );
    //glEnable( GL_TEXTURE_2D );
    
    if( !m_dead )
    {
        RGBAColour colour = g_app->m_location->m_teams[ teamId ].m_colour;
        
        if( m_reloading > 0.0f )
        {
            colour.r *= 0.7f;
            colour.g *= 0.7f;
            colour.b *= 0.7f;
        }

        //
        // Draw our texture

        float maxHealth = EntityBlueprint::GetStat(m_type, StatHealth);
        float health = (float) m_stats[StatHealth] / maxHealth;
        if( health > 1.0f ) health = 1.0f;

		colour *= 0.5f + 0.5f * health;
        glColor3ubv(colour.GetData());
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f);     glVertex3fv( (predictedPos - entityRight + entityUp).GetData() );
            glTexCoord2f(1.0f, 1.0f);     glVertex3fv( (predictedPos + entityRight + entityUp).GetData() );
            glTexCoord2f(1.0f, 0.0f);     glVertex3fv( (predictedPos + entityRight).GetData() );
            glTexCoord2f(0.0f, 0.0f);     glVertex3fv( (predictedPos - entityRight).GetData() );
        glEnd();


        //
        // Draw our shadow on the landscape

        if( m_onGround )
        {
            glColor4ub( 0, 0, 0, 90 );
            Vector3 pos1 = predictedPos - entityRight;
            Vector3 pos2 = predictedPos + entityRight;
            Vector3 pos4 = pos1 + Vector3( 0.0f, 0.0f, size * 2.0f );
            Vector3 pos3 = pos2 + Vector3( 0.0f, 0.0f, size * 2.0f );

            pos1.y = 0.2f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos1.x, pos1.z );
            pos2.y = 0.2f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos2.x, pos2.z );
            pos3.y = 0.2f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos3.x, pos3.z );
            pos4.y = 0.2f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos4.x, pos4.z );

            glLineWidth( 1.0f );
            glBegin( GL_QUADS );
                glTexCoord2f(0.0f, 0.0f);       glVertex3fv(pos1.GetData());
                glTexCoord2f(1.0f, 0.0f);       glVertex3fv(pos2.GetData());
                glTexCoord2f(1.0f, 1.0f);       glVertex3fv(pos3.GetData());
                glTexCoord2f(0.0f, 1.0f);       glVertex3fv(pos4.GetData());
            glEnd();
        }


        //
        // Draw a line through us if we are side-on with the camera

		float alpha = 1.0f - fabsf(g_app->m_camera->GetFront() * m_front);
        if( alpha > 0.5f )
        {
            //colour.a = 255;
            colour.a = 255 * alpha;
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            glColor4ubv(colour.GetData());
			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 1.0f);     glVertex3fv( (predictedPos - entityFront*1.5f + entityUp).GetData() );
				glTexCoord2f(1.0f, 1.0f);     glVertex3fv( (predictedPos + entityFront*1.5f + entityUp).GetData() );
				glTexCoord2f(1.0f, 0.0f);     glVertex3fv( (predictedPos + entityFront*1.5f).GetData() );
				glTexCoord2f(0.0f, 0.0f);     glVertex3fv( (predictedPos - entityFront*1.5f).GetData() );
			glEnd();
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }
                   
    }
    else
    {
        entityFront = m_front;
        entityFront.Normalise();
        entityUp = g_upVector;
        entityRight = entityFront ^ entityUp;
		entityUp *= size * 2.0f;
        entityRight.Normalise();
		entityRight *= size;
        unsigned char alpha = (float)m_stats[StatHealth] * 2.55f;
       
        glColor4ub( 0, 0, 0, alpha );

        entityRight *= 0.5f;
        entityUp *= 0.5;
        float predictedHealth = m_stats[StatHealth];
        if( m_onGround ) predictedHealth -= 40 * predictionTime;
        else             predictedHealth -= 20 * predictionTime;
        float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );

        for( int i = 0; i < 3; ++i )
        {
            Vector3 fragmentPos = predictedPos;
            if( i == 0 ) fragmentPos.x += 10.0f - predictedHealth / 10.0f;
            if( i == 1 ) fragmentPos.z += 10.0f - predictedHealth / 10.0f;
            if( i == 2 ) fragmentPos.x -= 10.0f - predictedHealth / 10.0f;            
            fragmentPos.y += ( fragmentPos.y - landHeight ) * i * 0.5f;
            
            
            float left = 0.0f;
            float right = 1.0f;
            float top = 1.0f;
            float bottom = 0.0f;

            if( i == 0 )
            {
                right -= (right-left)/2;
                bottom -= (bottom-top)/2;
            }
            if( i == 1 )
            {
                left += (right-left)/2;
                bottom -= (bottom-top)/2;
            }
            if( i == 2 )
            {
                top += (bottom-top)/2;
                left += (right-left)/2;
            }

            glBegin(GL_QUADS);
                glTexCoord2f(left, bottom);     glVertex3fv( (fragmentPos - entityRight + entityUp).GetData() );
                glTexCoord2f(right, bottom);    glVertex3fv( (fragmentPos + entityRight + entityUp).GetData() );
                glTexCoord2f(right, top);       glVertex3fv( (fragmentPos + entityRight).GetData() );
                glTexCoord2f(left, top);        glVertex3fv( (fragmentPos - entityRight).GetData() );
            glEnd();
        }
    }
}
