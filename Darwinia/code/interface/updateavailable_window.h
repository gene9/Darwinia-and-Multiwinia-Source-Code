#if !defined __UPDATE_AVAILABLEWINDOW_H
#define __UPDATE_AVAILABLEWINDOW_H

#include "interface/message_dialog.h"

class UpdateAvailableWindow : public MessageDialog {
public:
	UpdateAvailableWindow( const char *newVersion, const char *changeLog );
	void Create();
};

#endif