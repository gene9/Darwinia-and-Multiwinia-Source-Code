#ifndef __PASTEKEYBUTTON__
#define __PASTEKEYBUTTON__


class PasteKeyButton : public GameMenuButton
{
public:
	PasteKeyButton() : GameMenuButton("")
	{
        m_transitionDirection = -1;
	}

    void MouseUp()
    {
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
		parent->PasteFromClipboard();
    }
};

#endif
