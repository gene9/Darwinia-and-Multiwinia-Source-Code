#ifndef __SYNCDIFF_H
#define __SYNCDIFF_H

#include "lib/vector3.h"
#include "lib/rgb_colour.h"
#include "worldobject/worldobject.h"

class SyncDiff 
{
public:
	SyncDiff( const WorldObjectId &_id, const Vector3 &_pos, const RGBAColour &_colour, const char *_name, const char *_description );
	~SyncDiff();

	void			Render();

	void			Print( std::ostream &_o );

private:
	WorldObjectId	m_id;
	Vector3			m_pos;
	char			*m_name;
	char			*m_description;
	RGBAColour		m_colour;

};

#endif
