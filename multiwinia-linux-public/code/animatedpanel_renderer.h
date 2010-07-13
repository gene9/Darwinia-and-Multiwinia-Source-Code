#ifndef _included_animatedpanelrenderer_h
#define _included_animatedpanelrenderer_h

/*
 * AnimatedPanelRenderer
 *
 * Extends AnimatedPanel to provide rendering within Multiwinia
 *
 */

#include "animatedpanel.h"


class AnimatedPanelRenderer : public AnimatedPanel
{
public:
    double m_animTimer;
    
    float m_screenX;                // Please fill these in.                
    float m_screenY;                
    float m_screenW;
    float m_screenH;
    
    float m_borderSize;
    float m_titleH;
    float m_captionH;

protected:
    void Blit ( int texId, 
                float x, float y, float w, float h, 
                float angle, float texX, float texY, float texW, float texH );

public:
    AnimatedPanelRenderer();

    void BeginRender        ();
    void RenderBackground   ( float alpha );    
    void RenderObjects      ();    
    void RenderTitle        ( UnicodeString &title );
    void RenderCaption      ( UnicodeString &caption );
    void EndRender          ();
};



#endif
