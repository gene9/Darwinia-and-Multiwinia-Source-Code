#ifndef _included_rocketpanel_h
#define _included_rocketpanel_h

class EscapeRocket;



class RocketStatusPanel
{
public:
    float m_x;
    float m_y;
    float m_w;
    int   m_teamId;
    
protected:
    double m_lastDamage;
    double m_damageTimer;
    double m_previousFuelLevel;

public:
    RocketStatusPanel();

    EscapeRocket *GetMyRocket();

    void Render();
};


#endif
