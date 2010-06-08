#ifndef __BUYNOW_WINDOW_H
#define __BUYNOW_WINDOW_H

#include "interface/darwinia_window.h"

class BuyNowWindow : public DarwiniaWindow {
public:

	BuyNowWindow();

	void Create();
    void Render(bool _hasFocus);
};

#endif // __BUYNOW_WINDOW_H