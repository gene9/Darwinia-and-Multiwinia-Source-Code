#ifndef INCLUDED_MESSAGE_DIALOG_H
#define INCLUDED_MESSAGE_DIALOG_H


#include "interface/darwinia_window.h"

class MessageDialog : public DarwiniaWindow
{
protected:
	char *m_messageLines[20];
	int m_numLines;

public:
	MessageDialog(char const *_name, char const *_message);
	~MessageDialog();

	void Create();
	void Render(bool _hasFocus);
};


#endif
