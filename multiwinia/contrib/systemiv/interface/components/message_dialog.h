#ifndef INCLUDED_MESSAGE_DIALOG_H
#define INCLUDED_MESSAGE_DIALOG_H

#include "core.h"


class InvertedBox;
class CloseButton;


class MessageDialog : public InterfaceWindow
{
protected:
	char   *m_messageLines[20];
	int     m_numLines;
	char   *m_message;
	bool    m_messageIsLanguagePhrase;
	char   *m_messageText;

	void    GetLines();

public:
	MessageDialog(char *_name, char *_message, bool _messageIsLanguagePhrase = false,
	              char *_title = NULL, bool _titleIsLanguagePhrase = false);
	~MessageDialog();

	void Create();
	void Render(bool _hasFocus);
};


#endif


