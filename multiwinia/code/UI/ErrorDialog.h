#ifndef __ERROR_DIALOG__
#define __ERROR_DIALOG__


class ErrorDialog : public DarwiniaWindow
{
public:
	ErrorDialog() : DarwiniaWindow("multiwinia_mainmenu_title"){};
	ErrorDialog(UnicodeString _error);
	void Create ();
    void Update();
    void Render ( bool _hasFocus );

protected:
	UnicodeString		m_message;


};



#endif
