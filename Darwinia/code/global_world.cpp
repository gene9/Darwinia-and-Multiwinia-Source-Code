#include "lib/universal_include.h"

#include <float.h>

#include "lib/debug_render.h"
#include "lib/debug_utils.h"
#include "lib/language_table.h"
#include "lib/filesys_utils.h"
#include "lib/file_writer.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/sphere_renderer.h"
#include "lib/string_utils.h"
#include "lib/text_renderer.h"
#include "lib/text_stream_readers.h"
#include "lib/vector3.h"

#include "eclipse.h"

#include "app.h"
#include "camera.h"
#include "global_internet.h"
#include "global_world.h"
#include "landscape.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "script.h"
#include "sepulveda.h"
#include "testharness.h"
#include "user_input.h"
#include "helpsystem.h"
#include "startsequence.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "tutorial.h"

#include "worldobject/building.h"
#include "worldobject/trunkport.h"

#include "lib/window_manager.h"
#include "lib/preferences.h"
#include "lib/preference_names.h"

#include "interface/buynow_window.h"


// ****************************************************************************
// Class GlobalLocation
// ****************************************************************************

GlobalLocation::GlobalLocation()
:   m_id(-1),
    m_available(false),
    m_numSpirits(0),
    m_missionCompleted(false)
{
    strcpy( m_name, "none" );
    strcpy( m_mapFilename, "none" );
    strcpy( m_missionFilename, "none" );
}


void GlobalLocation::AddSpirits( int _count )
{
    m_numSpirits += _count;
}



// ****************************************************************************
// Class GlobalBuilding
// ****************************************************************************

GlobalBuilding::GlobalBuilding()
:   m_id(-1),
    m_teamId(-1),
    m_locationId(-1),
    m_online(false),
    m_type(Building::TypeTrunkPort),
    m_shape(NULL),
    m_link(-1)
{
    m_shape = g_app->m_resource->GetShape( "trunkport.shp" );
}


// ****************************************************************************
// Class GlobalEventCondition
// ****************************************************************************

GlobalEventCondition::GlobalEventCondition()
:   m_type(-1),
    m_id(-1),
    m_locationId(-1),
    m_stringId(NULL),
    m_cutScene(NULL)
{
}

GlobalEventCondition::GlobalEventCondition(const GlobalEventCondition &_other)
:   m_type(_other.m_type),
    m_id(_other.m_id),
    m_locationId(_other.m_locationId),
    m_stringId(NewStr(_other.m_stringId)),
    m_cutScene(NewStr(_other.m_cutScene))
{
}

GlobalEventCondition::~GlobalEventCondition()
{
	delete [] m_stringId;
	delete [] m_cutScene;
}

void GlobalEventCondition::SetStringId ( char *_stringId )
{
	delete [] m_stringId;
    m_stringId = NewStr( _stringId );
}


void GlobalEventCondition::SetCutScene ( char *_cutScene )
{
    delete [] m_cutScene;
    m_cutScene = NewStr( _cutScene );
}


char *GlobalEventCondition::GetTypeName( int _type )
{
    static char *names[] = {
                                "AlwaysTrue",
                                "BuildingOnline",
                                "BuildingOffline",
                                "ResearchOwned",
                                "NotInLocation",
								"DebugKey",
								"NeverTrue"
                           };

    DarwiniaDebugAssert( _type >= 0 && _type < NumConditions );

    return names[_type];
}


int GlobalEventCondition::GetType(char const *_typeName)
{
	for (int i = 0; i < NumConditions; ++i)
	{
		if (stricmp(_typeName, GetTypeName(i)) == 0)
		{
			return i;
		}
	}

	return -1;
}


bool GlobalEventCondition::Evaluate()
{
    switch( m_type )
    {
        case AlwaysTrue:
            return true;

        case BuildingOnline:
        {
            GlobalBuilding *building = g_app->m_globalWorld->GetBuilding( m_id, m_locationId );
            if( building ) return building->m_online;
            break;
        }

        case BuildingOffline:
        {
            GlobalBuilding *building = g_app->m_globalWorld->GetBuilding( m_id, m_locationId );
            if( building ) return !building->m_online;
            break;
        }

        case ResearchOwned:
            return( g_app->m_globalWorld->m_research->HasResearch( m_id ) );

        case NotInLocation:
            return( g_app->m_location == NULL );

#ifdef JAMES_FIX
		case DebugKey:
			return g_keys[KEY_0 + m_id];
#endif // JAMES_FIX

		case NeverTrue:
			return false;

        default:
            DarwiniaDebugAssert(false);
    }

    return false;
}


void GlobalEventCondition::Save( FileWriter *_out )
{
    _out->printf( "%s ", GetTypeName(m_type) );

    switch( m_type )
    {
        case AlwaysTrue:
            break;

        case BuildingOnline:
        case BuildingOffline:
            _out->printf( ":%s,%d ", g_app->m_globalWorld->GetLocationName(m_locationId), m_id);
            break;

        case ResearchOwned:
            _out->printf( ":%s ", GlobalResearch::GetTypeName(m_id) );
            break;

        case DebugKey:
            _out->printf( ":%d ", m_id);
            break;
    }
}



// ****************************************************************************
// Class GlobalEventAction
// ****************************************************************************

char *GlobalEventAction::GetTypeName(int _type)
{
    static char *names[] = {
                                "SetMission",
                                "RunScript",
                                "MakeAvailable"
                            };

    DarwiniaDebugAssert( _type >= 0 && _type < NumActionTypes );

    return names[_type];
}


GlobalEventAction::GlobalEventAction()
{
    strcpy( m_filename, "null" );
}


void GlobalEventAction::Read( TextReader *_in )
{
    char *action = _in->GetNextToken();

    if( stricmp( action, "SetMission" ) == 0 )
    {
        m_type = SetMission;
        m_locationId = g_app->m_globalWorld->GetLocationId( _in->GetNextToken() );
        DarwiniaDebugAssert( m_locationId != -1 );
        strcpy( m_filename, _in->GetNextToken() );
    }
    else if( stricmp( action, "RunScript" ) == 0 )
    {
        m_type = RunScript;
        strcpy( m_filename, _in->GetNextToken() );
    }
    else if( stricmp( action, "MakeAvailable" ) == 0 )
    {
        m_type = MakeAvailable;
        m_locationId = g_app->m_globalWorld->GetLocationId( _in->GetNextToken() );
        DarwiniaDebugAssert( m_locationId != -1 );
    }
    else
    {
        DarwiniaDebugAssert( false );
    }
}


void GlobalEventAction::Write(FileWriter *_out)
{
    _out->printf( "\t\tAction %-10s ", GetTypeName(m_type));

    char *locationName = g_app->m_globalWorld->GetLocationName( m_locationId );

    switch (m_type)
    {
        case SetMission:        _out->printf( "%s %s", locationName, m_filename);       break;
        case RunScript:         _out->printf( "%s", m_filename );                       break;
        case MakeAvailable:     _out->printf( "%s", locationName);                      break;

        default:
            DarwiniaDebugAssert(false);
    }

    _out->printf( "\n");
}


void GlobalEventAction::Execute()
{
#ifdef TEST_HARNESS_ENABLED
	switch (m_type)
	{
        case SetMission:
        {
            if (g_app->m_testHarness)
			{
				fprintf(g_app->m_testHarness->m_out, "%sSetting Mission: %s in location %s\n",
						g_app->m_testHarness->m_indent,
						m_filename,
						g_app->m_globalWorld->GetLocationName(m_locationId));
			}
            break;
        }
        case RunScript:
			if (g_app->m_testHarness)
			{
				fprintf(g_app->m_testHarness->m_out, "%sRunning script: %s\n",
						g_app->m_testHarness->m_indent,
						m_filename);
			}
            break;
        case MakeAvailable:
			if (g_app->m_testHarness)
			{
				fprintf(g_app->m_testHarness->m_out, "%sMaking location available: %s\n",
						g_app->m_testHarness->m_indent,
						g_app->m_globalWorld->GetLocationName(m_locationId));
			}
            break;
		default:
			break;
	}
#endif // TEST_HARNESS_ENABLED

    switch (m_type)
    {
        case SetMission:
        {
            GlobalLocation *loc = g_app->m_globalWorld->GetLocation(m_locationId);
            DarwiniaDebugAssert(loc);
            strcpy(loc->m_missionFilename, m_filename);
            break;
        }
        case RunScript:
        {
            g_app->m_script->RunScript(m_filename);
            break;
        }
        case MakeAvailable:
        {
            GlobalLocation *loc = g_app->m_globalWorld->GetLocation(m_locationId);
            DarwiniaDebugAssert(loc);
            loc->m_available = true;
            break;
        }

        default:
            DarwiniaDebugAssert(false);
    }
}



// ****************************************************************************
// Class GlobalEvent
// ****************************************************************************

GlobalEvent::GlobalEvent()
{
}


GlobalEvent::GlobalEvent(GlobalEvent &_other)
{
	for (int i = 0; i < _other.m_conditions.Size(); ++i)
	{
		GlobalEventCondition *newCond = new GlobalEventCondition(*(_other.m_conditions.GetData(i)));
		m_conditions.PutDataAtEnd(newCond);
	}

	for (int i = 0; i < _other.m_actions.Size(); ++i)
	{
		GlobalEventAction *newAction = new GlobalEventAction(*(_other.m_actions.GetData(i)));
		m_actions.PutDataAtEnd(newAction);
	}
}


bool GlobalEvent::Evaluate()
{
    bool success = true;

    for( int i = 0; i < m_conditions.Size(); ++i )
    {
        GlobalEventCondition *condition = m_conditions[i];
        if( !condition->Evaluate() )
        {
            success = false;
            break;
        }
    }

    return success;
}


bool GlobalEvent::Execute()
{
    if( m_actions.Size() == 0 ) return true;

    GlobalEventAction *action = m_actions[0];
    m_actions.RemoveData(0);
    action->Execute();
    delete action;

    if( m_actions.Size() == 0 ) return true;

    return false;
}


void GlobalEvent::MakeAlwaysTrue()
{
    if( m_conditions.Size() == 1 &&
        m_conditions[0]->m_type == GlobalEventCondition::AlwaysTrue )
    {
        return;
    }

    m_conditions.EmptyAndDelete();

    GlobalEventCondition *cond = new GlobalEventCondition();
    cond->m_type = GlobalEventCondition::AlwaysTrue;
    m_conditions.PutData( cond );
}


void GlobalEvent::Read( TextReader *_in )
{
    //
    // Parse conditions line

    while( _in->TokenAvailable() )
    {
		char *conditionTypeName = _in->GetNextToken();

        GlobalEventCondition *condition = new GlobalEventCondition;
		condition->m_type = condition->GetType(conditionTypeName);
        DarwiniaDebugAssert( condition->m_type != -1 );

		switch (condition->m_type)
		{
            case GlobalEventCondition::AlwaysTrue:
            case GlobalEventCondition::NotInLocation:
                break;

			case GlobalEventCondition::BuildingOffline:
			case GlobalEventCondition::BuildingOnline:
				condition->m_locationId = g_app->m_globalWorld->GetLocationId( _in->GetNextToken() );
				condition->m_id = atoi( _in->GetNextToken() );
                DarwiniaDebugAssert( condition->m_locationId != -1 );
				break;

            case GlobalEventCondition::ResearchOwned:
                condition->m_id = GlobalResearch::GetType( _in->GetNextToken() );
                DarwiniaDebugAssert( condition->m_id != -1 );
                break;

            case GlobalEventCondition::DebugKey:
                condition->m_id = atoi( _in->GetNextToken() );
				break;
		}

        m_conditions.PutData( condition );
    }


	//
	// Parse actions

    while( _in->ReadLine() )
    {
        if( _in->TokenAvailable() )
        {
            char *word = _in->GetNextToken();
            if( stricmp( word, "end" ) == 0 ) break;
            DarwiniaDebugAssert( stricmp( word, "action" ) == 0 );

            GlobalEventAction *action = new GlobalEventAction;
            action->Read( _in );
            m_actions.PutData( action );
        }
    }
}


void GlobalEvent::Write(FileWriter *_out)
{
    _out->printf( "\tEvent ");

    for (int i = 0; i < m_conditions.Size(); ++i)
    {
        GlobalEventCondition *gec = m_conditions[i];
        gec->Save( _out );
    }

    _out->printf( "\n");

    for (int i = 0; i < m_actions.Size(); ++i)
    {
        GlobalEventAction *gea = m_actions[i];
        gea->Write(_out);
    }

    _out->printf( "\t\tEnd\n" );
}


// ****************************************************************************
// Class GlobalResearch
// ****************************************************************************

GlobalResearch::GlobalResearch()
:   m_researchPoints(0),
    m_researchTimer(0.0f)
{
    for(int i = 0; i < NumResearchItems; ++i )
    {
        m_researchLevel[i] = 0;
        m_researchProgress[i] = 0;
    }

    m_currentResearch = TypeSquad;
}


void GlobalResearch::AddResearch( int _type )
{
    DarwiniaDebugAssert( _type >= 0 && _type < NumResearchItems );

    if( m_researchLevel[_type] < 1 )
    {
        m_researchProgress[_type] = RequiredProgress(0);
        EvaluateLevel( _type );
    }
}


bool GlobalResearch::HasResearch( int _type )
{
    DarwiniaDebugAssert( _type >= 0 && _type < NumResearchItems );

    return( m_researchLevel[_type] > 0 );
}


int GlobalResearch::CurrentProgress ( int _type )
{
    DarwiniaDebugAssert( _type >= 0 && _type < NumResearchItems );

    return m_researchProgress[_type];
}


int GlobalResearch::RequiredProgress( int _level )
{
    DarwiniaDebugAssert( _level >= 0 && _level < 4 );

    static int s_requiredProgress[] = { 1, 50, 100, 200 };

    return s_requiredProgress[_level];
}


void GlobalResearch::EvaluateLevel( int _type )
{
    int currentLevel = m_researchLevel[_type];

    if( currentLevel < 4 )
    {
        int newLevel = 0;

        int requiredProgress = RequiredProgress(currentLevel);

        if( m_researchProgress[_type] >= requiredProgress )
        {
            // Level change has just occurred
            m_researchLevel[_type]++;
            m_researchProgress[_type] -= requiredProgress;

            char sepStringId[256];
            sprintf( sepStringId, "research_%s_v%d", GetTypeName(_type), m_researchLevel[_type] );
            strlwr( sepStringId );

            if( ISLANGUAGEPHRASE( sepStringId ) )
            {
                g_app->m_sepulveda->Say( sepStringId );
            }

            if( currentLevel > 0 )
            {
                // This action should only go off if a player UPGRADES an existing research item
                // Not if he finds a new one
                g_app->m_helpSystem->PlayerDoneAction( HelpSystem::ResearchUpgrade );
                g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageResearchUpgrade, _type, 4.0f );
            }
        }
    }
}


void GlobalResearch::SetCurrentResearch( int _type )
{
    int currentLevel = m_researchLevel[_type];

    if( currentLevel == 4 )
    {
        // Fully researched already
        g_app->m_sepulveda->Say( "research_nomorepossible" );
        return;
    }


    if( m_currentResearch != _type )
    {
        m_currentResearch = _type;

        char sepStringId[256];
        sprintf( sepStringId, "research_%s", GetTypeName(_type) );
        strlwr( sepStringId );

        if( ISLANGUAGEPHRASE( sepStringId ) )
        {
            g_app->m_sepulveda->Say( sepStringId );
        }
    }
}


void GlobalResearch::GiveResearchPoints( int _numPoints )
{
    m_researchPoints += _numPoints;
}


void GlobalResearch::AdvanceResearch()
{
    if( m_researchPoints > 0 && CurrentLevel(m_currentResearch) < 4 )
    {
        m_researchTimer -= g_advanceTime;
        if( m_researchTimer <= 0.0f )
        {
            m_researchTimer = GLOBALRESEARCH_TIMEPERPOINT;
            IncreaseProgress( 1 );
            --m_researchPoints;
        }
    }
}


void GlobalResearch::SetCurrentProgress( int _type, int _progress )
{
    DarwiniaDebugAssert( _type >= 0 && _type < NumResearchItems );

    m_researchProgress[_type] = _progress;
    EvaluateLevel( _type );
}


void GlobalResearch::IncreaseProgress  ( int _amount )
{
    m_researchProgress[m_currentResearch] += _amount;
    EvaluateLevel( m_currentResearch );
}


void GlobalResearch::DecreaseProgress( int _amount )
{
    m_researchProgress[m_currentResearch] -= _amount;
    EvaluateLevel( m_currentResearch );
}


int GlobalResearch::CurrentLevel( int _type )
{
    DarwiniaDebugAssert( _type >= 0 && _type < NumResearchItems );

    return m_researchLevel[_type];
}


void GlobalResearch::Write(FileWriter *_out)
{
    _out->printf( "Research_StartDefinition\n" );

    for( int i = 0; i < NumResearchItems; ++i )
    {
        _out->printf( "\tResearch %s %d %d\n", GetTypeName(i), CurrentProgress(i), CurrentLevel(i) );
    }

    _out->printf( "\tCurrentResearch %s\n", GetTypeName(m_currentResearch) );
    _out->printf( "\tCurrentPoints %d\n", m_researchPoints );
    _out->printf( "Research_EndDefinition\n\n" );
}


void GlobalResearch::Read( TextReader *_in )
{
    while(_in->ReadLine())
    {
        if(!_in->TokenAvailable()) continue;
        char *word = _in->GetNextToken();

        if( stricmp(word, "research_enddefinition") == 0)
        {
            return;
        }
        else if( stricmp(word, "Research" ) == 0 )
        {
            char *type = _in->GetNextToken();
            int progress = atoi( _in->GetNextToken() );
            int level = atoi( _in->GetNextToken() );

            int researchType = GetType( type );
            m_researchLevel[researchType] = level;
            SetCurrentProgress( researchType, progress );
        }
        else if( stricmp(word, "CurrentResearch" ) == 0 )
        {
            char *type = _in->GetNextToken();
            int researchType = GetType( type );
            m_currentResearch = researchType;
        }
        else if( stricmp(word, "CurrentPoints" ) == 0 )
        {
            int points = atoi( _in->GetNextToken() );
            m_researchPoints = points;
        }
        else
        {
            DarwiniaReleaseAssert( false, "Error loading GlobalResearch" );
        }
    }
}

char *GlobalResearch::GetTypeName( int _type )
{
    char *names[] = {
                        "Darwinian",
                        "Officer",
                        "Squad",
                        "Laser",
                        "Grenade",
                        "Rocket",
                        "Controller",
                        "AirStrike",
                        "Armour",
                        "TaskManager",
                        "Engineer"
                    };

    DarwiniaDebugAssert( _type >= 0 && _type < NumResearchItems );
    return names[_type];
}


char *GlobalResearch::GetTypeNameTranslated ( int _type )
{
    char *typeName = GetTypeName(_type);

    char stringId[256];
    sprintf( stringId, "researchname_%s", typeName );

    if( ISLANGUAGEPHRASE( stringId ) )
    {
        return LANGUAGEPHRASE( stringId );
    }
    else
    {
        return typeName;
    }
}


int GlobalResearch::GetType( char *_name )
{
    for( int i = 0; i < NumResearchItems; ++i )
    {
        if( stricmp( _name, GetTypeName(i) ) == 0 )
        {
            return i;
        }
    }

    return -1;
}


// ****************************************************************************
// Class GlobalWorld
// ****************************************************************************


void ColourShapeFragment(ShapeFragment *_frag, RGBAColour const &_colour)
{
    if (_frag->m_numColours == 0)
    {
        _frag->m_colours = new RGBAColour [1];
    }
    _frag->m_colours[0] = _colour;

    for (int i = 0; i < _frag->m_numVertices; ++i)
    {
        VertexPosCol *vert = &_frag->m_vertices[i];
        vert->m_colId = 0;
    }

    for (int i = 0; i < _frag->m_childFragments.Size(); ++i)
    {
        ColourShapeFragment(_frag->m_childFragments[i], _colour);
    }
}

SphereWorld::SphereWorld()
:   m_shapeOuter(NULL),
    m_shapeMiddle(NULL),
    m_shapeInner(NULL),
    m_numLocations(0),
    m_spirits(NULL)
{
//#ifndef DEMOBUILD
    m_shapeOuter  = g_app->m_resource->GetShape( "globalworld_outer.shp" );
    m_shapeMiddle = g_app->m_resource->GetShape( "globalworld_middle.shp" );
    m_shapeInner  = g_app->m_resource->GetShape( "globalworld_inner.shp" );
//#endif
}


void SphereWorld::AddLocation( int _locationId )
{
	// Initialise the sphere world to
	if (_locationId < m_numLocations)
		return;

	int oldNumLocations = m_numLocations;
	m_numLocations = _locationId + 1;
	LList<float> *newSpirits = new LList<float>[m_numLocations];

	// Initialise the spirits for the new worlds.
    for( int locationId = 0; locationId < m_numLocations; ++locationId )
    {
		if (locationId < oldNumLocations)
		{
			// Copy across the old spirits
			newSpirits[locationId] = m_spirits[locationId];
		}
		else {
			// Generate some new spirits
			GlobalLocation *loc = g_app->m_globalWorld->GetLocation( locationId );
			if( loc )
			{
				if( strcmp( loc->m_name, "receiver" ) == 0 )
				{
					for( int i = 0; i < 60; ++i )
					{
						float value = frand();
						newSpirits[locationId].PutData(value);
					}
				}
				else
				{
					for( int i = 0; i < 10; ++i )
					{
						float value = frand();
						newSpirits[locationId].PutData(value);
					}
				}
			}
		}
    }

	delete [] m_spirits;
	m_spirits = newSpirits;
}


void SphereWorld::Render()
{
	// For some reason this fixes a slight glitch in the first few frames of the
	// start sequence on my machine. God knows why, but it won't cause any harm.
	static int frameCount = 0;
	if (frameCount < 10)
	{
		frameCount++;
		return;
	}

    RenderWorldShape();
    RenderIslands();
    RenderTrunkLinks();

    if( !g_app->m_editing )
    {
        RenderSpirits();
        RenderHeaven();
    }

	glEnable(GL_CULL_FACE); // CRASH WORKAROUND - FIX AND DELETE ASAP
	CHECK_OPENGL_STATE();
}


void SphereWorld::RenderSpirits()
{
    START_PROFILE( g_app->m_profiler, "Spirits" );

    //
    // Advance all spirits

    for( int locationId = 0; locationId < m_numLocations; ++locationId )
    {
        GlobalLocation *location = g_app->m_globalWorld->GetLocation(locationId);
        if( location )
        {
            bool isReceiver = ( strcmp( location->m_name, "receiver" ) == 0 );

            if( isReceiver && frand(30) < 1.0f )
            {
                m_spirits[locationId].PutDataAtStart( 0.0f );
            }
            else if( !isReceiver && frand(300) < 1.0f )
            {
                m_spirits[locationId].PutDataAtStart( 1.0f );
            }

            if( location->m_numSpirits > 0 && frand(20) < 1.0f)
            {
                m_spirits[locationId].PutDataAtStart( 1.0f );
                location->m_numSpirits--;
            }

            for( int i = 0; i < m_spirits[locationId].Size(); ++i )
            {
                float *thisSpirit = m_spirits[locationId].GetPointer(i);

                if( isReceiver )        *thisSpirit += g_advanceTime * 0.02f;
                else                    *thisSpirit -= g_advanceTime * 0.02f;

                if( *thisSpirit >= 1.0f || *thisSpirit <= 0.0f )
                {
                    m_spirits[locationId].RemoveData(i);
                    --i;
                }
            }
        }
    }


    //
    // Render all spirits

    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );

	glEnable		( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );

    Vector3 camRight = g_app->m_camera->GetRight();
    Vector3 camUp = g_app->m_camera->GetUp();


    for( int locationId = 0; locationId < m_numLocations; ++locationId )
    {
        GlobalLocation *location = g_app->m_globalWorld->GetLocation(locationId);
        if( location )
        {
            for( int i = 0; i < m_spirits[locationId].Size(); ++i )
            {
                float *thisSpirit = m_spirits[locationId].GetPointer(i);

                Vector3 fromPos = g_app->m_globalWorld->GetLocationPosition(locationId);

                float alphaValue = *thisSpirit * 3.0f;
                if( alphaValue > 1.0f ) alphaValue = 1.0f;

                Vector3 position = fromPos * (*thisSpirit);
                float timeOffset = g_gameTime/2.0f;
                float posOffset = 1000;

                position.x += sinf(*thisSpirit * 14 + timeOffset) * posOffset;
                position.y += sinf(*thisSpirit * 15 + timeOffset) * posOffset;
                position.z += sinf(*thisSpirit * 16 + timeOffset) * posOffset;

                float scale = 0.4f;

                glColor4f( 0.6f, 0.2f, 0.1f, alphaValue);
                glBegin( GL_QUADS );
                    glTexCoord2f(0.5f, 0.5f);      glVertex3fv( (position + camUp * 300 * scale).GetData() );
                    glTexCoord2f(0.5f, 0.5f);      glVertex3fv( (position + camRight * 300 * scale).GetData() );
                    glTexCoord2f(0.5f, 0.5f);      glVertex3fv( (position - camUp * 300 * scale).GetData() );
                    glTexCoord2f(0.5f, 0.5f);      glVertex3fv( (position - camRight * 300 * scale).GetData() );
                glEnd();

                glColor4f( 0.6f, 0.2f, 0.1f, alphaValue);
                glBegin( GL_QUADS );
                    glTexCoord2f(0.5f, 0.5f);      glVertex3fv( (position + camUp * 100 * scale).GetData() );
                    glTexCoord2f(0.5f, 0.5f);      glVertex3fv( (position + camRight * 100 * scale).GetData() );
                    glTexCoord2f(0.5f, 0.5f);      glVertex3fv( (position - camUp * 100 * scale).GetData() );
                    glTexCoord2f(0.5f, 0.5f);      glVertex3fv( (position - camRight * 100 * scale).GetData() );
                glEnd();

                glColor4f( 0.6f, 0.2f, 0.1f, alphaValue/4.0f);
                glBegin( GL_QUADS );
                    glTexCoord2i(0,0);      glVertex3fv( (position + camUp * 6000 * scale).GetData() );
                    glTexCoord2i(1,0);      glVertex3fv( (position + camRight * 6000 * scale).GetData() );
                    glTexCoord2i(1,1);      glVertex3fv( (position - camUp * 6000 * scale).GetData() );
                    glTexCoord2i(0,1);      glVertex3fv( (position - camRight * 6000 * scale).GetData() );
                glEnd();
            }
        }
    }


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glDisable   ( GL_TEXTURE_2D );

    glDepthMask ( true );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable   ( GL_BLEND );
    glEnable    ( GL_CULL_FACE );


    END_PROFILE( g_app->m_profiler, "Spirits" );
}


void SphereWorld::RenderWorldShape()
{
    START_PROFILE(g_app->m_profiler, "Shape");

	g_app->m_globalWorld->SetupLights();

    glEnable        (GL_LIGHTING);
    glEnable        (GL_LIGHT0);

	float spec = 0.5f;
	float diffuse = 1.0f;
    float amb = 0.0f;
	GLfloat materialShininess[] = { 10.0f };
	GLfloat materialSpecular[] = { spec, spec, spec, 1.0f };
   	GLfloat materialDiffuse[] = { diffuse, diffuse, diffuse, 1.0f };
	GLfloat ambCol[] = { amb, amb, amb, 1.0f };

	glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
	glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambCol);

    glPushMatrix();
    glScalef( 120.0f, 120.0f, 120.0f );
    glEnable( GL_NORMALIZE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glDisable( GL_CULL_FACE );

    //
    // Render outer

    m_shapeOuter->Render(0.0f, g_identityMatrix34);
    m_shapeMiddle->Render(0.0f, g_identityMatrix34);
    m_shapeInner->Render(0.0f, g_identityMatrix34);


    glDisable       (GL_NORMALIZE);
    glPopMatrix     ();


    glDisable       (GL_COLOR_MATERIAL);
    glDisable       (GL_LIGHTING);
    glDisable       (GL_LIGHT0);
    glDisable       (GL_LIGHT1);

    END_PROFILE(g_app->m_profiler, "Shape");
}



void SphereWorld::RenderTrunkLinks()
{
    //if( g_app->m_editing ) return;

    Matrix34    rootMat(0);

    glEnable    ( GL_BLEND );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask ( false );

    glBegin     ( GL_QUADS );

    for( int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i )
    {
        GlobalBuilding *building = g_app->m_globalWorld->m_buildings[i];
        if( building->m_type == Building::TypeTrunkPort &&
            building->m_link != -1 )
        {
            GlobalLocation *fromLoc = g_app->m_globalWorld->GetLocation( building->m_locationId );
            GlobalLocation *toLoc = g_app->m_globalWorld->GetLocation( building->m_link );

            if( fromLoc && toLoc &&
                (fromLoc->m_available && toLoc->m_available) ||
                g_app->m_editing )
            {
                Vector3 fromPos = g_app->m_globalWorld->GetLocationPosition(building->m_locationId);
                Vector3 toPos = g_app->m_globalWorld->GetLocationPosition(building->m_link);

                if( building->m_online )    glColor4f( 0.4f, 0.3f, 1.0f, 1.0f );
                else                        glColor4f( 0.4f, 0.3f, 1.0f, 0.4f );

                //fromPos *= 120.0f;
                //toPos *= 120.0f;

                Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0f;
                Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
                Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();

                rightAngle *= 200.0f;

                glVertex3fv( (fromPos - rightAngle).GetData() );
                glVertex3fv( (fromPos + rightAngle).GetData() );
                glVertex3fv( (toPos + rightAngle).GetData() );
                glVertex3fv( (toPos - rightAngle).GetData() );
            }
        }
    }

    glEnd       ();
    glDepthMask ( true );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable   ( GL_BLEND );

}


void SphereWorld::RenderHeaven()
{
    START_PROFILE( g_app->m_profiler, "Heaven" );

    g_app->m_globalWorld->SetupLights();

    //
    // Render the central repository of spirits

    glPushMatrix    ();
    glScalef        ( 120.0f, 120.0f, 120.0f );

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();

    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );

    for( int i = 0; i < 50; ++i )
    {
        Vector3 pos( sinf(i/g_gameTime+i) * 20,
                     sinf(g_gameTime+i) * i,
                     cosf(i/g_gameTime+i) * 20 );

        float size = i;

        glColor4f( 0.6f, 0.2f, 0.1f, 0.9f);

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }


    glPopMatrix     ();

    //
    // Render god rays going down

/*
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/godray.bmp" ) );

	for (int i = 0; i < g_app->m_globalWorld->m_locations.Size(); ++i)
	{
		GlobalLocation *loc = g_app->m_globalWorld->m_locations.GetData(i);
        Vector3 islandPos = g_app->m_globalWorld->GetLocationPosition(loc->m_id );
        Vector3 centrePos = g_zeroVector;

        for( int j = 0; j < 6; ++j )
        {
            Vector3 godRayPos = islandPos;

            godRayPos.x += sinf( g_gameTime + i + j/2 ) * 1000;
            godRayPos.y += sinf( g_gameTime + i + j/2 ) * 1000;
            godRayPos.z += cosf( g_gameTime + i + j/2 ) * 1000;

            Vector3 camToCentre = g_app->m_camera->GetPos() - centrePos;
            Vector3 lineToCentre = camToCentre ^ ( centrePos - godRayPos );
            lineToCentre.Normalise();

            glColor4f( 0.6f, 0.2f, 0.1f, 0.8f);

            glBegin( GL_QUADS );
                glTexCoord2f(0.75f,0);      glVertex3fv( (centrePos - lineToCentre * 1000).GetData() );
                glTexCoord2f(0.75f,1);      glVertex3fv( (centrePos + lineToCentre * 1000).GetData() );
                glTexCoord2f(0.05f,1);      glVertex3fv( (godRayPos + lineToCentre * 1000).GetData() );
                glTexCoord2f(0.05f,0);      glVertex3fv( (godRayPos - lineToCentre * 1000).GetData() );
            glEnd();
        }
    }

*/


    glDisable       ( GL_TEXTURE_2D );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );

    END_PROFILE( g_app->m_profiler, "Heaven" );
}


void SphereWorld::RenderIslands()
{
    if( g_app->m_camera->IsInMode( Camera::ModeSphereWorldIntro ) ||
        g_app->m_camera->IsInMode( Camera::ModeSphereWorldOutro ) )
    {
        return;
    }

    //
    // Render the islands

    START_PROFILE(g_app->m_profiler, "Islands");

	glMatrixMode(GL_MODELVIEW);

	Vector3 rayStart, rayDir;
	g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );

    Vector3 camRight = g_app->m_camera->GetRight();
    Vector3 camUp = g_app->m_camera->GetUp();

//    glColor4f       ( 1.0f, 1.0f, 1.0f, 1.0f );
    glColor4f       ( 0.6f, 0.2f, 0.1f, 1.0f);
    glDisable       ( GL_DEPTH_TEST );
    glDepthMask     ( false );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_BLEND );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

	for (int i = 0; i < g_app->m_globalWorld->m_locations.Size(); ++i)
	{
		GlobalLocation *loc =g_app->m_globalWorld->m_locations.GetData(i);
        if( loc->m_available || g_app->m_editing )
        {
		    Vector3 islandPos = g_app->m_globalWorld->GetLocationPosition(loc->m_id );

            int numRedraws = 5;
            if( !loc->m_missionCompleted &&
                stricmp( loc->m_missionFilename, "null" ) != 0 &&
                fmodf( g_gameTime, 1.0f ) < 0.7f ) numRedraws = 10;

            glBegin( GL_QUADS );
            for( int j = 0; j <= numRedraws; ++j )
            {
                glTexCoord2i(0,0);      glVertex3fv( (islandPos + camUp * 1000 * j).GetData() );
                glTexCoord2i(1,0);      glVertex3fv( (islandPos + camRight * 1000 * j).GetData() );
                glTexCoord2i(1,1);      glVertex3fv( (islandPos - camUp * 1000 * j).GetData() );
                glTexCoord2i(0,1);      glVertex3fv( (islandPos - camRight * 1000 * j).GetData() );
            }
            glEnd();
        }
	}

    glDisable   ( GL_TEXTURE_2D );
    glDisable   ( GL_BLEND );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask ( true );
    glEnable    ( GL_DEPTH_TEST );


	//
    // Render the islands names

    glColor4f(1.0f,1.0f,1.0f,1.0f);

	for (int i = 0; i < g_app->m_globalWorld->m_locations.Size(); ++i)
	{
		GlobalLocation *loc = g_app->m_globalWorld->m_locations.GetData(i);
        if( loc->m_available || g_app->m_editing )
        {
		    Vector3 islandPos = g_app->m_globalWorld->GetLocationPosition(loc->m_id );
            char *islandName = strdup( g_app->m_globalWorld->GetLocationNameTranslated( loc->m_id ) );
            strupr(islandName);

            float size = 5.0f * sqrtf(( g_app->m_camera->GetPos() - islandPos ).Mag());
            size = 1000.0f;

            g_gameFont.SetRenderShadow( true );
            glColor4f(0.7f,0.7f,0.7f,0.0f);
            g_gameFont.DrawText3DCentre(islandPos + camUp * size*1.5f, size*3.0f, islandName );

            if( g_app->m_editing )
            {
                g_gameFont.DrawText3DCentre(islandPos, size, loc->m_mapFilename );
		        g_gameFont.DrawText3DCentre(islandPos - camUp * size, size, loc->m_missionFilename);
            }

            islandPos += camUp * size * 0.3f;
            islandPos += camRight * size * 0.1f;

            g_gameFont.SetRenderShadow( false );
            glColor4f(1.0f,1.0f,1.0f,1.0f);
            if( stricmp(loc->m_missionFilename, "null" ) == 0 ) glColor4f(0.5f,0.5f,0.5f,1.0f);

            g_gameFont.DrawText3DCentre(islandPos + camUp * size*1.5f, size*3.0f, islandName );

            if( g_app->m_editing )
            {
                g_gameFont.DrawText3DCentre(islandPos, size, loc->m_mapFilename );
		        g_gameFont.DrawText3DCentre(islandPos - camUp * size, size, loc->m_missionFilename);
            }

            free(islandName);
        }
	}

    END_PROFILE(g_app->m_profiler, "Islands");
}


// ****************************************************************************
// Class SphereWorld
// ****************************************************************************


GlobalWorld::GlobalWorld()
:   m_nextBuildingId(0),
    m_nextLocationId(0),
	m_locationRequested(-1),
    m_myTeamId(255),
    m_editorMode(0),
    m_editorSelectionId(-1)
{
    m_globalInternet = new GlobalInternet();
    m_sphereWorld = new SphereWorld();
    m_research = new GlobalResearch();
}


GlobalWorld::GlobalWorld(GlobalWorld &_other)
:	m_globalInternet(NULL),
    m_sphereWorld(NULL),
	m_myTeamId(_other.m_myTeamId),
	m_nextLocationId(0),
	m_nextBuildingId(0),
	m_locationRequested(-1)
{
    m_research = new GlobalResearch();

    for (int i = 0; i < _other.m_locations.Size(); ++i)
	{
		GlobalLocation *newLoc = new GlobalLocation(*(_other.m_locations[i]));
		m_locations.PutDataAtEnd(newLoc);
	}
    for (int i = 0; i < _other.m_buildings.Size(); ++i)
	{
		GlobalBuilding *newBuilding = new GlobalBuilding(*(_other.m_buildings[i]));
		m_buildings.PutDataAtEnd(newBuilding);
	}
    for (int i = 0; i < _other.m_events.Size(); ++i)
	{
		GlobalEvent *newEvent = new GlobalEvent(*(_other.m_events[i]));
		m_events.PutDataAtEnd(newEvent);
	}
}


GlobalWorld::~GlobalWorld()
{
    m_locations.EmptyAndDelete();
    m_buildings.EmptyAndDelete();
    m_events.EmptyAndDelete();

    delete m_globalInternet;
    delete m_sphereWorld;
    delete m_research;
}


void GlobalWorld::Advance()
{
	if (g_app->m_editing)
	{
        if( m_editorMode == 0 )
        {
            // Edit locations
		    if ( g_inputManager->controlEvent( ControlSelectLocation ) )
		    {
			    Vector3 rayStart, rayDir;
				g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );
			    int locId = LocationHit(rayStart, rayDir);
			    if (locId != -1)
			    {
				    GlobalLocation *loc = GetLocation( locId );
				    g_app->m_requestedLocationId = locId;
				    strcpy(g_app->m_requestedMission, loc->m_missionFilename);
				    strcpy(g_app->m_requestedMap, loc->m_mapFilename);
				}
		    }
        }
        else
        {
            // Move locations
            if( g_inputManager->controlEvent( ControlSelectLocation ) )
            {
			    Vector3 rayStart, rayDir;
			    g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );
			    m_editorSelectionId = LocationHit(rayStart, rayDir);
            }
            else if( g_inputManager->controlEvent( ControlLocationDragActive ) )
            {
                GlobalLocation *loc = GetLocation( m_editorSelectionId );
                if( loc )
                {
                	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();
                    loc->m_pos = mousePos3D / 120.0f;
                }
            }
            else if( g_inputManager->controlEvent( ControlDeselectLocation ) )
            {
                m_editorSelectionId = -1;
            }
        }
	}
	else
	{
		bool chatLog = g_app->m_sepulveda->ChatLogVisible();

		// Has the user clicked on a location?
		if ( g_inputManager->controlEvent( ControlSelectLocation ) &&
		     m_locationRequested == -1 &&
		     EclGetWindows()->Size() == 0
			 && !chatLog )
		{
			Vector3 rayStart, rayDir;
			g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );
			int locId = LocationHit(rayStart, rayDir);
			if (locId != -1)
			{
				GlobalLocation *loc = GetLocation( locId );
				if( strcmp( loc->m_missionFilename, "null" ) != 0 &&
					loc->m_available )
				{
					if (!g_app->m_script->IsRunningScript()) {
						if ( !g_app->HasBoughtGame() ) {
							// We're not registered, we should run a script to end
							if (!(strcmp(loc->m_mapFilename, "map_garden.txt") == 0 ||
								  strcmp(loc->m_mapFilename, "map_containment.txt") == 0)) {

								// Buy me URL
								EclRegisterWindow( new BuyNowWindow );

								// Bar Location
								return;
							}
						}
					}

					// Default behaviour is to go the location
					m_locationRequested = locId;
					g_app->m_renderer->StartFadeOut();
				}
			}
		}
		// Is the cursor attracted to a point?
		else if ( m_locationRequested == -1 && EclGetWindows()->Size() == 0 && !chatLog ) {
			Vector3 rayStart, rayDir;
			g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );
			int locId = LocationHit(rayStart, rayDir, 10000.0f);
			if (locId != -1)
			{
				// We're close to a location, but not there, so drag the pointer towards it
				GlobalLocation *loc = GetLocation( locId );
				float locX, locY;
				g_app->m_camera->Get2DScreenPos( loc->m_pos, &locX, &locY );
				locY = g_app->m_renderer->ScreenH() - locY;
				int movX = int(locX - g_target->X());
				int movY = int(locY - g_target->Y());
				int movMag2 = movX * movX + movY * movY;
				int movFactor = 30 / movMag2;
				if ( movFactor > 0 )
					g_target->MoveCursor( movX * movFactor, movY * movFactor );
			}
		}

		// Has the fade out finished?
		if (m_locationRequested != -1 && g_app->m_renderer->IsFadeComplete())
		{
			GlobalLocation *loc = GetLocation( m_locationRequested );
			g_app->m_requestedLocationId = m_locationRequested;
			strcpy(g_app->m_requestedMission, loc->m_missionFilename);
			strcpy(g_app->m_requestedMap, loc->m_mapFilename);

			m_locationRequested = -1;
		}
	}
}


void GlobalWorld::Render()
{
    START_PROFILE(g_app->m_profiler, "Render Global World");

	if( !g_app->m_editing ) m_globalInternet->Render();
	CHECK_OPENGL_STATE();
    m_sphereWorld->Render();
	CHECK_OPENGL_STATE();

	END_PROFILE(g_app->m_profiler, "Render Global World");
}


// Returns the ID of the location the line intersects. Returns -1 if line
// does not intersect any location
int	GlobalWorld::LocationHit(Vector3 const &_pos, Vector3 const &_dir, float locationRadius)
{
    //float locationRadius = 5000.0f;

    for( int i = 0; i < m_locations.Size(); ++i )
    {
        GlobalLocation *gl = m_locations[i];
        Vector3 locPos = GetLocationPosition( gl->m_id );

		bool hit = RaySphereIntersection(_pos, _dir, locPos, locationRadius);
		if (hit)
		{
			return gl->m_id;
		}
    }

    return -1;
}


GlobalLocation *GlobalWorld::GetLocation( int _id )
{
    for( int i = 0; i < m_locations.Size(); ++i )
    {
        GlobalLocation *loc = m_locations[i];
        if( loc->m_id == _id )
        {
            return loc;
        }
    }

    return NULL;
}


GlobalLocation *GlobalWorld::GetHighlightedLocation()
{
	int screenX = g_target->X();
	int screenY = g_target->Y();

    Vector3 rayStart, rayDir;
	g_app->m_camera->GetClickRay(screenX, screenY, &rayStart, &rayDir);
	int locId = g_app->m_globalWorld->LocationHit(rayStart, rayDir);

    GlobalLocation *loc = GetLocation( locId );

    if( loc && loc->m_available ) return loc;
    if( loc && g_app->m_editing ) return loc;

    return NULL;
}


GlobalLocation *GlobalWorld::GetLocation( char const *_name )
{
    int locationId = GetLocationId( _name );
    if( locationId != -1 )
    {
        return GetLocation( locationId );
    }
    return NULL;
}


int GlobalWorld::GetLocationId( char const *_name )
{
    for( int i = 0; i < m_locations.Size(); ++i )
    {
        GlobalLocation *loc = m_locations[i];
        DarwiniaDebugAssert(loc);
        if( stricmp(loc->m_name, _name) == 0 )
        {
            return loc->m_id;
        }
    }

    return -1;
}


int GlobalWorld::GetLocationIdFromMapFilename(char const *_mapFilename)
{
	char buf[MAX_FILENAME_LEN];
	strcpy(buf, _mapFilename);
	char *mapName = strstr(buf, "map_") + 4;
	mapName[strlen(mapName) - 4] = '\0';

	return GetLocationId(mapName);
}


char *GlobalWorld::GetLocationName( int _id )
{
    GlobalLocation *loc = GetLocation( _id );
    if( loc ) return loc->m_name;
    return NULL;
}


char *GlobalWorld::GetLocationNameTranslated( int _id )
{
    GlobalLocation *loc = GetLocation( _id );
    if( !loc ) return NULL;

    char stringId[256];
    sprintf( stringId, "location_%s", loc->m_name );

    if( ISLANGUAGEPHRASE( stringId ) )
    {
        return LANGUAGEPHRASE( stringId );
    }
    else
    {
        return loc->m_name;
    }
}


Vector3 GlobalWorld::GetLocationPosition(int _id)
{
    GlobalLocation *location = GetLocation(_id);
    if( !location ) return g_zeroVector;
    return location->m_pos * 120.0f;
}


GlobalBuilding *GlobalWorld::GetBuilding( int _id, int _locationId )
{
    if( _id == -1 || _locationId == -1 ) return NULL;

    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        GlobalBuilding *buil = m_buildings[i];
        if( buil->m_id == _id && buil->m_locationId == _locationId )
        {
            return buil;
        }
    }

    return NULL;
}


void GlobalWorld::AddLocation( GlobalLocation *location )
{
    if( location->m_id == -1 )
    {
        location->m_id = m_nextLocationId;
        m_nextLocationId++;
    }
    else if( location->m_id >= m_nextLocationId )
    {
        m_nextLocationId = location->m_id + 1;
    }

    m_locations.PutData( location );
    m_sphereWorld->AddLocation( location->m_id );
}


void GlobalWorld::AddBuilding( GlobalBuilding *building )
{
    if( building->m_id == -1 )
    {
        building->m_id = m_nextBuildingId;
        m_nextBuildingId++;
    }
    else if( building->m_id >= m_nextBuildingId )
    {
        m_nextBuildingId = building->m_id + 1;
    }

    m_buildings.PutData( building );
}


void GlobalWorld::WriteLocations( FileWriter *_out )
{
    _out->printf( "Locations_StartDefinition\n" );
    _out->printf( "\t# Id  Avail                   mapFile                    missionFile\n" );
    _out->printf( "\t# ==================================================================\n" );

    for( int i = 0; i < m_locations.Size(); ++i )
    {
        GlobalLocation *location = m_locations[i];
        _out->printf( "\t%4d %4d %30s %40s\n",
                         location->m_id, int(location->m_available), location->m_mapFilename, location->m_missionFilename );
    }

    _out->printf( "Locations_EndDefinition\n\n" );
}


void GlobalWorld::WriteBuildings( FileWriter *_out )
{
    _out->printf( "Buildings_StartDefinition\n" );
    _out->printf( "\t# Id  teamId  locId   type   link  online\n" );
    _out->printf( "\t# =======================================\n" );

    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        GlobalBuilding *building = m_buildings[i];
        _out->printf( "\t%4d %4d %6d %6d %6d %6d\n",
                        building->m_id, building->m_teamId, building->m_locationId, building->m_type, building->m_link, building->m_online );
    }

    _out->printf( "Buildings_EndDefinition\n\n" );
}


void GlobalWorld::WriteEvents(FileWriter *_out)
{
    _out->printf( "Events_StartDefinition\n" );

    for (int i = 0; i < m_events.Size(); ++i)
    {
        GlobalEvent *ge = m_events[i];
        ge->Write(_out);
    }

    _out->printf( "Events_EndDefinition\n" );
}


void GlobalWorld::ParseLocations( TextReader *_in )
{
    while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;

		char *word = _in->GetNextToken();

		if (stricmp(word, "Locations_EndDefinition") == 0)
		{
			return;
		}

        GlobalLocation *location = new GlobalLocation();

		location->m_id          = atoi(word);
        location->m_available   = bool( atoi(_in->GetNextToken() ) );

        strcpy( location->m_mapFilename, _in->GetNextToken());
        strcpy( location->m_missionFilename, _in->GetNextToken());

        strcpy( location->m_name, (location->m_mapFilename+4) );
        location->m_name[ strlen(location->m_name)-4 ] = '\x0';

        AddLocation( location );
    }
}


void GlobalWorld::ParseBuildings( TextReader *_in )
{
    while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;

		char *word = _in->GetNextToken();

		if (stricmp(word, "buildings_enddefinition") == 0)
		{
			return;
		}

        GlobalBuilding *building = new GlobalBuilding();

        building->m_id          = atoi(word);
        building->m_teamId      = atoi(_in->GetNextToken());
        building->m_locationId  = atoi(_in->GetNextToken());
        building->m_type        = atoi(_in->GetNextToken());
        building->m_link        = atoi(_in->GetNextToken());
        building->m_online      = atoi(_in->GetNextToken());

        AddBuilding( building );
    }
}


void GlobalWorld::ParseEvents(TextReader *_in)
{
    while(_in->ReadLine())
    {
		if (!_in->TokenAvailable()) continue;
        char *word = _in->GetNextToken();

        if( stricmp(word, "events_enddefinition") == 0)
        {
            return;
        }

        DarwiniaDebugAssert( stricmp( word, "Event" ) == 0 );

        GlobalEvent *event = new GlobalEvent();
        event->Read( _in );
        m_events.PutData( event );
    }
}


void GlobalWorld::AddLevelBuildingToGlobalBuildings(Building *_building, int _locId)
{
	if( _building->m_isGlobal )
	{
		GlobalBuilding *gb = GetBuilding(_building->m_id.GetUniqueId(), _locId);
		if( !gb )
		{
			gb = new GlobalBuilding();
			gb->m_type = _building->m_type;
			gb->m_locationId = _locId;
			gb->m_id = _building->m_id.GetUniqueId();
            gb->m_teamId = _building->m_id.GetTeamId();
			m_buildings.PutData(gb);

			if( _building->m_type == Building::TypeTrunkPort )
			{
				gb->m_link = ((TrunkPort *)_building)->m_targetLocationId;
			}
		}
		gb->m_pos = _building->m_pos;
	}
}

void GlobalWorld::LoadGame( char *_filename )
{
    TextReader *in = NULL;
    char fullFilename[256];

    if( !g_app->m_editing )
    {
        sprintf( fullFilename, "%susers/%s/%s", g_app->GetProfileDirectory(), g_app->m_userProfileName, _filename );
        if( DoesFileExist( fullFilename ) ) in = new TextFileReader( fullFilename );
    }

    if( !in )
    {
        in = g_app->m_resource->GetTextReader(_filename);
    }

    if( in )
    {
	    while(in->ReadLine())
        {
			if (!in->TokenAvailable()) continue;
		    char *word = in->GetNextToken();

			if (stricmp("locations_startdefinition", word) == 0)
		    {
                ParseLocations( in );
		    }
            else if (stricmp("buildings_startdefinition", word) == 0)
            {
                ParseBuildings( in );
            }
            else if (stricmp("events_startdefinition", word) == 0)
            {
                ParseEvents( in );
            }
            else if (stricmp("research_startdefinition", word) == 0)
            {
                m_research->Read( in );
            }
            else if (stricmp("tutorial_startdefinition", word) == 0)
            {
                ParseTutorial( in );
            }
        }
    }

	delete in;
	in = NULL;

    //
    // Load locations

    LoadLocations( "locations.txt" );


    //
    // Load all map files into memory

	for (int i = 0; i < m_locations.Size(); i++)
	{
		if (!m_locations.ValidIndex(i)) continue;

		// Get the next location
		GlobalLocation *loc = m_locations.GetData(i);

		// Load all the level files for the location
        LevelFile levFile("null", loc->m_mapFilename);
		for( int b = 0; b < levFile.m_buildings.Size(); ++b )
		{
			Building *building = levFile.m_buildings[b];
			AddLevelBuildingToGlobalBuildings(building, loc->m_id);
		}


        char filter[256];
		sprintf(filter, "mission_%s*.txt", GetLocationName(loc->m_id));
		LList<char *> *missionFileNames = g_app->m_resource->ListResources("levels/", filter, false);
		for (int j = 0; j < missionFileNames->Size(); ++j)
		{
			LevelFile levFile(missionFileNames->GetData(j), loc->m_mapFilename);

			for( int b = 0; b < levFile.m_buildings.Size(); ++b )
			{
				Building *building = levFile.m_buildings[b];
				AddLevelBuildingToGlobalBuildings(building, loc->m_id);

                if( building->m_type == Building::TypeAntHill ||
                    building->m_type == Building::TypeTriffid ||
                    building->m_type == Building::TypeIncubator )
                {
                    if( !building->m_dynamic )
                    {
                        DebugOut( "%s found on level %s should be dynamic (otherwise save games wont work)\n",
                                    Building::GetTypeName(building->m_type),
                                    GetLocationName(loc->m_id) );
                    }
                }
			}

            bool objectivesComplete = true;
	        for (int i = 0; i < levFile.m_primaryObjectives.Size(); ++i)
	        {
		        GlobalEventCondition *gec = levFile.m_primaryObjectives[i];
		        if (!gec->Evaluate())
		        {
			        objectivesComplete = false;
                    break;
		        }
	        }

            if( objectivesComplete )
            {
                loc->m_missionCompleted = true;
            }
		}
		missionFileNames->EmptyAndDeleteArray();
		delete missionFileNames;
	}

    EvaluateEvents();
}


void GlobalWorld::SaveGame( char *_filename )
{
    FileWriter *out = NULL;
    char fullFilename[256];

    if( !g_app->m_editing && stricmp( g_app->m_userProfileName, "none" ) != 0 )
    {
        sprintf( fullFilename, "%susers/%s/%s", g_app->GetProfileDirectory(), g_app->m_userProfileName, _filename );
#ifdef TARGET_DEBUG
        out = new FileWriter(fullFilename, false);
#else
        out = new FileWriter(fullFilename, true);
#endif
    }

    if( !out )
    {
        out = g_app->m_resource->GetFileWriter( _filename, false );
    }

    WriteLocations(out);
    WriteBuildings(out);
    m_research->Write(out);
    WriteTutorial(out);
    WriteEvents(out);

    delete out;
}


void GlobalWorld::WriteTutorial(FileWriter *_out)
{
#ifdef DEMO2
    int currentChapter = -1;
    if( g_app->m_tutorial ) currentChapter = g_app->m_tutorial->GetCurrentChapter();

    _out->printf( "Tutorial_StartDefinition\n" );

    _out->printf( "\tCurrentChapter = %d\n", currentChapter );

    _out->printf( "Tutorial_EndDefinition\n\n" );
#endif
}


void GlobalWorld::ParseTutorial(TextReader *_in)
{
    while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;

		char *word = _in->GetNextToken();

		if (stricmp(word, "tutorial_enddefinition") == 0)
		{
			return;
		}

        char *temp = _in->GetNextToken();
        int chapter = atoi( _in->GetNextToken() );

        if( g_app->m_tutorial )
        {
            if( chapter == -1 )
            {
                delete g_app->m_tutorial;
                g_app->m_tutorial = NULL;
            }
            else
            {
                g_app->m_tutorial->SetChapter( chapter );
            }
        }
    }
}


void GlobalWorld::LoadLocations(char *_filename)
{
    TextReader *in = g_app->m_resource->GetTextReader(_filename);

	while(in->ReadLine())
    {
		if (!in->TokenAvailable()) continue;

        int locIndex    = atoi( in->GetNextToken() );
        float posX      = atof( in->GetNextToken() );
        float posY      = atof( in->GetNextToken() );
        float posZ      = atof( in->GetNextToken() );

        GlobalLocation *location = GetLocation( locIndex );
        if( location ) location->m_pos.Set( posX, posY, posZ );
    }

    delete in;
}


void GlobalWorld::SaveLocations(char *_filename)
{
    FileWriter *out = g_app->m_resource->GetFileWriter( _filename, false );

    out->printf( "# ================================\n" );
    out->printf( "# id   x        y        z\n" );
    out->printf( "# ================================\n\n" );

    for( int i = 0; i < m_locations.Size(); ++i )
    {
        GlobalLocation *loc = m_locations[i];
        out->printf( "%-6d %-8.2f %-8.2f %-8.2f\n", loc->m_id, loc->m_pos.x, loc->m_pos.y, loc->m_pos.z );
    }

    delete out;
}


// Find the lowest unused building ID in the current location
int GlobalWorld::GenerateBuildingId()
{
    int id = 0;
    while( true )
    {
        if( !g_app->m_location->GetBuilding(id) )
        {
            break;
        }
        ++id;
    }
    return id;
}


// Checks to see if any event's conditions are true. If they are, the first action
// for that event will be executed. That event action is then deleted, and if there
// are no more actions for the event, then the event is deleted too.
// Returns true if actions remain to be completed
bool GlobalWorld::EvaluateEvents()
{
    if( g_app->m_script &&
        g_app->m_script->IsRunningScript() ) return true;

    for( int i = 0; i < m_events.Size(); ++i )
    {
        GlobalEvent *event = m_events[i];

        if( event->Evaluate() )
        {
            event->MakeAlwaysTrue();
            bool amIDone = event->Execute();
            if( amIDone )
            {
                m_events.RemoveData(i);
                delete event;
                --i;
            }
			return true;
        }
    }

	return false;
}


void GlobalWorld::TransferSpirits(int _locationId)
{
    //
    // Count how many spirits remain on the location

    DarwiniaDebugAssert( g_app->m_location );
    int remainingSpirits = g_app->m_location->m_spirits.NumUsed();

    GlobalLocation *location = GetLocation( _locationId );
    DarwiniaReleaseAssert(location, "GlobalWorld::TransferSpirits, failed to lookup location %d", _locationId );

    int spiritCount = location->m_numSpirits + remainingSpirits/2;
    location->m_numSpirits = remainingSpirits/2;

	float position = 1.0f;
	for( int i = 0; i < spiritCount; ++i )
	{
		m_sphereWorld->m_spirits[_locationId].PutDataAtStart( position );
		position -= frand(0.04f);
	}
}


void GlobalWorld::SetupLights()
{
	float black[] = { 0, 0, 0, 0 };
    float colour1[] = { 2.0f, 1.5f, 0.75f, 1.0f };

	Vector3 light0(0, 1, 0);
	light0.Normalise();
	GLfloat light0AsFourFloats[] = { light0.x, light0.y, light0.z, 0.0f };

	glLightfv(GL_LIGHT0, GL_POSITION, light0AsFourFloats);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, colour1);
	glLightfv(GL_LIGHT0, GL_SPECULAR, colour1);
	glLightfv(GL_LIGHT0, GL_AMBIENT, black);

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHTING);
}


void GlobalWorld::SetupFog()
{
    float fog = 0.1f;
    float fogCol[] = { fog, fog, fog, fog };

    glFogf      ( GL_FOG_DENSITY, 1.0f );
    glFogf      ( GL_FOG_START, 0.0f );
    glFogf      ( GL_FOG_END, 19000.0f );
    glFogfv     ( GL_FOG_COLOR, fogCol );
    glFogi      ( GL_FOG_MODE, GL_LINEAR );
    //glEnable    (GL_FOG);
}


float GlobalWorld::GetSize()
{
	if (g_app->m_location) return g_app->m_location->m_landscape.GetWorldSizeX();

	return 2e5;
}
