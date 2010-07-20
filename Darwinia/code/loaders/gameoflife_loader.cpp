#include "lib/universal_include.h"
#include "lib/math_utils.h"
#include "lib/input/input.h"
#include "lib/resource.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/language_table.h"

#include "loaders/gameoflife_loader.h"

#include "app.h"
#include "renderer.h"

#include "sound/soundsystem.h"


#define CELLSIZE        20.0f
#define MAXAGE          50.0f
#define MAXSPAWNAGE     30.0f
#define MAXSPIRITAGE    20.0f

#ifdef USE_DIRECT3D
// Enable GL Tracing
void glTraceEnable( bool _enable);
#endif // USE_DIRECT3D

GameOfLifeLoader::GameOfLifeLoader( bool _glow )
:   Loader(),
    m_glow(_glow)
{
#ifdef USE_DIRECT3D
	glTraceEnable(true);
#endif // USE_DIRECT3D

    m_numCellsX = 128;
    m_numCellsY = int( m_numCellsX * (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW() );

    m_cells = new int[ m_numCellsX * m_numCellsY ];
    m_cellsTemp = new int[ m_numCellsX * m_numCellsY ];
    m_age = new int[ m_numCellsX * m_numCellsY ];

    ClearCells();
    RandomiseCells();

    m_zoom = 1.0f;
    m_speed = 0.1f;
    m_numFound = false;
    m_allDead = -1.0f;
    m_totalAge = 0;

    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

#ifdef USE_DIRECT3D
	glTraceEnable(false);
#endif // USE_DIRECT3D
}


void GameOfLifeLoader::ClearCells()
{
    memset( m_cells, 0, m_numCellsX * m_numCellsY * sizeof(int) );
    memset( m_age, 0, m_numCellsX * m_numCellsY * sizeof(int) );
}



void GameOfLifeLoader::RandomiseCells()
{
    for( int x = 0; x < m_numCellsX; ++x )
    {
        for( int y = 0; y < m_numCellsY; ++y )
        {
            if( syncrand() % 10 < 2 )
            {
                m_cells[ y * m_numCellsX + x] = CellStateAlive;
            }
        }
    }
}


void GameOfLifeLoader::PropagateCells( float _start, float _end )
{
    float startY = ceil(m_numCellsY * _start);
    float endY = m_numCellsY * _end;

    for( int x = 0; x < m_numCellsX; ++x )
    {
        for( int y = startY; y < endY; ++y )
        {
            int numNeighbours = 0;
            int numDead = 0;
            for( int i = -1; i <= 1; ++i )
            {
                for( int j = -1; j <= 1; ++j )
                {
                    int cellX = x + i;
                    int cellY = y + j;

                    if( ( i != 0 || j != 0 ) &&
                        cellX >= 0 && cellX < m_numCellsX &&
                        cellY >= 0 && cellY < m_numCellsY )
                    {

                        if( m_cells[cellY * m_numCellsX + cellX] == CellStateAlive &&
                            m_age[cellY * m_numCellsX + cellX] < MAXSPAWNAGE )
                        {
                            ++numNeighbours;
                            m_numFound++;
                        }
                        if( m_cells[cellY * m_numCellsX + cellX] == CellStateDead )
                        {
                            ++numDead;
                        }
                    }
                }
            }

            int prevCell = m_cells[y * m_numCellsX + x];
            int prevAge = m_age[y * m_numCellsX + x];

            if( prevCell != CellStateAlive && numNeighbours == 3 )
            {
                m_cellsTemp[y * m_numCellsX + x] = CellStateAlive;
                m_age[y * m_numCellsX + x] = 0;
            }
            else if( prevCell == CellStateAlive && numNeighbours <= 3 && numNeighbours > 1 && prevAge < MAXAGE )
            {
                m_cellsTemp[y * m_numCellsX + x] = CellStateAlive;
                m_age[y * m_numCellsX + x]++;
            }
            else if( prevCell == CellStateAlive )
            {
                m_cellsTemp[y * m_numCellsX + x] = CellStateDead;
                m_age[y * m_numCellsX + x] = 0;
            }
            else if( prevCell == CellStateDead && prevAge < MAXSPIRITAGE )
            {
                m_cellsTemp[y *m_numCellsX + x] = CellStateDead;
                m_age[y * m_numCellsX + x]++;
            }
        }
    }
}


void GameOfLifeLoader::CommitCells()
{
    memcpy( m_cells, m_cellsTemp, m_numCellsX * m_numCellsY * sizeof(int) );
    memset( m_cellsTemp, 0, m_numCellsX * m_numCellsY * sizeof(int) );
}


void GameOfLifeLoader::RenderDarwinian( int _cellX, int _cellY, int _age )
{
    float alpha = 1.0f - ( _age / MAXAGE );
    alpha = max( alpha, 0.0f );

    glColor4f( 0.4f, 1.0f, 0.4f, alpha );

    float screenX = _cellX * CELLSIZE;
    float screenY = _cellY * CELLSIZE;

    screenX -= m_numCellsX * CELLSIZE * 0.5f;
    screenY -= m_numCellsY * CELLSIZE * 0.5f;

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2f( screenX, screenY );
        glTexCoord2i(1,1);      glVertex2f( screenX+CELLSIZE, screenY );
        glTexCoord2i(1,0);      glVertex2f( screenX+CELLSIZE, screenY+CELLSIZE );
        glTexCoord2i(0,0);      glVertex2f( screenX, screenY+CELLSIZE );
    glEnd();

    if( m_glow )
    {
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
        alpha *= m_zoom;
        alpha = min( alpha, 1.0f );
        glColor4f( 0.4f, 1.0f, 0.4f, alpha*m_zoom );
        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( screenX-CELLSIZE*2, screenY-CELLSIZE*2 );
            glTexCoord2i(1,1);      glVertex2f( screenX+CELLSIZE*3, screenY-CELLSIZE*2 );
            glTexCoord2i(1,0);      glVertex2f( screenX+CELLSIZE*3, screenY+CELLSIZE*3 );
            glTexCoord2i(0,0);      glVertex2f( screenX-CELLSIZE*2, screenY+CELLSIZE*3 );
        glEnd();
    }

    glDisable( GL_TEXTURE_2D );
}


void GameOfLifeLoader::RenderSpirit( int _cellX, int _cellY, int _age )
{
    float alpha = 1.0f - ( _age / MAXSPIRITAGE );
    if( m_glow ) alpha /= m_zoom*10.0f;
    alpha = max( alpha, 0.0f );

    glColor4f( 0.4f, 1.0f, 0.4f, alpha * 0.1f );

    float screenX = _cellX * CELLSIZE;
    float screenY = _cellY * CELLSIZE;

    screenX -= m_numCellsX * CELLSIZE * 0.5f;
    screenY -= m_numCellsY * CELLSIZE * 0.5f;

    screenX += CELLSIZE * 0.5f;
    screenY += CELLSIZE * 0.5f;
    float size = CELLSIZE * 1.0f;

    for( int i = 0; i < 2; ++i )
    {
        size *= 0.33f;
        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( screenX, screenY-size );
            glTexCoord2i(1,1);      glVertex2f( screenX+size, screenY );
            glTexCoord2i(1,0);      glVertex2f( screenX, screenY+size );
            glTexCoord2i(0,0);      glVertex2f( screenX-size, screenY );
        glEnd();
    }

    if( m_glow )
    {
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
        alpha = 1.0f - ( _age / MAXSPIRITAGE );
        alpha *= m_zoom * 0.03f;
        alpha = min( alpha, 1.0f );
        glColor4f( 0.4f, 1.0f, 0.4f, alpha );
        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( screenX-CELLSIZE*1, screenY-CELLSIZE*1 );
            glTexCoord2i(1,1);      glVertex2f( screenX+CELLSIZE*2, screenY-CELLSIZE*1 );
            glTexCoord2i(1,0);      glVertex2f( screenX+CELLSIZE*2, screenY+CELLSIZE*2 );
            glTexCoord2i(0,0);      glVertex2f( screenX-CELLSIZE*1, screenY+CELLSIZE*2 );
        glEnd();
        glDisable( GL_TEXTURE_2D );
    }
}


void GameOfLifeLoader::SetupScreen( float _screenW, float _screenH )
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-_screenW/2.0f, _screenW/2.0f, _screenH/2.0f, -_screenH/2.0f);
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
}


void GameOfLifeLoader::RenderHelp()
{
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    SetupFor2D( 800 );

    g_gameFont.DrawText2DCentre( 400, 50, 50, LANGUAGEPHRASE("bootloader_life") );

    float y = 150;
    float h = 12;
    float dh = 15;

    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_1") );
    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_2") );
    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_3") );

    y+=dh*2;

    g_gameFont.DrawText2D( 100, y+=dh, h, LANGUAGEPHRASE("bootloader_life_4") );
    g_gameFont.DrawText2D( 100, y+=dh, h, LANGUAGEPHRASE("bootloader_life_5") );
    g_gameFont.DrawText2D( 100, y+=dh, h, LANGUAGEPHRASE("bootloader_life_6") );
    g_gameFont.DrawText2D( 100, y+=dh, h, LANGUAGEPHRASE("bootloader_life_7") );
    g_gameFont.DrawText2D( 100, y+=dh, h, LANGUAGEPHRASE("bootloader_life_8") );

    y+=dh*3;

    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_9") );
    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_10") );
    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_11") );
    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_12") );

    y+=dh;

    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_13") );
    g_gameFont.DrawText2DCentre( 400, y+=dh, h, LANGUAGEPHRASE("bootloader_life_14") );

    y+=dh*7;

    g_gameFont.DrawText2DCentre( 400, y, h, LANGUAGEPHRASE("bootloader_life_15") );
    g_gameFont.DrawText2DCentre( 400, y+20, h, LANGUAGEPHRASE("bootloader_life_16a") );
    g_gameFont.DrawText2DCentre( 450, y+20, h, LANGUAGEPHRASE("bootloader_life_16b") );

}


void GameOfLifeLoader::Run()
{
#ifdef USE_DIRECT3D
	glTraceEnable(true);
#endif // USE_DIRECT3D
	FlipBuffers();

    g_app->m_soundSystem->TriggerOtherEvent( NULL, "LoaderGameOfLife", SoundSourceBlueprint::TypeMusic );

    float startTime = GetHighResTime();
    float lastFrameTime = GetHighResTime();
    float lastPropagation = 0.0f;

    while( !g_inputManager->controlEvent( ControlSkipMessage ) )
    {
        if( g_app->m_requestQuit ) break;
        //
        // Advance

        float timeSinceLastFrame = GetHighResTime() - lastFrameTime;
        float newPropagation = timeSinceLastFrame / m_speed;
        newPropagation = min( newPropagation, 1.0f );
        newPropagation = max( newPropagation, 0.0f );
        PropagateCells( lastPropagation, newPropagation );
        lastPropagation = newPropagation;

        if( newPropagation >= 1.0f )
        {
            CommitCells();
            lastFrameTime = GetHighResTime();
            lastPropagation = 0.0f;
            if( m_numFound == 0 && m_allDead < 0.0f)
            {
                m_allDead = GetHighResTime();
            }
            if( m_numFound > 0 ) ++m_totalAge;
            m_numFound = 0;
        }

        if( g_inputManager->controlEvent( ControlGOLLoaderSpeedup ) ) m_speed *= 1.1f;
        if( g_inputManager->controlEvent( ControlGOLLoaderSlowdown ) ) m_speed *= 0.9f;
        if( g_inputManager->controlEvent( ControlGOLLoaderReset ) )
        {
            CommitCells();
            ClearCells();
            RandomiseCells();
            lastPropagation = 0;
            m_totalAge = 0;
        }

        //
        // Render

        m_zoom = 0.08f + powf( GetHighResTime() - startTime, 2.0f ) / 500;
        m_speed = 0.1f / (m_zoom * 5.0f);
        m_speed = max( m_speed, 0.1f );

        float screenW = g_app->m_renderer->ScreenW() * m_zoom;
        float screenH = g_app->m_renderer->ScreenH() * m_zoom;
        if( screenW > CELLSIZE * m_numCellsX )
        {
            screenW = CELLSIZE * m_numCellsX;
            screenH = screenW * (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW();
        }
        SetupScreen( screenW, screenH );

        for( int x = 0; x < m_numCellsX; ++x )
        {
            for( int y = 0; y < m_numCellsY; ++y )
            {
                int thisCell = m_cells[y * m_numCellsX + x];
                int age = m_age[y * m_numCellsX + x];
                if( thisCell == CellStateAlive )
                {
                    RenderDarwinian( x, y, age );
                }
                else if( thisCell == CellStateDead )
                {
                    RenderSpirit( x, y, age );
                }
            }
        }

        //
        // Are we all dead?

        if( m_allDead > 0.0f )
        {
            float alpha = (GetHighResTime() - m_allDead) * 0.2f;
            alpha = min( alpha, 1.0f );
            glColor4f( 1.0f, 1.0f, 1.0f, alpha );

            g_gameFont.DrawText2DCentre( 0.0f, screenH/3+screenW/100.0f, screenW/100.0f, "%s %d", LANGUAGEPHRASE("bootloader_life_17"), m_totalAge );
            g_gameFont.DrawText2DCentre( 0.0f, screenH/3+2*screenW/100.0f,screenW/100.0f, LANGUAGEPHRASE("bootloader_life_18") );

            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
        }


        if( g_inputManager->controlEvent( ControlLoaderHelp ) ) RenderHelp();

        FlipBuffers();
        AdvanceSound();
    }


    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music LoaderGameOfLife" );
#ifdef USE_DIRECT3D
	glTraceEnable(false);
#endif // USE_DIRECT3D
}
