#ifndef INCLUDED_LOCATION_INPUT
#define INCLUDED_LOCATION_INPUT


class Building;
class Engineer;


class LocationInput
{
private:
	void	AdvanceRadarDishControl(Building *_building);
    void    AdvanceCarryableControl(Building *_building);
	void	AdvanceNoSelection();
    void	AdvanceTeamControl();

    void    IssueDarwinianOrders( Vector3 const &pos );

    void    SelectObjectUnderMouse( WorldObjectId &objId );

public:
    int     m_routeId;
    bool    m_chatToggledThisUpdate;

public:
    LocationInput();

    bool    GetObjectUnderMouse( WorldObjectId &_id, int _teamId );

	void	Advance();
	void	Render();
};


#endif
