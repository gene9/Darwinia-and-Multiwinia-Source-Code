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
#include "lib/binary_stream_readers.h"
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
#include "location_input.h"

#include "interface/prefs_other_window.h"

#include "worldobject/radardish.h"
#include "worldobject/insertion_squad.h"

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
	void Set( int index, const char *_helpText, float _alpha );

    void Render( const Vector2 &_setPosition, float _alpha );

private:
    char m_filename[256], m_shadowFilename[256];
    Vector2 m_setRelativePos;

	struct PosText {		
		Vector2 m_pos;
		const char * m_text;
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
	t.m_text = NULL;
	t.m_alpha = 1.0f;

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

	_iconAlpha = 0.3f;	

	for (int i = 0; i < m_texts.size(); i++) {
		if (m_texts[i].m_text) {
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
		m_texts[i].m_text = NULL;
}

void HelpIcon::Set( int _index, const char *_helpText, float _alpha )
{
	DarwiniaDebugAssert( _helpText != NULL );

	m_texts[_index].m_text = _helpText;
	m_texts[_index].m_alpha = _alpha;
	
	if (m_texts[_index].m_text != m_lastTexts[_index].m_text) 
		m_lastHelpTextChangeTime = g_gameTime;
}


void HelpIcon::Render( const Vector2 &_setPosition, float _alpha )
{
    float iconSize = 50.0f;
    float iconGap = 10.0f;

	float iconAlpha;
    float shadowOffset = 0;
    float shadowSize = iconSize; 

	const bool enabled = Enabled( iconAlpha );

	if (enabled) {
		// Check to see whether we should blink
		double elapsedTime = g_gameTime - m_lastHelpTextChangeTime;
		if (elapsedTime < 1.5) {
			iconAlpha = 0.75 * iconAlpha + 0.25 * iconAlpha * sinf( 8 * elapsedTime );
		}
	}

	iconAlpha *= _alpha;

	Vector2 position = _setPosition + m_setRelativePos;

	Vector2 iconCentre = Vector2(position.x, position.y);

	//if( g_app->m_largeMenus ) 
	//{
	//	iconGap *= 2.0f;
	//	iconSize *= 1.5f;
	//	shadowSize *= 1.5f;
	//	iconCentre.x -= (g_app->m_renderer->ScreenW() - iconCentre.x ) * 0.5;
	//	iconCentre.y += iconGap;
	//}

    float shadowX = shadowSize/2;

	// Render the shadow
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( m_shadowFilename ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glDepthMask     ( false );
    glColor4f       ( iconAlpha, iconAlpha, iconAlpha, 0.0f );         
	
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
    
    glColor4f   ( 1.0f, 1.0f, 1.0f, iconAlpha );

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
			const char *text = m_texts[i].m_text;
			const Vector2 &pos = m_texts[i].m_pos;

			if (text == NULL)
				continue;

			float fontSize = 12.0f;
			//if( g_app->m_largeMenus )
			//{
			//	fontSize *= 1.5f;
			//}
			
			Vector2 textCentrePos( 
				iconCentre.x + (shadowSize / 2 + iconGap) * pos.x,
				iconCentre.y + (shadowSize / 2 + iconGap) * pos.y);

			g_gameFont.SetRenderOutline(true);
			glColor4f(1.0f,1.0f,1.0f,0.0f);
			g_gameFont.DrawText2DJustified( textCentrePos.x, textCentrePos.y, fontSize, pos.x, "%s", text );

			g_gameFont.SetRenderOutline(false);
			glColor4f(1.0f,1.0f,1.0f,iconAlpha);
			g_gameFont.DrawText2DJustified( textCentrePos.x, textCentrePos.y, fontSize, pos.x, "%s", text );
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
	const float fadeDuration = 1.0f;

	if (m_beginFadeOutTime > 0.0f) {
		alpha = 1.0 - (g_gameTime - m_beginFadeOutTime) / fadeDuration;
		if (alpha < 0.0) {
			m_beginFadeOutTime = 0.0f;
			alpha = 0.0f;
		}
	}
	else if (m_beginFadeInTime > 0.0f) {
		alpha = (g_gameTime - m_beginFadeInTime) / fadeDuration;
		if (alpha > 1.0f) {
			m_beginFadeInTime = 0.0f;
			alpha = 1.0f;
		}
	}
	else {
		alpha = atLeastOneEnabled ? 1.0f : 0.0f;
	}

	// If none enabled, we don't render the entire set
	if (alpha > 0.0f) {
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
	InitialiseIcons();
	InitialiseConditions();
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


	DarwiniaDebugAssert( set < MaxSets );
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
	m_conditionIconMap[CondMoveCameraOrUnit]		= TextIndicator(LA, 0, "gc_move", 2.0f, 3.0f, 9.0f, 0.3f );
	m_conditionIconMap[CondCameraAim]				= TextIndicator(RA, 0, "gc_aim", 2.0f, 3.0f, 9.0f, 0.3f );
	m_conditionIconMap[CondSquaddieFire]			= TextIndicator(RA, 0, "gc_fire", 2.0f, 3.0f, 9.0f );
	m_conditionIconMap[CondChangeWeapon]			= TextIndicator(DPAD, 0, "gc_changeweapon", 1.0f, 2.0f, 8.0f, 0.3f  );
	m_conditionIconMap[CondChangeOrders]			= TextIndicator(DPAD, 0, "gc_changeorders" );
	m_conditionIconMap[CondCameraUp]				= TextIndicator(DPAD, 1, "gc_cameraup", 1.0f, 2.0f, 3.0f, 0.3f );
	m_conditionIconMap[CondCameraDown]				= TextIndicator(DPAD, 2, "gc_cameradown", 1.0f, 2.0f, 3.0f, 0.3f   );
	m_conditionIconMap[CondZoom]					= TextIndicator(LT, 0, "gc_zoom" );
	m_conditionIconMap[CondFireGrenades]			= TextIndicator(RT, 0, "gc_grenade", 1.0f, 2.0f, 8.0f, 0.3f );
	m_conditionIconMap[CondFireRocket]				= TextIndicator(RT, 0, "gc_rocket", 1.0f, 2.0f, 8.0f, 0.3f  );
	m_conditionIconMap[CondFireAirstrike]			= TextIndicator(RT, 0, "gc_airstrike", 1.0f, 2.0f, 8.0f, 0.3f  );
	m_conditionIconMap[CondOfficerSetGoto]			= TextIndicator(RT, 0, "gc_setgoto" );
	m_conditionIconMap[CondArmourSetTurret]			= TextIndicator(RT, 0, "gc_setturret" );
	m_conditionIconMap[CondSwitchPrevUnit]			= TextIndicator(LB, 0, "gc_prev" );
	m_conditionIconMap[CondSwitchNextUnit]			= TextIndicator(RB, 0, "gc_next" );
    m_conditionIconMap[CondRadarAim]                = TextIndicator(A, 0, "control_help_radar" );
	m_conditionIconMap[CondSkipCutscene]			= TextIndicator(B, 0, "gc_skip" );
	m_conditionIconMap[CondOfficerSetFollow]        = TextIndicator(RT, 0, "gc_setfollow");
}


void ControlHelpSystem::Advance()
{
	if (g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD && g_prefsManager->GetInt(OTHER_CONTROLHELPENABLED, 1))
		return;

	// Clear all helpIcons
	for (int i = 0; i < MaxIcons; i++)
		m_icons[i]->Clear();

	// Decay the usage on the conditions
	for (int i = 0; i < MaxConditions; i++)
		DecayCond(i);

	// Check conditions	
	for (int i = 0; i < MaxConditions; i++)
		if (CheckCondition(i))
			SetCondIcon(i);
}

static bool RunningProgram()
{
	Task *currentTask = g_app->m_taskManager->GetCurrentTask();
    // if the task has just been ended or killed, it isnt valid
	return currentTask && currentTask->m_state != Task::StateStopping;
}

static bool PlacingOfficerProgram()
{
	Task *currentTask = g_app->m_taskManager->GetCurrentTask();
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
	Unit *unit = NULL;
	Task *currentTask = NULL;

	return 
		g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) &&
		(unit = GetSelectedUnit()) && 
		unit->m_troopType == Entity::TypeInsertionSquadie &&
		(currentTask = g_app->m_taskManager->GetCurrentTask()) &&
		currentTask->m_state == Task::StateRunning;
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

static bool OfficerSelected()
{
	Team *team = NULL;
	Entity *entity = NULL;

	return 
		g_app->m_location &&
		(team = g_app->m_location->GetMyTeam()) &&
		(entity = team->GetMyEntity()) &&
		(entity->m_type == Entity::TypeOfficer);
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

    Task *currentTask = g_app->m_taskManager->GetCurrentTask();
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
	return (g_app->m_taskManager->CapacityUsed() > 1);

	// The predicate below is true whenever the left button would
	// switch to a task (which includes when you have only one unit)
	// however, in order to try and keep screen clutter to a minimum
	// we've decided to simplify the predicate -- we should only
	// display the help if there is more than one task.

	//return (g_app->m_taskManager->m_currentTaskId == -1 &&
	//		g_app->m_taskManager->CapacityUsed() > 0) ||
	//		g_app->m_taskManager->CapacityUsed() > 1;
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
	switch (_condition) {
		case CondDestroyUnit:
			return RunningProgram() || UnitSelected();

		case CondTaskManagerCreateBlue:
			// Always tell the user that the it is possible to create a unit
			// (even if full up) (so long as you're not in the task manager)
			return !g_app->m_taskManagerInterface->m_visible ||
					g_app->m_taskManagerInterface->AdviseCreateControlHelpBlue();

		case CondTaskManagerCloseBlue:
			// Too cluttered to have two close buttons
			return false; // g_app->m_taskManagerInterface->AdviseCloseControlHelp();
		
		case CondTaskManagerCloseRed:
			return g_app->m_taskManagerInterface->m_visible;

		case CondDeselectUnit:
			return UnitSelected() || BuildingSelected();

		case CondSelectUnit:
			return !g_app->m_taskManagerInterface->m_visible && 
				   g_app->m_gameCursor->AdviseHighlightingSomething();

		case CondTaskManagerCreateGreen:
			return g_app->m_taskManagerInterface->m_visible && 
				   g_app->m_taskManagerInterface->AdviseCreateControlHelpGreen();

		case CondMoveUnit:
			return !g_app->m_taskManagerInterface->m_visible &&
				   g_app->m_gameCursor->AdviseMoveableEntitySelected();

		case CondTaskManagerSelect:
			return g_app->m_taskManagerInterface->m_visible && 
				   g_app->m_taskManagerInterface->AdviseOverSelectableZone();

		case CondPlaceUnit:
			return !g_app->m_taskManagerInterface->m_visible &&
				   g_app->m_gameCursor->AdvisePlacementOpportunity();

		case CondPromoteOfficer:
			return !g_app->m_taskManagerInterface->m_visible &&
				   g_app->m_gameCursor->AdviseHighlightingSomething() &&
				   PlacingOfficerProgram();		

		case CondMoveCameraOrUnit:
			return !g_app->m_taskManagerInterface->m_visible && 
				   (g_app->m_camera->IsInMode( Camera::ModeFreeMovement ) || 
				    g_app->m_camera->IsInMode( Camera::ModeEntityTrack ));

		case CondCameraAim:
			return !g_app->m_taskManagerInterface->m_visible && 
				   g_app->m_camera->IsInMode( Camera::ModeFreeMovement );

		case CondSquaddieFire:
			return !g_app->m_taskManagerInterface->m_visible && 
					SquaddieSelected();

		case CondChangeWeapon:
			return !g_app->m_taskManagerInterface->m_visible && 
					SquaddieSelected() &&
					HasMoreThanOneSecondaryWeapon();

		case CondChangeOrders:
			return !g_app->m_taskManagerInterface->m_visible && 
					OfficerOrArmourSelected();

		case CondCameraUp:
			return !m_icons[LA]->Enabled() && !m_icons[RA]->Enabled() && !g_app->m_taskManagerInterface->m_visible;

		case CondCameraDown:
			return !m_icons[LA]->Enabled() && !m_icons[RA]->Enabled() && !g_app->m_taskManagerInterface->m_visible;

		case CondZoom:
			return !g_app->m_taskManagerInterface->m_visible && 
				   // RadarDishSelected() &&
				   g_app->m_camera->IsInMode( Camera::ModeRadarAim );

		case CondFireGrenades:
			return !g_app->m_taskManagerInterface->m_visible &&
				   g_inputManager->controlEvent( ControlUnitPrimaryFireDirected ) &&
				   SquaddieSelected() &&				   
				   WeaponSelected( GlobalResearch::TypeGrenade );
							   
		case CondFireRocket:
			return !g_app->m_taskManagerInterface->m_visible &&
				   g_inputManager->controlEvent( ControlUnitPrimaryFireDirected ) &&
				   SquaddieSelected() &&				   
				   WeaponSelected( GlobalResearch::TypeRocket );

		case CondFireAirstrike:
			return !g_app->m_taskManagerInterface->m_visible &&
				   g_inputManager->controlEvent( ControlUnitPrimaryFireDirected ) &&
				   SquaddieSelected() &&				   
				   WeaponSelected( GlobalResearch::TypeAirStrike );

		case CondOfficerSetGoto:
			return !g_app->m_taskManagerInterface->m_visible && 
					OfficerSelected();

		case CondOfficerSetFollow:
		{
			WorldObjectId idUnderMouse;
			bool officerHighlighted = false;
            if( g_app->m_locationInput->GetObjectUnderMouse( idUnderMouse, g_app->m_globalWorld->m_myTeamId ) )
			{
				Entity *e = g_app->m_location->GetEntity( idUnderMouse );
				if( e && e->m_type == Entity::TypeOfficer ) officerHighlighted = true;
			}
			return !g_app->m_taskManagerInterface->m_visible && 
					OfficerSelected() &&
					officerHighlighted;
		}

		case CondArmourSetTurret:
			return !g_app->m_taskManagerInterface->m_visible && 
					ArmourSelected();

		case CondSwitchPrevUnit:
		case CondSwitchNextUnit:
			return !g_app->m_taskManagerInterface->m_visible && 
					CanSwitchUnit();

        case CondRadarAim:
            return !g_app->m_taskManagerInterface->m_visible &&
                RadarDishSelected();

		case CondSkipCutscene:
		{
			bool inCutscene = false;
			if( g_app->m_script->IsRunningScript() &&
				g_app->m_script->m_permitEscape ) inCutscene = true;
			if( g_app->m_camera->IsInMode( Camera::ModeBuildingFocus ) ) inCutscene = true;
			return inCutscene;
		}


		default:
			DarwiniaDebugAssert( false );
			return false;
	}
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

	if (ti.m_maxUsage != 0.0f)
	{
		if (ti.m_timeUsed > ti.m_maxUsage)
			ti.m_timeUsed = ti.m_maxUsage;
	}
}

void ControlHelpSystem::SetCondIcon( int _cond )
{
	const TextIndicator &ti = m_conditionIconMap[_cond];

	float alpha = 1.0f;

	if (ti.m_minTime != 0.0f && ti.m_maxTime != 0.0f) {
		if (ti.m_timeUsed < ti.m_minTime)
			alpha = 1.0f;
		else 
			alpha = (ti.m_maxTime - ti.m_timeUsed ) / (ti.m_maxTime - ti.m_minTime);

		if (alpha < 0.0f)
			return;
	}

	m_icons[ti.m_icon]->Set(ti.m_text, LANGUAGEPHRASE( ti.m_langPhrase ), alpha);
}

void ControlHelpSystem::Shutdown()
{
}

void ControlHelpSystem::Render()
{
	// Don't render if not in location
	if (g_app->m_locationId == -1 ||
		g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD || 
		!g_prefsManager->GetInt(OTHER_CONTROLHELPENABLED, 1))
	{
		return;
	}

	Vector2 setPosition( g_app->m_renderer->ScreenW() - 200, 50 );
	g_gameFont.BeginText2D();

	bool inCutscene = false;
	if( g_app->m_script->IsRunningScript() &&
		g_app->m_script->m_permitEscape ) inCutscene = true;
	if( g_app->m_camera->IsInMode( Camera::ModeBuildingFocus ) ) inCutscene = true;

	if( inCutscene )
	{
		float alpha;
		m_icons[B]->Enabled( alpha );
		m_icons[B]->Render( setPosition, alpha );
        g_gameFont.EndText2D();
		return;
	}

	if (g_app->m_taskManagerInterface->m_visible) 
		setPosition.x -= 100;

	for (int i = 0; i < MaxSets; i++)
		m_sets[i]->Render( setPosition );

    g_gameFont.EndText2D();
}

