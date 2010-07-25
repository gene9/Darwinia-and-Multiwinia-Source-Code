#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/file_writer.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/text_stream_readers.h"
#include "lib/hi_res_time.h"

#include "app.h"
#include "team.h"
#include "location.h"
#include "entity_grid.h"
#include "level_file.h"
#include "location_editor.h"
#include "global_world.h"

#include "worldobject/snowbox.h"


SnowBox::SnowBox()
:   Entity()
{
    SetType( TypeSnowBox );
	g_app->m_location->m_isSnowing = true;


}


void SnowBox::ChangeHealth( int _amount )
{
}
