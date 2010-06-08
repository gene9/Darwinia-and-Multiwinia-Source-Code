#ifndef __BUYNOWINFOBUTTON__
#define __BUYNOWINFOBUTTON__

class BuyNowInfoButton : public DarwiniaButton
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        float fontLarge, fontMed, fontSmall;
        GetFontSizes( fontLarge, fontMed, fontSmall );
        int x = realX + m_w * 0.05f;

        g_titleFont.DrawText2D( x, realY+=fontSmall, fontSmall, LANGUAGEPHRASE("multiwinia_buynow_1") );
        g_titleFont.DrawText2D( x, realY+=fontSmall, fontSmall, LANGUAGEPHRASE("multiwinia_buynow_2") );
        g_titleFont.DrawText2D( x, realY+=fontSmall, fontSmall, LANGUAGEPHRASE("multiwinia_buynow_3") );
        g_titleFont.DrawText2D( x, realY+=fontSmall, fontSmall, LANGUAGEPHRASE("multiwinia_buynow_4") );
        g_titleFont.DrawText2D( x, realY+=fontSmall, fontSmall, LANGUAGEPHRASE("multiwinia_buynow_5") );
    }
};

#endif
