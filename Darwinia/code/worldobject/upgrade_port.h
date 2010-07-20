#ifndef INCLUDED_UPGRADE_PORT_H
#define INCLUDED_UPGRADE_PORT_H


#include "building.h"


class UpgradePort: public Building
{
public:
	UpgradePort();
};



class PrimaryUpgradePort : public Building
{
public:
    int m_controlTowersOwned;

public:
    PrimaryUpgradePort();

    void ReprogramComplete();
};

#endif
