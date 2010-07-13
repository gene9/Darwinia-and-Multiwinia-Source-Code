#include "lib/universal_include.h"

#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "renderer.h"

#include "worldobject/spiritstore.h"


SpiritStore::SpiritStore()
:   m_sizeX(0.0),
    m_sizeY(0.0),
    m_sizeZ(0.0)
{
}

void SpiritStore::Initialise ( int _initialCapacity, int _maxCapacity, Vector3 _pos, 
                                double _sizeX, double _sizeY, double _sizeZ )
{
    m_spirits.SetSize( _maxCapacity );
    m_spirits.SetStepSize( _maxCapacity / 2 );
    m_pos = _pos;
    m_sizeX = _sizeX;
    m_sizeY = _sizeY;
    m_sizeZ = _sizeZ;

    for( int i = 0; i < _initialCapacity; ++i )
    {
        Spirit s;
        s.m_teamId = 0;
        s.Begin();
        AddSpirit( &s );
    }
}

void SpiritStore::Advance()
{
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *s = m_spirits.GetPointer(i);            
            s->Advance();
        }
    }
}

void SpiritStore::Render( double _predictionTime )
{
	START_PROFILE( "Spirit Store");

    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );
    glDisable       ( GL_CULL_FACE );

    glColor4f       ( 1.0, 1.0, 1.0, 0.5 );

    glEnable        (GL_TEXTURE_2D);
    glBindTexture	(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/triangleOutline.bmp", true, false));
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    glBegin(GL_QUADS);
        glNormal3f(0,1,0);
        glTexCoord2f( 0.0, 0.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0, 0.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0, 1.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 0.0, 1.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);

        glNormal3f(0,0,-1);
        glTexCoord2f( 0.0, 0.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0, 0.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0, 1.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 0.0, 1.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);

        glNormal3f(0,0,1);
        glTexCoord2f( 0.0, 0.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0, 0.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0, 1.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 0.0, 1.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);

        glNormal3f(1,0,0);
        glTexCoord2f( 0.0, 0.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0, 0.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 1.0, 1.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 0.0, 1.0 );     glVertex3f(m_pos.x + m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);

        glNormal3f(-1,0,0);
        glTexCoord2f( 0.0, 0.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0, 0.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z + m_sizeZ);
        glTexCoord2f( 1.0, 1.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y + m_sizeY, m_pos.z - m_sizeZ);
        glTexCoord2f( 0.0, 1.0 );     glVertex3f(m_pos.x - m_sizeX, m_pos.y - m_sizeY, m_pos.z - m_sizeZ);
    glEnd();

    glDisable       ( GL_TEXTURE_2D );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *s = m_spirits.GetPointer(i);
            s->Render( _predictionTime );
        }
    }

    glDisable       ( GL_BLEND );
    glEnable        ( GL_CULL_FACE );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask     ( true );
	
	END_PROFILE( "Spirit Store");
}


int SpiritStore::NumSpirits ()
{
    return m_spirits.NumUsed();
}


void SpiritStore::AddSpirit ( Spirit *_spirit )
{
    Spirit *target = m_spirits.GetPointer();
    *target = *_spirit;
    target->m_pos = m_pos + Vector3(syncsfrand(m_sizeX*1.5),
                                    syncsfrand(m_sizeY*1.5),
                                    syncsfrand(m_sizeZ*1.5) );
    target->m_state = Spirit::StateInStore;
    target->m_numNearbyEggs = 0;
}

void SpiritStore::RemoveSpirits ( int _quantity )
{
    int numRemoved = 0;
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            m_spirits.RemoveData(i);
            ++numRemoved;
            if( numRemoved == _quantity ) return;
        }
    }
}
