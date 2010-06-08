#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/3d_sprite.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"

#include "worldobject/lightning.h"

#include "main.h"
#include "globals.h"
#include "app.h"
#include "location.h"
#include "camera.h"
#include "particle_system.h"
#include "entity_grid.h"
#include "obstruction_grid.h"

#include "worldobject/darwinian.h"
#include "worldobject/tree.h"


static std::vector<WorldObjectId> s_neighbours;


// ****************************************************************************
// Class lightning
// ****************************************************************************
  
Lightning::Lightning()
:   WorldObject(),
    m_striking(false),
    m_life(0.0)
{    
    m_type = EffectLightning;
}
    

bool Lightning::Advance()
{
    if( m_life > 0.0 )
    {
        m_life -= SERVER_ADVANCE_PERIOD;
        if( m_life <= 0.0 ) return true;
    }

    if( syncfrand( 1.0 ) < 0.01 )
    {
        m_striking = true;
        m_life = 1.2;

        Vector3 pos = m_pos;
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );

        for( int i = 0; i < 5; ++i )
        {
             Vector3 vel( sfrand( 60.0 ), 
                     60.0 + frand( 60.0 ), 
                     sfrand( 60.0 ) );
            double size = 150.0 + frand(100.0);
            g_app->m_particleSystem->CreateParticle( pos + g_upVector * 5.0, vel, 
    											     Particle::TypeExplosionDebris, size );
        }

        for( int i = 0; i < 10; ++i )
        {
            Vector3 fireSpawn = pos + g_upVector * frand(5);
            double fireSize = 160 + frand(120.0);
            Vector3 vel( sfrand( 60.0 ), 
                     5.0 + frand( 5.0 ), 
                     sfrand( 60.0 ) );

            int particleType = Particle::TypeFire;
            g_app->m_particleSystem->CreateParticle( fireSpawn, vel, particleType, fireSize );
        }

        // set any darwinians unfortunate enough to be too close on fire
        int numFound;
        g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, 30.0, &numFound );
        for( int i = 0; i < numFound; ++i )
        {
            Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
            if( d )
            {
                d->SetFire();
            }
        }

        // if we hit a tree, set fire to it
        LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );
        for( int b = 0; b < buildings->Size(); ++b )
        {
            int buildingId = buildings->GetData(b);
            Building *building = g_app->m_location->GetBuilding( buildingId );
            if( building )
            {
                if( building->m_type == Building::TypeTree )
                {
                    Tree *tree = (Tree *)building;
                    tree->Damage( -100.0 );
                }
            }
        }
    }

    return false;
}


void Lightning::Render( double _predictionTime )
{
    if( m_striking )
    {
        _predictionTime -= SERVER_ADVANCE_PERIOD;
        
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/lightning.bmp" ) );
        glEnable(GL_TEXTURE_2D);
	    glDisable(GL_CULL_FACE);
	    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

        Vector3 up = g_upVector;
        Vector3 right = g_app->m_camera->GetRight();
        double alpha = iv_abs(iv_sin(m_life - _predictionTime));

        Vector3 pos = m_pos;
        double size = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
        double width = 30.0;
        
        for( int i = 0; i < 3; ++i )
        {
            glColor4f ( 1.0, 1.0, 1.0, alpha );
            glBegin(GL_QUADS);
                glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * width - up * size).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * width ).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * width ).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * width - up * size).GetData() );  
            glEnd();
            alpha /= 2.0;
            width *= 1.2;
        }

        glDisable(GL_TEXTURE_2D);
	    glEnable(GL_CULL_FACE);
    }
    else
    {        
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
        glEnable(GL_TEXTURE_2D);
	    glDisable(GL_CULL_FACE);
	    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

        Vector3 right = g_app->m_camera->GetRight();
        Vector3 up = g_upVector ^ right;
        double alpha = iv_abs(iv_sin(g_gameTime * 2.0));

        Vector3 pos = m_pos;
        double size = 200.0;

        glColor4f ( 1.0, 1.0, 1.0, alpha );
        glBegin(GL_QUADS);
            glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * size - up * size).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * size + up * size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * size + up * size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * size - up * size).GetData() );  
        glEnd();

        glDisable(GL_TEXTURE_2D);
	    glEnable(GL_CULL_FACE);
    }
}

