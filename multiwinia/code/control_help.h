
#ifndef _included_controlhelp_h
#define _included_controlhelp_h

#include "lib/tosser/llist.h"
#include "lib/tosser/darray.h"
#include "lib/tosser/bounded_array.h"
#include "worldobject/worldobject.h"
#include "lib/unicode/unicode_string.h"

#define NUM_ON_SCREEN_AIDS 16

class HelpIcon;
class HelpIconSet;

class CheckRunner
{
public:
	virtual ~CheckRunner() {}	
	virtual bool Check() const = 0;
};


template <class T> 
struct BoolChecker : CheckRunner
{
	BoolChecker( bool (T::*_func)(), T *_this )
	:	m_func( _func ),
		m_this( _this )
	{
	}

	bool Check() const
	{
		return (m_this->*m_func)();
	}

private:

	bool (T::*m_func)();
	T * m_this;
};

class ControlHelpSystem
{
private:
	void	GetOrderNameAndBanner		( int _order, char* _nameBuf, char* _bannerBuf );
	void	RenderHelp					( int _selectedType );
	void	RenderControlHelpDPadIcon	( float xOffset = 0.0f, char* _label=NULL );

	void	RenderControlHelp		(	int position, char *caption=NULL, char *iconKb=NULL, char *iconXbox=NULL, 
										float alpha = 1.0f, float highlight = 0.0f, 
										float xOffset = 0.0f, bool tutorialHighlight=false );
	void	RenderControlHelpDPad	(	int position, char *caption=NULL, char *flagIcon=NULL, 
										float alpha = 1.0f, float highlight = 0.0f, 
										float xOffset = 0.0f, bool tutorialHighlight=false );
	void	RenderOnScreenAids		();
public:
    ControlHelpSystem();
	~ControlHelpSystem();

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
		CondArmourSetTurret,
		CondSwitchPrevUnit,
		CondSwitchNextUnit,
        CondRadarAim,
		CondRotateCameraLeft,
		CondRotateCameraRight,
		MaxConditions
	};

	void RecordCondUsed( int _cond );

	void AddOnScreenAid( const char* _icon, const char* _iconShadow, UnicodeString _text, ControlType _control, CheckRunner *_checker=NULL );
	void ClearOnScreenAids();

	bool HasSquadRunning();

private:
	void InitialiseIcons();
	void InitialiseConditions();
	bool CheckCondition( int _cond );
	void SetCondIcon( int _cond );
	void DecayCond( int _cond );

private:

	typedef struct OnScreenAidInfo {
		OnScreenAidInfo( const char* _iconFilename, const char* _iconShadowFilename, UnicodeString _text, ControlType _control, CheckRunner *_checker )
			:	m_isDone(false),
				m_text(_text),
				m_control(_control),
				m_deadTime(-1.0f),
				m_checker(_checker)
		{
			sprintf(m_iconFileName, _iconFilename);
			sprintf(m_iconShadowFilename, _iconShadowFilename);
		}

		OnScreenAidInfo& operator =	(const OnScreenAidInfo &_other)
		{
			// No op for self assign
			if( this == &_other )
				return *this;

			m_isDone = _other.m_isDone;
			sprintf(m_iconFileName, _other.m_iconFileName);
			sprintf(m_iconShadowFilename, _other.m_iconShadowFilename);
			m_text = _other.m_text;
			m_control = _other.m_control;
			m_checker = _other.m_checker;

			return *this;
		}

		bool IsConditionMet()
		{
			if( m_checker )
			{
				return m_checker->Check();
			}
			return false;
		}

		bool			m_isDone;
		char			m_iconFileName[256];
		char			m_iconShadowFilename[256];
		double			m_deadTime;
		UnicodeString	m_text;
		ControlType		m_control;
		CheckRunner		*m_checker;
	};

	LList<OnScreenAidInfo*>	m_onScreenAids;

	HelpIcon *m_icons[MaxIcons];
	HelpIconSet *m_sets[MaxSets];

	typedef struct TextIndicator {
		TextIndicator() 
			: m_icon( 0 ),
			  m_text( 0 ),
			  m_minTime( 0.0 ),
			  m_maxTime( 0.0 ),
			  m_maxUsage( 0.0 ),
			  m_decayRate( 1.0 )
		{
		}

		TextIndicator( int _icon, int _text, const char *_langPhrase, float _minTime = 0.0, float _maxTime = 0.0, float _maxUsage = 0.0, float _decayRate = 1.0 )
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
