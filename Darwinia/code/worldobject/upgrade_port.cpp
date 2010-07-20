#include "lib/universal_include.h"
#include "lib/resource.h"

#include "lib/debug_utils.h"

#include "worldobject/upgrade_port.h"

#include "app.h"
#include "global_world.h"


UpgradePort::UpgradePort()
:   Building()
{
	m_type = TypeUpgradePort;
	SetShape( g_app->m_resource->GetShape("upgrade_port.shp") );
}



PrimaryUpgradePort::PrimaryUpgradePort()
:   Building(),
    m_controlTowersOwned(0)
{
    m_type = TypePrimaryUpgradePort;
    SetShape( g_app->m_resource->GetShape("primaryupgradeport.shp" ) );
}


void PrimaryUpgradePort::ReprogramComplete()
{
    m_controlTowersOwned++;

    if( m_controlTowersOwned == 3 )
    {
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb )
        {
            gb->m_online = true;
            g_app->m_globalWorld->EvaluateEvents();
        }
    }
}
