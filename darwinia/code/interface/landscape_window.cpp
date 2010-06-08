#include "lib/universal_include.h"

#include <string.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/language_table.h"

#include "interface/input_field.h"
#include "interface/landscape_window.h"

#include "app.h"
#include "camera.h"
#include "location_editor.h"
#include "level_file.h"
#include "renderer.h"
#include "location.h"


#ifdef LOCATION_EDITOR


// ****************************************************************************
// Class LandscapeTileButton
// ****************************************************************************

class LandscapeTileButton: public DarwiniaButton
{
public:
	LandscapeTile *m_def;

	LandscapeTileButton(LandscapeTile *_def): m_def(_def) {}
	
	void MouseUp()
	{
		if (stricmp(m_name, LANGUAGEPHRASE("editor_generate")) == 0)
		{
			LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
			g_app->m_location->m_landscape.Init(def);
		}
		else if (stricmp(m_name, LANGUAGEPHRASE("editor_randomise")) == 0)
		{
			m_def->m_randomSeed = (int)(GetHighResTime() * 1000.0f);
			InputField *randomSeed = (InputField *)m_parent->GetButton(LANGUAGEPHRASE("editor_seed"));
			if (randomSeed)
			{
				randomSeed->Refresh();
			}
			LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
			g_app->m_location->m_landscape.Init(def);
		}
        else if( stricmp(m_name, LANGUAGEPHRASE("editor_delete")) == 0 )
        {
            int tileId = ((LandscapeTileEditWindow *) m_parent)->m_tileId;
            g_app->m_location->m_landscape.DeleteTile( tileId );
            EclRemoveWindow( m_parent->m_name );
        }
        else if( stricmp(m_name, LANGUAGEPHRASE("editor_clone")) == 0 )
        {
            Vector3 rayStart;
	        Vector3 rayDir;
	        g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2, g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
            Vector3 _pos;
            g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

            LandscapeDef *landscapeDef = &(g_app->m_location->m_levelFile->m_landscape);
    	    LandscapeTile *tile = new LandscapeTile();
    	    g_app->m_location->m_levelFile->m_landscape.m_tiles.PutDataAtEnd(tile);
            tile->m_size = m_def->m_size;
            tile->m_posX = _pos.x - tile->m_size/2;
            tile->m_posY = m_def->m_posY;
            tile->m_posZ = _pos.z - tile->m_size/2;
            tile->m_fractalDimension = m_def->m_fractalDimension;
            tile->m_heightScale = m_def->m_heightScale;
            tile->m_desiredHeight = m_def->m_desiredHeight;
            tile->m_randomSeed = m_def->m_randomSeed;
            tile->m_lowlandSmoothingFactor = m_def->m_lowlandSmoothingFactor;
            tile->m_generationMethod = m_def->m_generationMethod;
            
		    LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
            g_app->m_location->m_landscape.Init(def);
            
        }
        else if( stricmp(m_name, LANGUAGEPHRASE("editor_guidegrid")) == 0 )
        {
            int tileId = ((LandscapeTileEditWindow *) m_parent)->m_tileId;
            LandscapeGuideGridWindow *guide = new LandscapeGuideGridWindow(LANGUAGEPHRASE("editor_guidegrid"), tileId );
            guide->SetSize( 300, 300 );
            guide->SetPosition( m_parent->m_x + m_parent->m_w + 10, m_parent->m_y );
            EclRegisterWindow( guide, m_parent );
        }
	}
};


// ****************************************************************************
// Class LandscapeTileEditWindow
// ****************************************************************************

LandscapeTileEditWindow::LandscapeTileEditWindow( char *name, int tileId )
:   DarwiniaWindow(name),
    m_tileId(tileId) 
{
}


LandscapeTileEditWindow::~LandscapeTileEditWindow()
{
    g_app->m_locationEditor->m_selectionId = -1;
}


void LandscapeTileEditWindow::Create()
{
	DarwiniaWindow::Create();

	m_tileDef = g_app->m_location->m_levelFile->m_landscape.m_tiles.GetData(m_tileId);
	Landscape *land = &g_app->m_location->m_landscape;

	int height = 5;
	int pitch = 17;
	int buttonWidth = m_w - 50;

	LandscapeTileButton *gen = new LandscapeTileButton(m_tileDef);
	gen->SetShortProperties(LANGUAGEPHRASE("editor_generate"), 10, height += pitch, m_w - 20 );
	RegisterButton(gen);

	LandscapeTileButton *randomise = new LandscapeTileButton(m_tileDef);
	randomise->SetShortProperties(LANGUAGEPHRASE("editor_randomise"), 10, height += pitch, m_w - 20 );
	RegisterButton(randomise);

    LandscapeTileButton *deleteTile = new LandscapeTileButton(m_tileDef);
    deleteTile->SetShortProperties(LANGUAGEPHRASE("editor_delete"), 10, height += pitch, m_w - 20 );
    RegisterButton(deleteTile);

    LandscapeTileButton *clone = new LandscapeTileButton(m_tileDef);
    clone->SetShortProperties(LANGUAGEPHRASE("editor_clone"), 10, height += pitch, m_w - 20 );
    RegisterButton(clone);

    LandscapeTileButton *guideGrid = new LandscapeTileButton(m_tileDef);
    guideGrid->SetShortProperties(LANGUAGEPHRASE("editor_guidegrid"), 10, height += pitch, m_w - 20 );
    RegisterButton(guideGrid);
    
    height += 6;

#define FLOAT InputField::TypeFloat
#define INTGR InputField::TypeInt
#define Y height += pitch
    int celSz = floorf(land->m_heightMap->m_cellSizeX);
    CreateValueControl( LANGUAGEPHRASE("editor_noiseperiod"),       FLOAT, &m_tileDef->m_fractalDimension,		Y, 0.01f,	0.0f,	10.0f,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_noisescale"),		FLOAT, &m_tileDef->m_heightScale,			Y, 0.02f,	0.0f,	10.0f,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_height"),            FLOAT, &m_tileDef->m_desiredHeight,			Y, 5.0f,	0.0f,	1000.0f,gen );
    CreateValueControl( LANGUAGEPHRASE("editor_method"),            INTGR, &m_tileDef->m_generationMethod,		Y, 1,		0,		2,		gen );
    CreateValueControl( LANGUAGEPHRASE("editor_lowlandsmooth"),     FLOAT, &m_tileDef->m_lowlandSmoothingFactor,Y, 0.02f,	0.0f,	3.0f,	gen );
	CreateValueControl( LANGUAGEPHRASE("editor_posX"),              INTGR, &m_tileDef->m_posX,					Y, celSz,	-10000,	10000,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_posY"),              FLOAT, &m_tileDef->m_posY,					Y, 0.5f,	-1000,	1000,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_posZ"),              INTGR, &m_tileDef->m_posZ,					Y, celSz,	-10000,	10000,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_size"),	            INTGR, &m_tileDef->m_size,					Y, 10,		0,		10000,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_seed"),              INTGR, &m_tileDef->m_randomSeed,			Y, 1,		0,		1e10,	gen );
#undef FLOAT
#undef INTGR
#undef Y
}


// ****************************************************************************
// Class LandscapeFlatAreaDeleteButton
// ****************************************************************************

class LandscapeFlattenAreaDeleteButton: public DarwiniaButton
{
public:
	int m_areaId;

	LandscapeFlattenAreaDeleteButton(int _areaId): m_areaId(_areaId) {}
	
	void MouseUp()
	{
		g_app->m_location->m_levelFile->m_landscape.m_flattenAreas.RemoveData(m_areaId);
		g_app->m_locationEditor->m_selectionId = -1;
        EclRemoveWindow( m_parent->m_name );
	}
};


// ****************************************************************************
// Class LandscapeFlatAreaEditWindow
// ****************************************************************************

LandscapeFlattenAreaEditWindow::LandscapeFlattenAreaEditWindow(char *_name, int areaId)
:   DarwiniaWindow(_name),
    m_areaId(areaId) 
{
}


LandscapeFlattenAreaEditWindow::~LandscapeFlattenAreaEditWindow()
{
    g_app->m_locationEditor->m_selectionId = -1;
}


void LandscapeFlattenAreaEditWindow::Create()
{
	DarwiniaWindow::Create();

	m_areaDef = g_app->m_location->m_levelFile->m_landscape.m_flattenAreas.GetData(m_areaId);

	int height = 5;
	int pitch = 17;
	int buttonWidth = m_w - 20;

	LandscapeTileButton *gen = new LandscapeTileButton(NULL);
	gen->SetShortProperties(LANGUAGEPHRASE("editor_generate"), 10, height += pitch, m_w - 20);
	RegisterButton(gen);

    CreateValueControl( LANGUAGEPHRASE("editor_size"), InputField::TypeFloat, &m_areaDef->m_size, height += pitch, 1, 0, 1000, gen );
    CreateValueControl( LANGUAGEPHRASE("editor_height"), InputField::TypeFloat, &m_areaDef->m_centre.y, height += pitch, 1, -1000, 1000, gen );

	LandscapeFlattenAreaDeleteButton *del = new LandscapeFlattenAreaDeleteButton(m_areaId);
	del->SetShortProperties(LANGUAGEPHRASE("editor_delete"), 10, height += pitch, buttonWidth);
	RegisterButton(del);

    
}


// ****************************************************************************
// Class NewTileButton
// ****************************************************************************

class NewTileButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
	    Vector3 rayStart;
	    Vector3 rayDir;
	    g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2, g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
        Vector3 _pos;
        g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

        LandscapeDef *landscapeDef = &(g_app->m_location->m_levelFile->m_landscape);
    	LandscapeTile *tile = new LandscapeTile();
    	g_app->m_location->m_levelFile->m_landscape.m_tiles.PutDataAtEnd(tile);
        tile->m_size = 384;
        tile->m_posX = _pos.x - tile->m_size/2;
        tile->m_posY = 0.0f;
        tile->m_posZ = _pos.z - tile->m_size/2;
        //tile->m_fractalDimension = 0.8f;
        //tile->m_heightScale = 0.4f;
        //tile->m_randomSeed = 1;
        //tile->m_lowlandSmoothingFactor = 1.0f;
        // Moved to LandscapeTile constructor

		LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
        g_app->m_location->m_landscape.Init(def);
    }
};


// ****************************************************************************
// Class NewFlattenAreaButton
// ****************************************************************************

class NewFlattenAreaButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
		int const screenH = g_app->m_renderer->ScreenH();
		int const screenW = g_app->m_renderer->ScreenW();
	    Vector3 rayStart;
	    Vector3 rayDir;
	    g_app->m_camera->GetClickRay(screenW/2, screenH/2, &rayStart, &rayDir);
        Vector3 _pos;
        g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

        LandscapeDef *landscapeDef = &(g_app->m_location->m_levelFile->m_landscape);
    	LandscapeFlattenArea *def = new LandscapeFlattenArea();
    	g_app->m_location->m_levelFile->m_landscape.m_flattenAreas.PutDataAtEnd(def);
        def->m_centre = _pos;
        def->m_size = 40.0f;

        g_app->m_location->m_landscape.Init(landscapeDef);
    }
};


// ****************************************************************************
// Class ScaleLandscapeButton
// ****************************************************************************

class ScaleLandscapeButton : public DarwiniaButton
{
public:
    float m_scaleFactor;
    void MouseUp()
    {
        //
        // Landscape and tiles

        LevelFile *levelFile = g_app->m_location->m_levelFile;

        for( int i = 0; i < levelFile->m_landscape.m_tiles.Size(); ++i )
        {
            LandscapeTile *tile = levelFile->m_landscape.m_tiles[i];
            tile->m_posX *= m_scaleFactor;
            tile->m_posZ *= m_scaleFactor;
            tile->m_size *= m_scaleFactor;
        }

        levelFile->m_landscape.m_worldSizeX *= m_scaleFactor;
        levelFile->m_landscape.m_worldSizeZ *= m_scaleFactor;
        
    	g_app->m_location->m_landscape.Init(&levelFile->m_landscape);

        //
        // Buildings

        for( int i = 0; i < levelFile->m_buildings.Size(); ++i )
        {
            Building *building = levelFile->m_buildings[i];
            building->m_pos.x *= m_scaleFactor;
            building->m_pos.z *= m_scaleFactor;
        }

        //
        // Instant units

        for( int i = 0; i < levelFile->m_instantUnits.Size(); ++i )
        {
            InstantUnit *unit = levelFile->m_instantUnits[i];
            unit->m_posX *= m_scaleFactor;
            unit->m_posZ *= m_scaleFactor;
            unit->m_spread *= m_scaleFactor;
        }

        //
        // Cam mounts

        for( int i = 0; i < levelFile->m_cameraMounts.Size(); ++i )
        {
            CameraMount *mount = levelFile->m_cameraMounts[i];
            mount->m_pos.x *= m_scaleFactor;
            mount->m_pos.z *= m_scaleFactor;
        }
    }
};


// ****************************************************************************
// Class LandscapeEditWindow
// ****************************************************************************

LandscapeEditWindow::LandscapeEditWindow( char *name )
:   DarwiniaWindow(name)
{
}


LandscapeEditWindow::~LandscapeEditWindow()
{
	g_app->m_locationEditor->RequestMode(LocationEditor::ModeNone);
	EclRemoveWindow(LANGUAGEPHRASE("editor_landscapetile"));
    EclRemoveWindow(LANGUAGEPHRASE("editor_guidegrid"));
}


void LandscapeEditWindow::Create()
{
	DarwiniaWindow::Create();

	int height = 5;
	int pitch = 17;
	int buttonWidth = m_w - 20;

	LandscapeTileButton *gen = new LandscapeTileButton(NULL);
	gen->SetShortProperties(LANGUAGEPHRASE("editor_generate"), 10, height += pitch, m_w - 20);
	RegisterButton(gen);

    NewTileButton *newTile = new NewTileButton();
    newTile->SetShortProperties(LANGUAGEPHRASE("editor_newtile"), 10, height += pitch, m_w - 20);
    RegisterButton(newTile);

    NewFlattenAreaButton *newFlat = new NewFlattenAreaButton();
    newFlat->SetShortProperties(LANGUAGEPHRASE("editor_newflattenarea"), 10, height += pitch, m_w - 20);
    RegisterButton(newFlat);

    height += 8;
    
    ScaleLandscapeButton *scaleDown = new ScaleLandscapeButton();
    scaleDown->SetShortProperties( LANGUAGEPHRASE("editor_scaledown"), 10, height += pitch, m_w - 20 );
    scaleDown->m_scaleFactor = 0.95f;
    RegisterButton( scaleDown );

    ScaleLandscapeButton *scaleUp = new ScaleLandscapeButton();
    scaleUp->SetShortProperties( LANGUAGEPHRASE("editor_scaleup"), 10, height += pitch, m_w - 20 );
    scaleUp->m_scaleFactor = 1.05f;
    RegisterButton( scaleUp );

    height += 8;

#define FLOAT InputField::TypeFloat
#define INTGR InputField::TypeInt
#define Y height += pitch
	
	LandscapeDef *landDef = &g_app->m_location->m_levelFile->m_landscape;
    CreateValueControl( LANGUAGEPHRASE("editor_outsideheight"), FLOAT, &landDef->m_outsideHeight,	Y, 1.0f,	-100,	100,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_cellsize"),		FLOAT, &landDef->m_cellSize,		Y, 1,		1.0f,	100.0f,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_worldsizex"),	INTGR, &landDef->m_worldSizeX,		Y, 10,		1,		1e6,	gen );
    CreateValueControl( LANGUAGEPHRASE("editor_worldsizez"),    INTGR, &landDef->m_worldSizeZ,		Y, 10,		1,		1e6,	gen );

#undef FLOAT
#undef INTGR
#undef Y

    CreateValueControl( LANGUAGEPHRASE("editor_movebuildings"), InputField::TypeInt, &g_app->m_locationEditor->m_moveBuildingsWithLandscape, 
                        height+=pitch, 1, 0, 1 );
}


// ****************************************************************************
// Class LandscapeGuideGridWindow
// ****************************************************************************

class GuideGridButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        if( stricmp( m_name, LANGUAGEPHRASE("editor_generate") ) == 0 )
        {
            LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
	        parent->m_tileDef->GuideGridSetPower(parent->m_guideGridPower);
			LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
            g_app->m_location->m_landscape.Init(def);   
            
            parent->Remove();
            parent->Create();
        }
    }
};

class GuideGrid : public EclButton
{
public:
    void RenderBorder( int realX, int realY, bool highlighted, bool clicked )
    {
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        LandscapeTile *def = parent->m_tileDef;

        if( def->m_guideGrid )
        {
            int w = def->m_guideGrid->GetNumColumns() * parent->m_pixelSizePerSample;
            int h = def->m_guideGrid->GetNumColumns() * parent->m_pixelSizePerSample;

            glColor3f( 1.0f, 1.0f, 1.0f );
            glBegin( GL_LINE_LOOP );
                glVertex2i( realX, realY );
                glVertex2i( realX + w, realY );
                glVertex2i( realX + w, realY + h );
                glVertex2i( realX, realY + h );
            glEnd();
        }
    }

    void RenderGrid( int realX, int realY, bool highlighted, bool clicked )
    {
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        LandscapeTile *def = parent->m_tileDef;

        if( def->m_guideGrid )
        {
            int w = def->m_guideGrid->GetNumColumns() * parent->m_pixelSizePerSample;
            int h = def->m_guideGrid->GetNumColumns() * parent->m_pixelSizePerSample;

            glBegin( GL_LINES );
            glColor3f( 0.0f, 0.0f, 0.8f );
            for( int x = 0; x < def->m_guideGrid->GetNumColumns(); x++ )
            {
                glVertex2i( realX + (x+0.5f) * parent->m_pixelSizePerSample, realY );
                glVertex2i( realX + (x+0.5f) * parent->m_pixelSizePerSample, realY + h );
            }
            for( int z = 0; z < def->m_guideGrid->GetNumColumns(); z++ )
            {            
                glVertex2i( realX, realY + (z+0.5f) * parent->m_pixelSizePerSample );
                glVertex2i( realX + w, realY + (z+0.5f) * parent->m_pixelSizePerSample );
            }
            glEnd();
        }
    }

    void RenderColours( int realX, int realY, bool highlighted, bool clicked )
    {
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        LandscapeTile *def = parent->m_tileDef;

        if( def->m_guideGrid )
        {
            glBegin( GL_QUADS );
            for( int x = 0; x < def->m_guideGrid->GetNumColumns(); ++x )
            {
                for( int z = 0; z < def->m_guideGrid->GetNumColumns(); ++z )
                {
                    unsigned char val = def->m_guideGrid->GetData( x, z );
                    glColor4ub( val, val, val, 255 );
                    glVertex2i( realX + x * parent->m_pixelSizePerSample, 
                                realY + z * parent->m_pixelSizePerSample );
                    glVertex2i( realX + (x+1) * parent->m_pixelSizePerSample, 
                                realY + z * parent->m_pixelSizePerSample );
                    glVertex2i( realX + (x+1) * parent->m_pixelSizePerSample, 
                                realY + (z+1) * parent->m_pixelSizePerSample );
                    glVertex2i( realX + x * parent->m_pixelSizePerSample, 
                                realY + (z+1) * parent->m_pixelSizePerSample );
                }
            }
            glEnd();
        }
    }

    void RenderMouse( int realX, int realY, bool highlighted, bool clicked )
    {
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        LandscapeTile *def = parent->m_tileDef;

        if( def->m_guideGrid )
        {
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );

            for( int x = 0; x < def->m_guideGrid->GetNumColumns(); ++x )
            {
                for( int z = 0; z < def->m_guideGrid->GetNumColumns(); ++z )
                {
                    float effect = GetToolEffect( x, z );
                    if( effect > 0.0f )
                    {
                        glColor3f( effect, effect, 0.0f );
                        glBegin( GL_LINE_LOOP );
                            glVertex2i( realX + x * parent->m_pixelSizePerSample, 
                                        realY + z * parent->m_pixelSizePerSample );
                            glVertex2i( realX + (x+1) * parent->m_pixelSizePerSample, 
                                        realY + z * parent->m_pixelSizePerSample );
                            glVertex2i( realX + (x+1) * parent->m_pixelSizePerSample, 
                                        realY + (z+1) * parent->m_pixelSizePerSample );
                            glVertex2i( realX + x * parent->m_pixelSizePerSample, 
                                        realY + (z+1) * parent->m_pixelSizePerSample );        
                        glEnd();
                    }
                }
            }

            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }        
    }

    void HandleMouseInput()
    {
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        LandscapeTile *def = parent->m_tileDef;

		int mouseX = g_target->X();
		int mouseY = g_target->Y();
		// TODO: What should this really be?
		bool mousePressed = g_inputManager->controlEvent( ControlEclipseLMousePressed ) ||
		                    g_inputManager->controlEvent( ControlEclipseRMousePressed );
        if( mousePressed &&
            mouseX >= m_x + m_parent->m_x &&
            mouseY >= m_y + m_parent->m_y &&
            mouseX < m_x + m_w + m_parent->m_x &&
            mouseY < m_y + m_h + m_parent->m_y )
        {            
            for( int x = 0; x < def->m_guideGrid->GetNumColumns(); ++x )
            {
                for( int z = 0; z < def->m_guideGrid->GetNumColumns(); ++z )
                {
                    float effect = GetToolEffect( x, z );
                    if( effect > 0.0f )
                    {
                        unsigned char currentVal = def->m_guideGrid->GetData( x, z );
                        int newVal = 0;

                        switch( parent->m_tool )
                        {
                            case LandscapeGuideGridWindow::GuideGridToolFreehand:
								// TODO: What should this really be?
								if ( g_inputManager->controlEvent( ControlEclipseLMousePressed ) )
								{
									newVal = currentVal + parent->m_toolSize * effect;
								}
								else
								{
									newVal = currentVal - parent->m_toolSize * effect;
								}
                                break;

                            case LandscapeGuideGridWindow::GuideGridToolFlatten:
                                if( currentVal > 20 )
                                {
                                    newVal = currentVal - parent->m_toolSize * effect;
                                }
                                else
                                {
                                    newVal = currentVal + parent->m_toolSize * effect;
                                }
                                break;

                            case LandscapeGuideGridWindow::GuideGridToolBinary:
								// TODO: What should this really be?
                                if ( g_inputManager->controlEvent( ControlEclipseLMousePressed ) )
                                {
                                    newVal = 255;
                                }
                                else
                                {
                                    newVal = 0;
                                }
                                break;
                        }

                        if( newVal < 0 ) newVal = 0;
                        if( newVal > 255 ) newVal = 255;
                        def->m_guideGrid->PutData( x, z, newVal );
                    }
                }
            }
        }
    }

    float GetToolEffect( int x, int y )
    {
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        LandscapeTile *def = parent->m_tileDef;
        int toolSize = parent->m_toolSize;

        if( def->m_guideGrid->GetNumColumns() > 0 )
        {
            float mouseX = g_target->X();
            float mouseY = g_target->Y();

            mouseX = ( mouseX - m_x - parent->m_x ) / (float) parent->m_pixelSizePerSample;
            mouseY = ( mouseY - m_y - parent->m_y ) / (float) parent->m_pixelSizePerSample;

            float providedX = ( x + 0.5f );
            float providedY = ( y + 0.5f );

            if( toolSize == 1 && 
                int(mouseX) == int(providedX) &&
                int(mouseY) == int(providedY) )
            {
                return 1.0f;
            }
            
            float distance = sqrtf( (providedX - mouseX) * (providedX - mouseX) +
                                    (providedY - mouseY) * (providedY - mouseY) );

            if( distance > toolSize/2.0f )
            {
                return 0.0f;
            }
            else
            {
                return 1.0f - distance / (toolSize/2.0f);
            }
        }

        return 0.0f;
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {        
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        LandscapeTile *def = parent->m_tileDef;

        if( def->m_guideGrid )
        {
            RenderColours   ( realX, realY, highlighted, clicked );
            RenderGrid      ( realX, realY, highlighted, clicked );
            RenderBorder    ( realX, realY, highlighted, clicked );
            RenderMouse     ( realX, realY, highlighted, clicked );

            HandleMouseInput();
        }
    }
};

class GuideGridTool : public DarwiniaButton
{
public:
    int     m_toolType;

    GuideGridTool( int _toolType )
        :   DarwiniaButton(),
            m_toolType(_toolType) {}

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        if( parent->m_tool == m_toolType )
        {
            DarwiniaButton::Render( realX, realY, highlighted, true );
        }
        else
        {
            DarwiniaButton::Render( realX, realY, highlighted, clicked );
        }
    }    

    void MouseUp()
    {
        LandscapeGuideGridWindow *parent = (LandscapeGuideGridWindow *) m_parent;        
        parent->m_tool = m_toolType;
    }
};

LandscapeGuideGridWindow::LandscapeGuideGridWindow( char *name, int tileId )
:   DarwiniaWindow(name),
    m_tileId(tileId),
    m_tool(GuideGridToolFreehand),
    m_toolSize(1)
{
}

LandscapeGuideGridWindow::~LandscapeGuideGridWindow()
{
}

void LandscapeGuideGridWindow::Create()
{
	m_tileDef = g_app->m_location->m_levelFile->m_landscape.m_tiles.GetData(m_tileId);
    m_guideGridPower = m_tileDef->GuideGridGetPower();

    int gridW = m_w - 80;
    int gridH = m_h - 80;

    int biggerSize = min( gridW, gridH );
    if( m_guideGridPower > 0 )
    {   
        m_pixelSizePerSample = biggerSize / m_tileDef->m_guideGrid->GetNumColumns();
    }
    else
    {
        m_pixelSizePerSample = 0;
    }

    GuideGridButton *generate = new GuideGridButton();
    generate->SetShortProperties( LANGUAGEPHRASE("editor_generate"), 10, 25, 75 );
    RegisterButton( generate );

    CreateValueControl( LANGUAGEPHRASE("editor_resolution"), InputField::TypeInt, &m_guideGridPower,	25, 1, 0, 5, generate, 100, 150  );
    CreateValueControl( LANGUAGEPHRASE("editor_toolsize"), InputField::TypeFloat, &m_toolSize,			45, 1, 1.0f, 40.0f, NULL, 100, 150 );


    //
    // Create guide grid and paint controls

    GuideGrid *guideGrid = new GuideGrid();
    guideGrid->SetProperties( LANGUAGEPHRASE("editor_guidegrid"), 10, 70, gridW, gridH );
    RegisterButton( guideGrid );

    int controlsX = gridW + 15;
    int controlsY = 70;
    int controlsW = 60;
    int controlsH = 17;

    GuideGridTool *freehand = new GuideGridTool( GuideGridToolFreehand );
    freehand->SetShortProperties( LANGUAGEPHRASE("editor_freehand"), controlsX, controlsY, controlsW );
    RegisterButton( freehand );

    GuideGridTool *flatten = new GuideGridTool( GuideGridToolFlatten );
    flatten->SetShortProperties( LANGUAGEPHRASE("editor_flatten"), controlsX, controlsY += controlsH, controlsW );
    RegisterButton( flatten );

    GuideGridTool *binary = new GuideGridTool( GuideGridToolBinary );
    binary->SetShortProperties( LANGUAGEPHRASE("editor_binary"), controlsX, controlsY += controlsH, controlsW );
    RegisterButton( binary );
    
    DarwiniaWindow::Create();
}

#endif // LOCATION_EDITOR
