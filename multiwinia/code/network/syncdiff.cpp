#include "lib/universal_include.h"
#include "network/syncdiff.h"
#include "lib/string_utils.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/math/random_number.h"

SyncDiff::SyncDiff( const WorldObjectId &_id, const Vector3 &_pos, const RGBAColour &_colour, const char *_name, const char *_description )
:	m_id( _id ),
	m_pos( _pos ),
	m_colour( _colour ),
	m_name( newStr( _name ) ),
	m_description( newStr( _description ) )
{
}


SyncDiff::~SyncDiff()
{
	delete[] m_description;
	delete[] m_name;
}


void SyncDiff::Render()
{
#ifdef DEBUG_RENDER_ENABLED
	Vector3 start = m_pos;

	start.y += 500.0;

	RenderArrow(start, m_pos, 20.0, m_colour);

	for( int i = 0; i < 2; i++)
	{
		RGBAColour colour = !i ? g_colourWhite : m_colour;
		//colour.a = !i ? 0 : 0.9;
		g_gameFont.SetRenderShadow( !i );
		glColor4ubv( colour.GetData() );

		Vector3 textPos = start;
		g_gameFont.DrawText3DSimple( textPos, 5, m_name );			textPos.y -= 10;
		g_gameFont.DrawText3DSimple( textPos, 5, m_description );
	}
#endif
}

void SyncDiff::Print( std::ostream &_o )
{
	_o	<< "syncdiff: "
		<< "id: (team: " << (int) m_id.GetTeamId() << ", unit: " << m_id.GetUnitId() << ", index: " << m_id.GetIndex() << ", uniqueId: " << m_id.GetUniqueId() << ") "
		<< "(" << m_pos.x << ", " << m_pos.y << ", " << m_pos.z << ") " 
		<< " - " << m_name << " - " << m_description << "\n";
}