
#ifndef included_shaman_interface_h
#define included_shaman_interface_h

#include "lib/vector2.h"
#include "lib/tosser/llist.h"

#include "worldobject/worldobject.h"

class ShamanIcon
{
public:
	int m_iconId;
	int m_type;
	Vector2 m_screenPos;

	char *m_iconFilename;

public:
	ShamanIcon( int _type );

	void Render( float _alpha );
	void SetPos( int _x, int _y );
};
 
class ShamanInterface
{
protected:

	WorldObjectId	m_currentShamanId;
	bool			m_visible;

	LList<ShamanIcon *> m_icons;
	int				m_currentSelectedIcon;

	float			m_alpha;
	float			m_circleOffset;
	int				m_movement;

protected:

	void RenderSelectionIcon();
	void RenderBackground();
	void RenderEggs();

	void AdvanceMouseInIcon();
	void AdvanceSelectionInput();

	void SetupIcons();

public:
	ShamanInterface();

	void Advance();
	void Render();

	void ShowInterface( WorldObjectId _shamanId );
	void HideInterface();

	bool IsVisible();

	WorldObjectId GetShamanId();
	void GetIconPos( int _iconNumber, int *_x, int *_y, bool _useOffset = true );
    int GetNumIcons();

	static char *GetIconFileName( int _type );
};

#endif