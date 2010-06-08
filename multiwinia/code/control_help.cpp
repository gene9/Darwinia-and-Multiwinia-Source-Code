#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/language_table.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/hi_res_time.h"
#include "lib/preferences.h"
#include "lib/input/input.h"
#include "lib/input/input_types.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/bitmap.h"

#include "control_help.h"
#include "gamecursor.h"
#include "camera.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "main.h"
#include "app.h"
#include "renderer.h"
#include "location.h"
#include "taskmanager.h"
#include "team.h"
#include "unit.h"
#include "global_world.h"
#include "script.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h" 
#endif
#include "user_input.h"

#include "interface/prefs_other_window.h"

#include "worldobject/radardish.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/officer.h"
#include "worldobject/armour.h"

#include <vector>


// HelpIcon ------------------
//	(This class is mainly good for rendering).

class HelpIcon
{
public:
    HelpIcon( const char *_filename, const char *_shadowFilename, const Vector2 &_setRelativePos );
	
	void AddTextPosition( int _relx, int _rely );

	bool Enabled() const;
	bool Enabled( float &_iconAlpha ) const;

	void Clear();
	void Set( int index, const UnicodeString _helpText, float _alpha );

    void Render( const Vector2 &_setPosition, float _alpha );

private:
    char m_filename[256], m_shadowFilename[256];
    Vector2 m_setRelativePos;

	struct PosText {		
		Vector2 m_pos;
		UnicodeString m_text;
		float m_alpha;
	};

	std::vector<PosText> m_texts, m_lastTexts;

	double m_lastHelpTextChangeTime;
};

void HelpIcon::AddTextPosition( int _relx, int _rely )
{
	PosText t;

	t.m_pos.x = _relx;
	t.m_pos.y = _rely;
	t.m_text = "";
	t.m_alpha = 1.0;

	m_texts.push_back( t );
}

HelpIcon::HelpIcon( const char *_filename, const char *_shadowFilename, const Vector2 &_setRelativePos )
:  m_setRelativePos( _setRelativePos )
{
	strcpy( m_filename, _filename );
	strcpy(m_shadowFilename, _shadowFilename);
}

inline
bool HelpIcon::Enabled() const
{
	float iconAlpha;
	return Enabled( iconAlpha );
}

bool HelpIcon::Enabled( float &_iconAlpha ) const
{
	bool enabled = false;

	_iconAlpha = 0.3;	

	for (int i = 0; i < m_texts.size(); i++) {
		if (wcscmp(m_texts[i].m_text.m_unicodestring, UnicodeString().m_unicodestring) != 0) {
			enabled = true;
			if (m_texts[i].m_alpha > _iconAlpha)
				_iconAlpha = m_texts[i].m_alpha;
		}
	}

	return enabled;
}

void HelpIcon::Clear()
{
	m_lastTexts = m_texts;

	for (int i = 0; i < m_texts.size(); i++)
		m_texts[i].m_text = "";
}

void HelpIcon::Set( int _index, const UnicodeString _helpText, float _alpha )
{
	AppDebugAssert( _helpText.m_unicodestring != NULL );

	m_texts[_index].m_text = _helpText;
	m_texts[_index].m_alpha = _alpha;
	
	if (wcscmp(m_texts[_index].m_text.m_unicodestring, m_lastTexts[_index].m_text.m_unicodestring)!=0) 
		m_lastHelpTextChangeTime = g_gameTime;
}


void HelpIcon::Render( const Vector2 &_setPosition, float _alpha )
{
    float iconSize = 50.0;
    float iconGap = 10.0;

	float iconAlpha;
    float shadowOffset = 0;
    float shadowSize = iconSize; 

	const bool enabled = Enabled( iconAlpha );

	if (enabled) {
		// Check to see whether we should blink
		double elapsedTime = g_gameTime - m_lastHelpTextChangeTime;
		if (elapsedTime < 1.5) {
			iconAlpha = 0.75 * iconAlpha + 0.25 * iconAlpha * sin( 8 * elapsedTime );
		}
	}

	iconAlpha *= _alpha;

	Vector2 position = _setPosition + m_setRelativePos;

	Vector2 iconCentre = Vector2(position.x, position.y);

	//if( g_app->m_largeMenus ) 
	//{
	//	iconGap *= 2.0;
	//	iconSize *= 1.5;
	//	shadowSize *= 1.5;
	//	iconCentre.x -= (g_app->m_renderer->ScreenW() - iconCentre.x ) * 0.5;
	//	iconCentre.y += iconGap;
	//}

    float shadowX = shadowSize/2;

	// Render the shadow
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( m_shadowFilename ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glDepthMask     ( false );
    glColor4f       ( iconAlpha, iconAlpha, iconAlpha, 0.0 );         
	
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowX + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
    glEnd();	

    unsigned int texId = g_app->m_resource->GetTexture( m_filename );       

	// Render the icon
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, texId );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    
    glColor4f   ( 1.0, 1.0, 1.0, iconAlpha );

    float x = iconSize/2;

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - x, iconCentre.y - iconSize/2 );
        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + x, iconCentre.y - iconSize/2 );
        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + x, iconCentre.y + iconSize/2 );
        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - x, iconCentre.y + iconSize/2 );
    glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );

	// Render the associated text

	if (enabled) {
		for (int i = 0; i < m_texts.size(); i++) {
			UnicodeString text = m_texts[i].m_text;
			const Vector2 &pos = m_texts[i].m_pos;

			if (wcscmp(text.m_unicodestring, UnicodeString().m_unicodestring) == 0)
				continue;

			float fontSize = 12.0;
			//if( g_app->m_largeMenus )
			//{
			//	fontSize *= 1.5;
			//}
			
			Vector2 textCentrePos( 
				iconCentre.x + (shadowSize / 2 + iconGap) * pos.x,
				iconCentre.y + (shadowSize / 2 + iconGap) * pos.y);

			g_gameFont.SetRenderOutline(true);
			glColor4f(1.0,1.0,1.0,0.0);
			g_gameFont.DrawText2DJustified( textCentrePos.x, textCentrePos.y, fontSize, pos.x, text );

			g_gameFont.SetRenderOutline(false);
			glColor4f(1.0,1.0,1.0,iconAlpha);
			g_gameFont.DrawText2DJustified( textCentrePos.x, textCentrePos.y, fontSize, pos.x, text );
		}
	}
}

//  HelpIconSet ------------------
//	(This class groups sets of icons together)

class HelpIconSet {
public:
	HelpIconSet( int _height );

	void AddIcon( HelpIcon *_icon );
	void Render( Vector2 &_setPosition );

private:
	std::vector<HelpIcon *> m_icons;
	int m_height;
	double m_beginFadeOutTime, m_beginFadeInTime;
	bool m_lastEnabled;
};

HelpIconSet::HelpIconSet( int _height )
:	m_height( _height ),
	m_beginFadeOutTime( 0.0 ),
	m_beginFadeInTime( 0.0 ),
	m_lastEnabled( false )
{
}

void HelpIconSet::AddIcon( HelpIcon *_icon )
{
	m_icons.push_back( _icon );
}

void HelpIconSet::Render( Vector2 &_setPosition )
{
	// Are any of the icons enabled?
	bool atLeastOneEnabled = false;

	float iconAlpha;

	for (int i = 0; i < m_icons.size(); i++)
		if (m_icons[i]->Enabled( iconAlpha )) {
			atLeastOneEnabled = true;
			break;
		}

	if (!atLeastOneEnabled && m_lastEnabled) 
		m_beginFadeOutTime = g_gameTime;

	else if (atLeastOneEnabled && !m_lastEnabled) 
		m_beginFadeInTime = g_gameTime;

	float alpha;
	const float fadeDuration = 1.0;

	if (m_beginFadeOutTime > 0.0) {
		alpha = 1.0 - (g_gameTime - m_beginFadeOutTime) / fadeDuration;
		if (alpha < 0.0) {
			m_beginFadeOutTime = 0.0;
			alpha = 0.0;
		}
	}
	else if (m_beginFadeInTime > 0.0) {
		alpha = (g_gameTime - m_beginFadeInTime) / fadeDuration;
		if (alpha > 1.0) {
			m_beginFadeInTime = 0.0;
			alpha = 1.0;
		}
	}
	else {
		alpha = atLeastOneEnabled ? 1.0 : 0.0;
	}

	// If none enabled, we don't render the entire set
	if (alpha > 0.0) {
		// Render
		for (int i = 0; i < m_icons.size(); i++) 
			m_icons[i]->Render( _setPosition, alpha );
	}	
	
	// Adjust the y value
	_setPosition.y += m_height;
	m_lastEnabled = atLeastOneEnabled;
}

ControlHelpSystem::ControlHelpSystem()
{
	memset( m_icons, 0, sizeof(m_icons) );
	memset( m_sets, 0, sizeof(m_sets) );

	InitialiseIcons();
	InitialiseConditions();

	//for( int i = 0; i < NUM_ON_SCREEN_AIDS; i++ )
	//	m_onScreenAids[i] = NULL;
}

ControlHelpSystem::~ControlHelpSystem()
{
	for( int i = 0; i < MaxIcons; i++ )
	{
		delete m_icons[i];
		m_icons[i] = NULL;
	}

	for( int i = 0; i < MaxSets; i++ )
	{
		delete m_sets[i];
		m_sets[i] = NULL;
	}
}


void ControlHelpSystem::InitialiseIcons()
{
	int set = -1;
	// Set up the icons

	m_icons[A] = new HelpIcon("icons/button_a.bmp", "icons/button_abxy_shadow.bmp", Vector2(50, 100));
	m_icons[A]->AddTextPosition( 0, 1 );

	m_icons[B] = new HelpIcon("icons/button_b.bmp", "icons/button_abxy_shadow.bmp", Vector2(100, 50));
	m_icons[B]->AddTextPosition( 1, 0 );	

	m_icons[X] = new HelpIcon("icons/button_x.bmp", "icons/button_abxy_shadow.bmp", Vector2(0, 50));
	m_icons[X]->AddTextPosition( -1, 0 );	

	m_icons[Y] = new HelpIcon("icons/button_y.bmp", "icons/button_abxy_shadow.bmp", Vector2(50, 0));
	m_icons[Y]->AddTextPosition( 0, -1 );	

	m_icons[DPAD] = new HelpIcon("icons/button_dpad.bmp", "icons/button_control_shadow.bmp", Vector2(50, 0));
	m_icons[DPAD]->AddTextPosition( 1, 0 );
	m_icons[DPAD]->AddTextPosition( 0, -1 );	
	m_icons[DPAD]->AddTextPosition( 0, 1 );

	const int hsep = 25;

	m_icons[LB] = new HelpIcon("icons/button_lb.bmp", "icons/button_lb_shadow.bmp", Vector2(-hsep + 25, 0));
	m_icons[LB]->AddTextPosition( -1, 0 );	

	m_icons[RB] = new HelpIcon("icons/button_rb.bmp", "icons/button_rb_shadow.bmp", Vector2(+hsep + 75, 0));
	m_icons[RB]->AddTextPosition( 1, 0 );	

	const int vsep = -25;
	const int yoffset = -25;

	m_icons[LA] = new HelpIcon("icons/button_la.bmp", "icons/button_control_shadow.bmp", Vector2(-hsep + 50, yoffset + -vsep + 0));
	m_icons[LA]->AddTextPosition( 1, 0 );	

	m_icons[LT] = new HelpIcon("icons/button_lt.bmp", "icons/button_trigger_shadow.bmp", Vector2(-hsep + 0, yoffset + -vsep + 0));
	m_icons[LT]->AddTextPosition( 0, 1 );	

	m_icons[RA] = new HelpIcon("icons/button_ra.bmp", "icons/button_control_shadow.bmp", Vector2(+hsep + 50, yoffset + +vsep + 100));
	m_icons[RA]->AddTextPosition( -1, 0 );	

	m_icons[RT] = new HelpIcon("icons/button_rt.bmp", "icons/button_trigger_shadow.bmp", Vector2(+hsep + 100, yoffset + +vsep + 100));
	m_icons[RT]->AddTextPosition( 0, 1 );	

	m_sets[++set] = new HelpIconSet( 175 );
	m_sets[set]->AddIcon( m_icons[A] );
	m_sets[set]->AddIcon( m_icons[B] );
	m_sets[set]->AddIcon( m_icons[X] );
	m_sets[set]->AddIcon( m_icons[Y] );	

	m_sets[++set] = new HelpIconSet( 75 );
	m_sets[set]->AddIcon( m_icons[LB] );
	m_sets[set]->AddIcon( m_icons[RB] );

	m_sets[++set] = new HelpIconSet( 150 );
	m_sets[set]->AddIcon( m_icons[LA] );
	m_sets[set]->AddIcon( m_icons[RA] );
	// m_sets[++set] = new HelpIconSet( 150 );
	m_sets[set]->AddIcon( m_icons[LT] );
	m_sets[set]->AddIcon( m_icons[RT] );

	m_sets[++set] = new HelpIconSet( 75 );
	m_sets[set]->AddIcon( m_icons[DPAD] );

	AppDebugAssert( set < MaxSets );
}

void ControlHelpSystem::InitialiseConditions()
{
	m_conditionIconMap[CondDestroyUnit]				= TextIndicator(Y, 0, "gc_destroy");
	m_conditionIconMap[CondTaskManagerCreateBlue]	= TextIndicator(X, 0, "gc_create");
	m_conditionIconMap[CondTaskManagerCloseBlue]	= TextIndicator(X, 0, "gc_close");
	m_conditionIconMap[CondTaskManagerCloseRed]		= TextIndicator(B, 0, "gc_close");
	m_conditionIconMap[CondDeselectUnit]			= TextIndicator(B, 0, "gc_deselect");
	m_conditionIconMap[CondSelectUnit]				= TextIndicator(A, 0, "gc_select");
	m_conditionIconMap[CondTaskManagerSelect]		= TextIndicator(A, 0, "gc_select");
	m_conditionIconMap[CondTaskManagerCreateGreen]	= TextIndicator(A, 0, "gc_create");
	m_conditionIconMap[CondPlaceUnit]				= TextIndicator(A, 0, "gc_place");
	m_conditionIconMap[CondMoveUnit]				= TextIndicator(A, 0, "gc_move" );
	m_conditionIconMap[CondPromoteOfficer]			= TextIndicator(A, 0, "gc_promote" );
	m_conditionIconMap[CondMoveCameraOrUnit]		= TextIndicator(LA, 0, "gc_move", 2.0, 3.0, 9.0, 0.3 );
	m_conditionIconMap[CondCameraAim]				= TextIndicator(RA, 0, "gc_aim", 2.0, 3.0, 9.0, 0.3 );
	m_conditionIconMap[CondSquaddieFire]			= TextIndicator(RA, 0, "gc_fire", 2.0, 3.0, 9.0 );
	m_conditionIconMap[CondChangeWeapon]			= TextIndicator(DPAD, 0, "gc_changeweapon", 1.0, 2.0, 8.0, 0.3  );
	m_conditionIconMap[CondChangeOrders]			= TextIndicator(DPAD, 0, "gc_changeorders" );
	m_conditionIconMap[CondCameraUp]				= TextIndicator(RT, 0, "gc_cameraup", 1.0, 2.0, 3.0, 0.3 );
	m_conditionIconMap[CondCameraDown]				= TextIndicator(LT, 0, "gc_cameradown", 1.0, 2.0, 3.0, 0.3   );
	m_conditionIconMap[CondZoom]					= TextIndicator(LT, 0, "gc_zoom" );
	m_conditionIconMap[CondFireGrenades]			= TextIndicator(RT, 0, "gc_grenade", 1.0, 2.0, 8.0, 0.3 );
	m_conditionIconMap[CondFireRocket]				= TextIndicator(RT, 0, "gc_rocket", 1.0, 2.0, 8.0, 0.3  );
	m_conditionIconMap[CondFireAirstrike]			= TextIndicator(RT, 0, "gc_airstrike", 1.0, 2.0, 8.0, 0.3  );
	m_conditionIconMap[CondOfficerSetGoto]			= TextIndicator(RT, 0, "gc_setgoto" );
	m_conditionIconMap[CondArmourSetTurret]			= TextIndicator(RT, 0, "gc_setturret" );
	m_conditionIconMap[CondSwitchPrevUnit]			= TextIndicator(LB, 0, "gc_prev" );
	m_conditionIconMap[CondSwitchNextUnit]			= TextIndicator(RB, 0, "gc_next" );
    m_conditionIconMap[CondRadarAim]                = TextIndicator(A, 0, "control_help_radar" );
	m_conditionIconMap[CondRotateCameraLeft]        = TextIndicator(LT, 0, "control_help_camerarotateleft" );
	m_conditionIconMap[CondRotateCameraRight]       = TextIndicator(RT, 0, "control_help_camerarotateright" );
}


void ControlHelpSystem::Advance()
{
	if (g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD && g_prefsManager->GetInt(OTHER_CONTROLHELPENABLED, 1))
		return;

	// Clear all helpIcons
	//for (int i = 0; i < MaxIcons; i++)
	//	m_icons[i]->Clear();

	// Decay the usage on the conditions
	//for (int i = 0; i < MaxConditions; i++)
	//	DecayCond(i);

	// Check conditions	
	//for (int i = 0; i < MaxConditions; i++)
	//	if (CheckCondition(i))
	//		SetCondIcon(i);

	// On screen helps
	for( int i = 0; i < m_onScreenAids.Size(); i++ )
	{
		OnScreenAidInfo* info = m_onScreenAids.GetData(i);
		if( !info ) continue;

		if( (info->m_checker ? info->IsConditionMet() : g_inputManager->controlEvent(info->m_control)) && !info->m_isDone )
		{
			info->m_isDone = true;
			info->m_deadTime = GetHighResTime();
		}
		break;
	}

	for( int i = m_onScreenAids.Size(); i >= 0; i-- )
	{
		OnScreenAidInfo* info = m_onScreenAids.GetData(i);
		if( !info ) continue;
		if( info->m_isDone && info->m_deadTime + 1.0f < GetHighResTime() )
		{
			m_onScreenAids.RemoveData(i);
		}
	}
}

static inline TaskManager *GetTaskManager()
{
	Location *location = g_app->m_location;
	Team *team = location ? location->GetMyTeam() : NULL;
	TaskManager *taskManager = team ? team->m_taskManager : NULL;

	return taskManager;
}


static bool RunningProgram()
{
	TaskManager *taskManager = GetTaskManager();
	if (!taskManager)
		return false;

	Task *currentTask = taskManager->GetCurrentTask();
    // if the task has just been ended or killed, it isnt valid
	return currentTask && currentTask->m_state != Task::StateStopping;
}

static bool PlacingOfficerProgram()
{
	TaskManager *taskManager = GetTaskManager();
	if (!taskManager)
		return false;

	Task *currentTask = taskManager->GetCurrentTask();
	return currentTask && currentTask->m_state == Task::StateStarted &&
		    currentTask->m_type == GlobalResearch::TypeOfficer;
}

static Unit *GetSelectedUnit()
{
	Team *team = NULL;

	if (g_app->m_location && 
	    (team = g_app->m_location->GetMyTeam()))
		return team->GetMyUnit();
	else
		return NULL;
}

static bool SquaddieSelected()
{
	TaskManager *taskManager = GetTaskManager();
	if (!taskManager)
		return false;

	Unit *unit = NULL;
	Task *currentTask = NULL;

	return 
		g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) &&
		(unit = GetSelectedUnit()) && 
		unit->m_troopType == Entity::TypeInsertionSquadie &&
		(currentTask = taskManager->GetCurrentTask()) &&
		currentTask->m_state == Task::StateRunning;
}

static bool SquaddieFiring()
{
	return g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) && 
		GetSelectedUnit()->m_troopType == Entity::TypeInsertionSquadie &&
		g_inputManager->controlEvent( ControlUnitPrimaryFireTarget );
}

static bool WeaponSelected( int _type )
{
    InsertionSquad *squad = NULL;

	return 
		(squad = (InsertionSquad *) GetSelectedUnit()) &&
		squad->m_troopType == Entity::TypeInsertionSquadie &&
		_type == squad->m_weaponType &&
		g_app->m_globalWorld->m_research->HasResearch( _type );
}

static bool OfficerOrArmourSelected()
{
	Team *team = NULL;
	Entity *entity = NULL;

	return 
		g_app->m_location &&
		(team = g_app->m_location->GetMyTeam()) &&
		(entity = team->GetMyEntity()) &&
		(entity->m_type == Entity::TypeOfficer ||
		 entity->m_type == Entity::TypeArmour);
}

static int OfficerPreviousOrder()
{
	Team *team = NULL;
	Entity *entity = NULL;

	if(	g_app->m_location &&
		(team = g_app->m_location->GetMyTeam()) &&
		(entity = team->GetMyEntity()) &&
		(entity->m_type == Entity::TypeOfficer) )
	{
		Officer* o = (Officer*) entity;
		return o->GetPreviousMode();
	}
	return -1;
}

static int OfficerNextOrder()
{
	Team *team = NULL;
	Entity *entity = NULL;

	if(	g_app->m_location &&
		(team = g_app->m_location->GetMyTeam()) &&
		(entity = team->GetMyEntity()) &&
		(entity->m_type == Entity::TypeOfficer) )
	{
		Officer* o = (Officer*) entity;
		return o->GetNextMode();
	}
	return -1;
}

static int OfficerOrder()
{
	Team *team = NULL;
	Entity *entity = NULL;

	if(	g_app->m_location &&
		(team = g_app->m_location->GetMyTeam()) &&
		(entity = team->GetMyEntity()) &&
		(entity->m_type == Entity::TypeOfficer) )
	{
		Officer* o = (Officer*) entity;
		return o->m_orders;
	}
	return -1;
}

static int GetSelectedType()
{
	Team *team = NULL;
	Entity *entity = NULL;
	Unit *unit = NULL;
	int returnType = -1;

	if( g_app->m_location &&
		(team = g_app->m_location->GetMyTeam()) &&
		((entity = team->GetMyEntity()) || (unit = team->GetMyUnit())) )
	{
		returnType = entity ? entity->m_type : unit->m_troopType;
	}

	return returnType;
}

static bool ArmourSelected()
{
	Team *team = NULL;
	Entity *entity = NULL;

	return 
		g_app->m_location &&
		(team = g_app->m_location->GetMyTeam()) &&
		(entity = team->GetMyEntity()) &&
		(entity->m_type == Entity::TypeArmour);
}


static bool HasMoreThanOneSecondaryWeapon()
{
	int numSecondaryWeapons = 
		g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeGrenade ) +
		g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeAirStrike ) +
		g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeRocket );

	return numSecondaryWeapons > 1;
}

static bool UnitSelected()
{
	if( !g_app->m_location )
		return false;    

    Team *team = g_app->m_location->GetMyTeam();

	if( !team )
		return false;

    if( team->GetMyEntity() )
		return true;

	TaskManager *taskManager = GetTaskManager();
	if (!taskManager)
		return false;

    Task *currentTask = taskManager->GetCurrentTask();
    // if the task has just been ended or killed, it isnt valid
    if (currentTask && currentTask->m_state == Task::StateStopping )
        return false;

    if( !currentTask )
        return false;

	Unit *unit = team->GetMyUnit();
	return unit != NULL;
}

static bool CanSwitchUnit()
{
	TaskManager *taskManager = GetTaskManager();
	if (!taskManager)
		return false;

	return (taskManager->CapacityUsed() > 1);

	// The predicate below is true whenever the left button would
	// switch to a task (which includes when you have only one unit)
	// however, in order to try and keep screen clutter to a minimum
	// we've decided to simplify the predicate -- we should only
	// display the help if there is more than one task.

	//return (g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId == -1 &&
	//		g_app->m_location->GetMyTeam()->m_taskManager->CapacityUsed() > 0) ||
	//		g_app->m_location->GetMyTeam()->m_taskManager->CapacityUsed() > 1;
}

static bool BuildingSelected()
{
    Team *team = g_app->m_location->GetMyTeam();

    if( team && team->m_currentBuildingId != -1 )
    {
        Building *building = g_app->m_location->GetBuilding( team->m_currentBuildingId );
        if( building )
			return true;
    }

	return false;
}

static bool RadarDishSelected()
{
    Team *team = g_app->m_location->GetMyTeam();

    if( team && team->m_currentBuildingId != -1 )
    {
        Building *building = g_app->m_location->GetBuilding( team->m_currentBuildingId );
		if( building && building->m_type == Building::TypeRadarDish)
			return true;
    }

	return false;
}


bool ControlHelpSystem::CheckCondition( int _condition )
{
	return false;
	/*
	switch (_condition) {
		case CondDestroyUnit:
			return RunningProgram() || UnitSelected();

		case CondTaskManagerCreateBlue:
			// Always tell the user that the it is possible to create a unit
			// (even if full up) (so long as you're not in the task manager)
			return !g_app->m_taskManagerInterface->IsVisible()  ||
					g_app->m_taskManagerInterface->AdviseCreateControlHelpBlue();

		case CondTaskManagerCloseBlue:
			// Too cluttered to have two close buttons
			return false; // g_app->m_taskManagerInterface->AdviseCloseControlHelp();
		
		case CondTaskManagerCloseRed:
			return g_app->m_taskManagerInterface->IsVisible() ;

		case CondDeselectUnit:
			return UnitSelected() || BuildingSelected();

		case CondSelectUnit:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
				   g_app->m_gameCursor->AdviseHighlightingSomething();

		case CondTaskManagerCreateGreen:
			return g_app->m_taskManagerInterface->IsVisible()  && 
				   g_app->m_taskManagerInterface->AdviseCreateControlHelpGreen();

		case CondMoveUnit:
			return !g_app->m_taskManagerInterface->IsVisible()  &&
				   g_app->m_gameCursor->AdviseMoveableEntitySelected();

		case CondTaskManagerSelect:
			return g_app->m_taskManagerInterface->IsVisible()  && 
				   g_app->m_taskManagerInterface->AdviseOverSelectableZone();

		case CondPlaceUnit:
			return !g_app->m_taskManagerInterface->IsVisible()  &&
				   g_app->m_gameCursor->AdvisePlacementOpportunity();

		case CondPromoteOfficer:
			return !g_app->m_taskManagerInterface->IsVisible()  &&
				   g_app->m_gameCursor->AdviseHighlightingSomething() &&
				   PlacingOfficerProgram();		

		case CondMoveCameraOrUnit:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
				   (g_app->m_camera->IsInMode( Camera::ModeFreeMovement ) || 
				    g_app->m_camera->IsInMode( Camera::ModeEntityTrack ));

		case CondCameraAim:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
				   g_app->m_camera->IsInMode( Camera::ModeFreeMovement );

		case CondSquaddieFire:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
					SquaddieSelected();

		case CondChangeWeapon:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
					SquaddieSelected() &&
					HasMoreThanOneSecondaryWeapon();

		case CondChangeOrders:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
					OfficerOrArmourSelected();

		case CondCameraUp:
			return !m_icons[LA]->Enabled() && !m_icons[RA]->Enabled() && !g_app->m_taskManagerInterface->IsVisible() && !SquaddieSelected();

		case CondCameraDown:
			return !m_icons[LA]->Enabled() && !m_icons[RA]->Enabled() && !g_app->m_taskManagerInterface->IsVisible() && !SquaddieSelected();

		case CondZoom:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
				   // RadarDishSelected() &&
				   g_app->m_camera->IsInMode( Camera::ModeRadarAim );

		case CondFireGrenades:
			return !g_app->m_taskManagerInterface->IsVisible()  &&
				   g_inputManager->controlEvent( ControlUnitPrimaryFireDirected ) &&
				   SquaddieSelected() &&				   
				   WeaponSelected( GlobalResearch::TypeGrenade );
							   
		case CondFireRocket:
			return !g_app->m_taskManagerInterface->IsVisible()  &&
				   g_inputManager->controlEvent( ControlUnitPrimaryFireDirected ) &&
				   SquaddieSelected() &&				   
				   WeaponSelected( GlobalResearch::TypeRocket );

		case CondFireAirstrike:
			return !g_app->m_taskManagerInterface->IsVisible()  &&
				   g_inputManager->controlEvent( ControlUnitPrimaryFireDirected ) &&
				   SquaddieSelected() &&				   
				   WeaponSelected( GlobalResearch::TypeAirStrike );

		case CondOfficerSetGoto:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
					OfficerSelected();

		case CondArmourSetTurret:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
					ArmourSelected();

		case CondSwitchPrevUnit:
		case CondSwitchNextUnit:
			return !g_app->m_taskManagerInterface->IsVisible()  && 
					CanSwitchUnit();

        case CondRadarAim:
            return !g_app->m_taskManagerInterface->IsVisible()  &&
                RadarDishSelected();

		case CondRotateCameraLeft:
			// We can rotate the camera if we have a squad selected, and we're not firing
			//return SquaddieSelected() && SquaddieFiring();

		case CondRotateCameraRight:
			// We can rotate the camera if we have a squad selected, and we're not firing
			return SquaddieSelected() && !SquaddieFiring();

		default:
			AppDebugAssert( false );
			return false;
	}
	*/
}

void ControlHelpSystem::DecayCond( int _cond )
{
	TextIndicator &ti = m_conditionIconMap[_cond];
	ti.m_timeUsed -= ti.m_decayRate * g_advanceTime;
	if (ti.m_timeUsed < 0)
		ti.m_timeUsed = 0;
}

void ControlHelpSystem::RecordCondUsed( int _cond )
{
	TextIndicator &ti = m_conditionIconMap[_cond];
	ti.m_timeUsed += 2 * g_advanceTime;

	if (ti.m_maxUsage != 0.0)
	{
		if (ti.m_timeUsed > ti.m_maxUsage)
			ti.m_timeUsed = ti.m_maxUsage;
	}
}

void ControlHelpSystem::SetCondIcon( int _cond )
{
	const TextIndicator &ti = m_conditionIconMap[_cond];

	float alpha = 1.0;

	if (ti.m_minTime != 0.0 && ti.m_maxTime != 0.0) {
		if (ti.m_timeUsed < ti.m_minTime)
			alpha = 1.0;
		else 
			alpha = (ti.m_maxTime - ti.m_timeUsed ) / (ti.m_maxTime - ti.m_minTime);

		if (alpha < 0.0)
			return;
	}

	m_icons[ti.m_icon]->Set(ti.m_text, LANGUAGEPHRASE( ti.m_langPhrase ), alpha);
}

void ControlHelpSystem::Shutdown()
{
}

void ControlHelpSystem::Render()
{
	if( g_app->Multiplayer() )
	{
		return;
	}

	// Don't render if not in location
	if (g_app->m_locationId == -1 ||
		g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD || 
		!g_prefsManager->GetInt(OTHER_CONTROLHELPENABLED, 1))
	{
		return;
	}

	// Don't render if a cutscene is playing
	if( g_app->m_script && g_app->m_script->IsRunningScript() )
	{
		return;
	}

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
	// Don't render if we're in a cutscene
	if( g_app->m_sepulveda && g_app->m_sepulveda->IsInCutsceneMode() )
	{
		return;
	}
#endif

	if( EclGetWindow("dialog_locationmenu") )
	{
		return;
	}

	// Render the on screen help, even if the task manager is up
	g_gameFont.BeginText2D();
	RenderOnScreenAids();
	g_gameFont.EndText2D();

	// Don't render if task manager is up
	if( g_app->m_taskManagerInterface && g_app->m_taskManagerInterface->IsVisible() )
	{
		return;
	}

	glEnable    ( GL_BLEND );
	glDisable   ( GL_CULL_FACE );
	
	g_gameFont.BeginText2D();
	
	RenderHelp(GetSelectedType());
	
	g_gameFont.EndText2D();
	
	glEnable    ( GL_CULL_FACE );
	glDisable   ( GL_BLEND );

	/***** OLD METHOD *****
	else
	{
		g_gameFont.BeginText2D();
		
		Vector2 setPosition( g_app->m_renderer->ScreenW() - 200, 50 );

		if (g_app->m_taskManagerInterface->IsVisible() ) 
			setPosition.x -= 100;

		
		for (int i = 0; i < MaxSets; i++)
			m_sets[i]->Render( setPosition );
		
		g_gameFont.EndText2D();
	}
	*/
}

void ControlHelpSystem::RenderOnScreenAids()
{
	OnScreenAidInfo* info = NULL;
	info = m_onScreenAids.GetData(0);
	if( !info ) return;

	float yPos = g_app->m_renderer->ScreenH() / 3;
	float xPos = g_app->m_renderer->ScreenW() / 2;
	float fontLarge, fontSmall, fontSize;
	GetFontSizes(fontLarge, fontSize, fontSmall);

	float alpha = 0.8f;
	if( info->m_deadTime > 0.0f )
	{
		alpha -= max((GetHighResTime() - info->m_deadTime), 0.0);
	}
	float leftOffset = g_gameFont.GetTextWidthReal(info->m_text, fontSize) / 2;
	float iconSize = g_app->m_renderer->ScreenW()/40;

	xPos -= leftOffset;

	glDepthMask     ( false );
	
	glColor4f( alpha, alpha, alpha, 0.0f);
	g_gameFont.SetRenderOutline(true);
	g_gameFont.DrawText2D( xPos, yPos, fontSize, info->m_text );

	glColor4f( 1.0f, 1.0f, 1.0f, alpha );
	g_gameFont.SetRenderOutline(false);
	g_gameFont.DrawText2D( xPos, yPos, fontSize, info->m_text );

	char fullFilename[512];
	sprintf( fullFilename, "icons/%s.bmp", info->m_iconFileName );
	int iconTexId = g_app->m_resource->GetTexture( fullFilename );
	sprintf( fullFilename, "icons/%s.bmp", info->m_iconShadowFilename );
	int iconShadowTexId = g_app->m_resource->GetTexture( fullFilename );

	if( iconTexId != -1 && iconShadowTexId != -1 )
	{

		glEnable        ( GL_TEXTURE_2D );
		glColor4f		( alpha, alpha, alpha, 0.0f );
		glBindTexture   ( GL_TEXTURE_2D, iconShadowTexId );
		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );

		glBegin( GL_QUADS );
			glTexCoord2i(1,0);      glVertex2f( xPos,				yPos+iconSize );
			glTexCoord2i(1,1);      glVertex2f( xPos,				yPos-iconSize );
			glTexCoord2i(0,1);      glVertex2f( xPos-(iconSize*2),	yPos-iconSize );
			glTexCoord2i(0,0);      glVertex2f( xPos-(iconSize*2),	yPos+iconSize );
		glEnd();

		glColor4f		( 1.0f, 1.0f, 1.0f, alpha );
		glBindTexture   ( GL_TEXTURE_2D, iconTexId );
		glBlendFunc		( GL_SRC_ALPHA, GL_ONE );

		glBegin( GL_QUADS );
			glTexCoord2i(1,0);      glVertex2f( xPos,				yPos+iconSize );
			glTexCoord2i(1,1);      glVertex2f( xPos,				yPos-iconSize );
			glTexCoord2i(0,1);      glVertex2f( xPos-(iconSize*2),	yPos-iconSize );
			glTexCoord2i(0,0);      glVertex2f( xPos-(iconSize*2),	yPos+iconSize );
		glEnd();

		glDisable		( GL_TEXTURE_2D );
		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
}

void ControlHelpSystem::GetOrderNameAndBanner(int _order, char* _nameBuf, char* _bannerBuf)
{
	switch( _order )
	{
		case Officer::OrderNone:
			sprintf(_nameBuf, "multiwinia_control_noorder");
			sprintf(_bannerBuf, "banner_none");
			break;
		case Officer::OrderFollow:
			sprintf(_nameBuf, "multiwinia_control_follow");
			sprintf(_bannerBuf, "banner_follow");
			break;
		case Officer::OrderGoto:
			sprintf(_nameBuf, "multiwinia_control_order");
			sprintf(_bannerBuf, "banner_goto");
			break;
		case Officer::OrderPrepareGoto:
			sprintf(_nameBuf, "multiwinia_control_order");
			sprintf(_bannerBuf, "banner_goto");
			break;
		default:
			AppDebugOut("CONTROL HELP: Unknown officer orders: %d\n", OfficerOrder());
			sprintf(_nameBuf, "");
			sprintf(_bannerBuf, "");
			break;
	}
}

void ControlHelpSystem::RenderHelp(int _selectedType)
{
	if( !g_app->IsSinglePlayer() ) return;
	float xOffset = g_app->m_renderer->ScreenW() * -1.0f/6.0f;
	Armour* armour = NULL;
	char* name = NULL;
	Entity* e;

	RenderControlHelpDPadIcon(xOffset);

	switch (_selectedType)
	{
		case Entity::TypeOfficer :
			// D-Pad help
			RenderControlHelpDPad(1, "multiwinia_control_order", "banner_goto", 1.0f, 0.0f, xOffset);
			RenderControlHelpDPad(2, "multiwinia_control_follow", "banner_follow", 1.0f, 0.0f, xOffset);
			RenderControlHelpDPad(4, "multiwinia_control_noorder", "banner_none", 1.0f, 0.0f, xOffset);

			xOffset *= -1;

			// Normal help icons
			e = g_app->m_location->GetMyTeam() ? g_app->m_location->GetMyTeam()->GetMyEntity() : NULL;
			name = e && (g_app->m_userInput->GetMousePos3d()-e->m_pos).MagSquared() < 2500 ? "multiwinia_control_follow" : "multiwinia_control_order";
			RenderControlHelp(1, name, "mouse_lmb", "button_x", 1.0f, 0.0f, xOffset);
			RenderControlHelp(2, "gc_destroy", "mouse_lmb", "button_y", 1.0f, 0.0f, xOffset);
			RenderControlHelp(3, "gc_move", "mouse_lmb", "button_a", 1.0f, 0.0f, xOffset);
			RenderControlHelp(4, "gc_deselect", "mouse_lmb", "button_b", 1.0f, 0.0f, xOffset);
			break;
		case Entity::TypeInsertionSquadie : 
			// D-Pad help
			RenderControlHelpDPad(1, "gc_zoomin", NULL, 1.0f, 0.0f, xOffset);
			RenderControlHelpDPad(4, "gc_zoomout", NULL, 1.0f, 0.0f, xOffset);

			xOffset *= -1;

			// Normal help icons
			RenderControlHelp(1, "gc_changeweapon", "mouse_lmb", "button_x", 1.0f, 0.0f, xOffset);
			RenderControlHelp(2, "gc_destroy", "mouse_lmb", "button_y", 1.0f, 0.0f, xOffset);
			RenderControlHelp(4, "gc_deselect", "mouse_lmb", "button_b", 1.0f, 0.0f, xOffset);
			break;
		case Entity::TypeArmour : 
			armour = (Armour*)g_app->m_location->GetMyTeam()->GetMyEntity();
			// D-Pad help
			RenderControlHelpDPad(1, "gc_setturret", NULL, 1.0f, 0.0f, xOffset);

			xOffset *= -1;

			// Normal help icons
			armour->GetNumPassengers() > 0 ?
				RenderControlHelp(1, "multiwinia_control_unload", "mouse_lmb", "button_x", 1.0f, 0.0f, xOffset) :
				RenderControlHelp(1, "multiwinia_control_load", "mouse_lmb", "button_x", 1.0f, 0.0f, xOffset);

			RenderControlHelp(2, "gc_destroy", "mouse_lmb", "button_y", 1.0f, 0.0f, xOffset);
			RenderControlHelp(3, armour->m_awaitingDeployment ? "gc_setturret" : "gc_move", "mouse_lmb", "button_a", 1.0f, 0.0f, xOffset);
			RenderControlHelp(4, armour->m_awaitingDeployment ? "gc_cancel" : "gc_deselect", "mouse_lmb", "button_b", 1.0f, 0.0f, xOffset);
			break;
		default: // nothing is selected
			// D-Pad help
			RenderControlHelpDPad(1, "researchname_squad", NULL, 1.0f, 0.0f, xOffset);
			RenderControlHelpDPad(2, "researchname_engineer", NULL, 1.0f, 0.0f, xOffset);

			if( g_app->m_globalWorld && g_app->m_globalWorld->m_research &&
				g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeArmour] > 0 )
			{
				RenderControlHelpDPad(3, "researchname_armour", NULL, 1.0f, 0.0f, xOffset);
			}

			xOffset *= -1;

			// Normal help icons
			RenderControlHelp(1, "multiwinia_control_promote", "mouse_lmb", "button_x", 1.0f, 0.0f, xOffset);
			RenderControlHelp(3, "multiwinia_control_select", "mouse_lmb", "button_a", 1.0f, 0.0f, xOffset);
			break;
	}
}

bool ControlHelpSystem::HasSquadRunning()
{
	if( g_app->m_location &&
		g_app->m_location->GetMyTaskManager() )
	{
		Task* task = g_app->m_location->GetMyTaskManager()->GetCurrentTask();
		if( task && task->m_type == GlobalResearch::TypeSquad)
		{
			return true;
		}
	}

	return false;
}

void ControlHelpSystem::ClearOnScreenAids()
{
	m_onScreenAids.EmptyAndDelete();
}

void ControlHelpSystem::AddOnScreenAid( const char* _icon, const char* _iconShadow, UnicodeString _text, ControlType _control, CheckRunner *_checker )
{
	return;
	static UnicodeString prefix(" : ");
	m_onScreenAids.PutDataAtEnd(new OnScreenAidInfo(_icon, _iconShadow, prefix+_text, _control, _checker));
}

void ControlHelpSystem::RenderControlHelp( int position, char *caption, char *iconKb, char *iconXbox, float alpha, float highlight, float yOffset, bool tutorialHighlight )
{
    float overallScale = 1.0f;

    bool keyboardInput = ( g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD );
    char *icon = ( keyboardInput ? iconKb : iconXbox );


    // Position arrangement :       1       2
    //                              3       4

    if( alpha <= 0.0f ) return;
    if( alpha > 1.0f ) alpha = 1.0f;
    if( !caption && !iconKb && !iconXbox ) alpha = 0.3f;

    if( position < 1 || position > 4 ) return;

    // Note : first element is not used

    static float s_timers[]     = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
    static char *s_captions[]   = { NULL, NULL, NULL, NULL, NULL };
    static char *s_icons[]      = { NULL, NULL, NULL, NULL, NULL };


    //
    // Handle new requests for highlights
        
    if( highlight > 0.0f )
    {
        //if( s_timers[position] <= 0.0f )
        {
            s_captions[position] = caption;
            s_icons[position] = icon;
        }
        s_timers[position] = max( s_timers[position], highlight );
    }


    //
    // Advance previous highlights

    for( int i = 1; i <= 4; ++i )
    {
        if( s_timers[i] > 0.0f ) 
        {
            if( position == i )
            {
                caption = s_captions[position];
                icon = s_icons[position];
            }
            s_timers[i] -= g_advanceTime * 0.5f;
        }
    }


    float currentTimer = ( s_timers[position] );
    if( currentTimer > 0.0f ) currentTimer = 1.0f;
    if( alpha < currentTimer ) alpha = currentTimer;
    

    //
    // Where will our box be on screen?

    int leftOrRight = ( position == 1 || position == 3 ? -1 : +1 );
    int upOrDown = ( position == 3 || position == 4 ? +1 : 0 );

    float w = g_app->m_renderer->ScreenH() * 0.15f * overallScale;
    float h = w * 0.3f;
    float screenY = g_app->m_renderer->ScreenH() * 0.75f;    
    float screenX = g_app->m_renderer->ScreenW() * 0.5f;    
    screenX += leftOrRight * w * 0.6f;
    screenX -= w/2.0f;
    screenY += upOrDown * h * 1.2f;
	screenX += yOffset;


    // 
    // Background box + outline
 
    if( icon || caption )
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glColor4f( 0.0f, 0.0f, 0.0f, alpha * 0.75f );
        glBegin( GL_QUADS );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();

        glColor4f( 1.0f, 1.0f, 1.0f, alpha * 0.2f );
        glBegin( GL_LINE_LOOP );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();
    }


    //
    // Highlight

    if( ( icon || caption ) && currentTimer > 0.0f )
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;
        glColor4ub( col.r * 0.7f, col.g * 0.7f, col.b * 0.7f, alpha * 255 );   

        glBegin( GL_QUADS );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin( GL_LINE_LOOP );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();

    }
   

    //
    // Tutorial Highlight

    if( tutorialHighlight )
    {
        RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;
        glColor4ub( col.r, col.g, col.b, alpha * 255 );   

        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        
        glBegin( GL_QUADS );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();
        
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin( GL_LINE_LOOP );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();
    }


    //
    // Icon

    if( icon )
    {
        char fullFilename[512];
        sprintf( fullFilename, "icons/%s.bmp", icon );
        int iconTexId = g_app->m_resource->GetTexture( fullFilename );
        const BitmapRGBA *bitmap = g_app->m_resource->GetBitmap( fullFilename );

        float iconSize = w * 0.25f;
        float iconW = bitmap->m_width/64.0f * iconSize;
        float iconH = bitmap->m_height/64.0f * iconSize;
        float iconCentreX = screenX + w * 0.5f;
        float iconCentreY = screenY + iconSize * 1.1f - iconH * 0.5f;
                
        iconCentreX += leftOrRight * w * -0.5f;
        iconCentreX += leftOrRight * iconW * 0.5f;
        iconCentreX += leftOrRight * w * 0.05f;

        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        
        glBindTexture   ( GL_TEXTURE_2D, iconTexId );
        glEnable        ( GL_TEXTURE_2D );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( iconCentreX-iconW/2.0f, iconCentreY-iconH/2.0f );
            glTexCoord2i(1,1);      glVertex2f( iconCentreX+iconW/2.0f, iconCentreY-iconH/2.0f );
            glTexCoord2i(1,0);      glVertex2f( iconCentreX+iconW/2.0f, iconCentreY+iconH/2.0f );
            glTexCoord2i(0,0);      glVertex2f( iconCentreX-iconW/2.0f, iconCentreY+iconH/2.0f  );
        glEnd();

        glDisable( GL_TEXTURE_2D );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );    
    }


    //
    // Caption

    if( caption )
    {
        UnicodeString translatedCaption = LANGUAGEPHRASE( caption );
		translatedCaption.StrUpr();

        float fontX = screenX + w / 2.0f;
        float fontY = screenY + h * 0.5f;        
        float fontSize = h * 0.4f;

        if( upOrDown > 0 && leftOrRight > 0 ) fontX += leftOrRight * w * 0.1f;
        fontX += leftOrRight * w * 0.1f;

        glColor4f( alpha, alpha, alpha, 0.0f);
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( fontX, fontY, fontSize, translatedCaption );

        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( fontX, fontY, fontSize, translatedCaption );
    }
}

void ControlHelpSystem::RenderControlHelpDPadIcon(float yOffset, char* _label)
{
	float iconSize = 50.0f;
	float w = g_app->m_renderer->ScreenH() * 0.15f;
    float h = w * 0.3f - (iconSize/2);
    float screenY = g_app->m_renderer->ScreenH() * 0.75f;    
    float screenX = g_app->m_renderer->ScreenW() * 0.5f;
	screenX += yOffset;
	screenY += iconSize/4;
	
    int iconTexId = g_app->m_resource->GetTexture( "icons/button_dpad.bmp" );
    int iconShadowTexId = g_app->m_resource->GetTexture( "icons/button_control_shadow.bmp" );


	if( iconTexId == -1 || iconShadowTexId == -1 )
		return;

	// Shadow
	glBindTexture   ( GL_TEXTURE_2D, iconShadowTexId );
    glEnable        ( GL_TEXTURE_2D );
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
	glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );

	glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex2f( screenX-iconSize/2, screenY-iconSize/2 );
        glTexCoord2i(0,1);      glVertex2f( screenX-iconSize/2, screenY+iconSize/2 );
        glTexCoord2i(1,1);      glVertex2f( screenX+iconSize/2, screenY+iconSize/2 );
        glTexCoord2i(1,0);      glVertex2f( screenX+iconSize/2, screenY-iconSize/2 );
    glEnd();

	glBindTexture   ( GL_TEXTURE_2D, iconTexId );
    
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
	glColor4f(1.0f,1.0f,1.0f,1.0f);

    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex2f( screenX-iconSize/2, screenY-iconSize/2 );
        glTexCoord2i(0,1);      glVertex2f( screenX-iconSize/2, screenY+iconSize/2 );
        glTexCoord2i(1,1);      glVertex2f( screenX+iconSize/2, screenY+iconSize/2 );
        glTexCoord2i(1,0);      glVertex2f( screenX+iconSize/2, screenY-iconSize/2 );
    glEnd();

	if( _label )
	{      
        float fontSize = h * 0.4f;
		glColor4f( 1.0f, 1.0f, 1.0f, 0.0f);
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( screenX, screenY, fontSize, LANGUAGEPHRASE(_label) );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( screenX, screenY, fontSize, LANGUAGEPHRASE(_label) );
	}

    glDisable( GL_TEXTURE_2D );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

void ControlHelpSystem::RenderControlHelpDPad( int position, char *caption, char *flagIcon, float alpha, float highlight, float yOffset, bool tutorialHighlight )
{
    float overallScale = 1.0f;

    bool keyboardInput = ( g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD );
    char *icon = flagIcon;


    // Position arrangement :       1       
    //							2	+	3
	//								4

    if( alpha <= 0.0f ) return;
    if( alpha > 1.0f ) alpha = 1.0f;
    if( !caption && !icon ) alpha = 0.3f;

    if( position < 1 || position > 4 ) return;

    // Note : first element is not used

    static float s_timers[]     = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
    static char *s_captions[]   = { NULL, NULL, NULL, NULL, NULL };
    static char *s_icons[]      = { NULL, NULL, NULL, NULL, NULL };


    //
    // Handle new requests for highlights
        
    if( highlight > 0.0f )
    {
        //if( s_timers[position] <= 0.0f )
        {
            s_captions[position] = caption;
            s_icons[position] = icon;
        }
        s_timers[position] = max( s_timers[position], highlight );
    }


    //
    // Advance previous highlights

    for( int i = 1; i <= 4; ++i )
    {
        if( s_timers[i] > 0.0f ) 
        {
            if( position == i )
            {
                caption = s_captions[position];
                icon = s_icons[position];
            }
            s_timers[i] -= g_advanceTime * 0.5f;
        }
    }


    float currentTimer = ( s_timers[position] );
    if( currentTimer > 0.0f ) currentTimer = 1.0f;
    if( alpha < currentTimer ) alpha = currentTimer;
    

    //
    // Where will our flags be on screen?

	float leftOrRight = position == 2 ? -1.5f : position == 3 ? 1.5f : 0;
	float upOrDown = position == 1 ? -1.5f : position == 4 ? 1.5f : 0;

    float w = g_app->m_renderer->ScreenH() * 0.15f * overallScale;
    float h = w * 0.3f;
    float screenY = g_app->m_renderer->ScreenH() * 0.75f;    
    float screenX = g_app->m_renderer->ScreenW() * 0.5f;    
    screenX += leftOrRight * w * 0.6f;
    screenX -= w/2.0f;
    screenY += upOrDown * h * 1.2f;
	screenX += yOffset;

	// 
    // Background box + outline
 
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glColor4f( 0.0f, 0.0f, 0.0f, alpha * 0.75f );
    glBegin( GL_QUADS );
        glVertex2f( screenX, screenY );
        glVertex2f( screenX + w, screenY );
        glVertex2f( screenX + w, screenY + h );
        glVertex2f( screenX, screenY + h );
    glEnd();

    glColor4f( 1.0f, 1.0f, 1.0f, alpha * 0.2f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( screenX, screenY );
        glVertex2f( screenX + w, screenY );
        glVertex2f( screenX + w, screenY + h );
        glVertex2f( screenX, screenY + h );
    glEnd();
	
    //
    // Icon

	float iconCentreX = 0.0f;
    float iconCentreY = 0.0f;
	float iconSize = w * 0.25f;
    float iconW = 0.0f;
    float iconH = 0.0f;

    if( icon )
    {
        char fullFilename[512];
        sprintf( fullFilename, "icons/%s.bmp", icon );
        int iconTexId = g_app->m_resource->GetTexture( fullFilename );
        const BitmapRGBA *bitmap = g_app->m_resource->GetBitmap( fullFilename );

		iconW = bitmap->m_width/64.0f * iconSize;
		iconH = bitmap->m_height/64.0f * iconSize;
		iconCentreY = screenY + iconSize * 1.1f - iconH * 0.5f;
		
		if( position == 3 )
		{
			iconCentreX = screenX + w - (iconSize*0.5f);
		}
		else
		{
			iconCentreX = screenX + (iconSize*0.5f);
		}

		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
		glColor4f( 0.5f, 0.5f, 0.5f, 0.0f );

		glBegin( GL_QUADS );
			glVertex2f( iconCentreX-iconW/2.0f, iconCentreY-iconH/2.0f );
            glVertex2f( iconCentreX+iconW/2.0f, iconCentreY-iconH/2.0f );
            glVertex2f( iconCentreX+iconW/2.0f, iconCentreY+iconH/2.0f );
            glVertex2f( iconCentreX-iconW/2.0f, iconCentreY+iconH/2.0f  );
		glEnd();

		glEnable        ( GL_TEXTURE_2D );
		glColor4f( 1.0f, 1.0f, 1.0f, alpha );
		glBindTexture   ( GL_TEXTURE_2D, iconTexId );
		glBlendFunc		( GL_SRC_ALPHA, GL_ONE );

        glBegin( GL_QUADS );
            glTexCoord2i(1,0);      glVertex2f( iconCentreX-iconW/2.0f, iconCentreY-iconH/2.0f );
            glTexCoord2i(0,0);      glVertex2f( iconCentreX+iconW/2.0f, iconCentreY-iconH/2.0f );
            glTexCoord2i(0,1);      glVertex2f( iconCentreX+iconW/2.0f, iconCentreY+iconH/2.0f );
            glTexCoord2i(1,1);      glVertex2f( iconCentreX-iconW/2.0f, iconCentreY+iconH/2.0f  );
        glEnd();

        glDisable( GL_TEXTURE_2D );
		glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    }


    //
    // Caption

    if( caption )
    {
		iconCentreX = screenX + w * 0.5f;
        iconCentreY = screenY + iconSize * 1.1f - iconH * 0.5f;
                
		iconCentreX += (leftOrRight == 0 ? 1 : leftOrRight) * w * -0.5f;
		if( icon )
		{
			iconCentreX += (leftOrRight == 0 ? 1 : leftOrRight) * iconW * 0.5f;
		}

        iconCentreX += (leftOrRight == 0 ? 1 : leftOrRight) * w * 0.05f;
		if( position == 1 || position == 4 )
		{
			iconCentreX += w * 1.5f * 0.5f;
		}
        UnicodeString translatedCaption = LANGUAGEPHRASE( caption );
		translatedCaption.StrUpr();

		float fontX = iconCentreX + (leftOrRight * w/5.0f);//screenX + w / 2.0f;
        float fontY = screenY + h * 0.5f;        
        float fontSize = h * 0.4f;

        if( upOrDown > 0 && leftOrRight > 0 ) fontX += (leftOrRight == 0 ? -1 : leftOrRight) * w * 0.1f;
        fontX += (leftOrRight > 0 ? -1 : leftOrRight) * w * 0.1f;

        glColor4f( alpha, alpha, alpha, 0.0f);
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( fontX, fontY, fontSize, translatedCaption );

        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( fontX, fontY, fontSize, translatedCaption );
    }
}






