
#ifndef _included_controlhelp_h
#define _included_controlhelp_h

#include "lib/llist.h"
#include "lib/darray.h"
#include "lib/bounded_array.h"
#include "worldobject/worldobject.h"

class HelpIcon;
class HelpIconSet;

class ControlHelpSystem
{
public:
    ControlHelpSystem();

    void Advance();
    void Render();

	void Shutdown();

	enum {
		A, B, X, Y, LA, RA, LB, RB, LT, RT, DPAD, MaxIcons
	};

	enum {
		MaxSets = 4
	};

	enum {
		CondDestroyUnit,
		CondTaskManagerCreateBlue,
		CondTaskManagerCloseBlue,
		CondTaskManagerCloseRed,
		CondDeselectUnit,
		CondMoveUnit,
		CondSelectUnit,
		CondTaskManagerCreateGreen,
		CondTaskManagerSelect,
		CondPlaceUnit,
		CondPromoteOfficer,
		CondMoveCameraOrUnit,
		CondZoom,
		CondCameraAim,
		CondSquaddieFire,
		CondChangeWeapon,
		CondChangeOrders,
		CondCameraUp,
		CondCameraDown,
		CondFireGrenades,
		CondFireRocket,
		CondFireAirstrike,
		CondOfficerSetGoto,
		CondOfficerSetFollow,
		CondArmourSetTurret,
		CondSwitchPrevUnit,
		CondSwitchNextUnit,
        CondRadarAim,
		CondSkipCutscene,
		MaxConditions
	};

	void RecordCondUsed( int _cond );

private:
	void InitialiseIcons();
	void InitialiseConditions();
	bool CheckCondition( int _cond );
	void SetCondIcon( int _cond );
	void DecayCond( int _cond );

private:

	HelpIcon *m_icons[MaxIcons];
	HelpIconSet *m_sets[MaxSets];

	typedef struct TextIndicator {
		TextIndicator() 
			: m_icon( 0 ),
			  m_text( 0 ),
			  m_minTime( 0.0f ),
			  m_maxTime( 0.0f ),
			  m_maxUsage( 0.0f ),
			  m_decayRate( 1.0f )
		{
		}

		TextIndicator( int _icon, int _text, const char *_langPhrase, float _minTime = 0.0f, float _maxTime = 0.0f, float _maxUsage = 0.0f, float _decayRate = 1.0f )
			:	m_icon( _icon ), 
				m_text( _text ),
				m_langPhrase( _langPhrase ),
				m_minTime( _minTime ),
				m_maxTime( _maxTime ),
				m_maxUsage( _maxUsage ),
				m_decayRate( _decayRate )
		{
		}

		int m_icon, m_text;
		const char *m_langPhrase;

		float m_timeUsed;
		float m_minTime, m_maxTime, m_maxUsage, m_decayRate;

	};

	TextIndicator m_conditionIconMap[MaxConditions];
};

#endif
