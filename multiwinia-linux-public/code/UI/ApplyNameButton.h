#ifndef __APPLYNAMEBUTTON__
#define __APPLYNAMEBUTTON__

class ApplyNameButton : public BackPageButton
{
public:
	ApplyNameButton(int page)
	:	BackPageButton(page)
	{
	}

	void MouseUp()
	{
		GameMenuWindow *parent = (GameMenuWindow *)m_parent;
		g_prefsManager->SetString("PlayerName", parent->m_teamName.Get());
		g_prefsManager->Save();
		BackPageButton::MouseUp();
	}
};

#endif
