 #include "lib/universal_include.h"

#include <time.h>

#include "lib/hi_res_time.h"
//#include "lib/eclipse/eclipse.h"
#include "lib/debug_utils.h"

#include "input.h"
#include "window_manager.h"

#include <memory.h>

InputManager *g_inputManager = NULL;
signed char g_keyDeltas[KEY_MAX];
signed char g_keys[KEY_MAX];

// *** Constructor
InputManager::InputManager()
:   m_windowHasFocus(true),
    m_lmb(false),
    m_mmb(false),
    m_rmb(false),
    m_lmbOld(false),
    m_mmbOld(false),
    m_rmbOld(false),
	m_rawLmbClicked(false),
    m_lmbClicked(false),
    m_mmbClicked(false),
    m_rmbClicked(false),
    m_lmbUnClicked(false),
    m_mmbUnClicked(false),
    m_rmbUnClicked(false),
    m_mouseX(0),
    m_mouseY(0),
    m_mouseZ(0),
    m_mouseOldX(0),
    m_mouseOldY(0),
    m_mouseOldZ(0),
    m_mouseVelX(0),
    m_mouseVelY(0),
    m_mouseVelZ(0),
    m_altTabBound(false),
	m_quitRequested(false)
{
	memset(g_keys, 0, KEY_MAX);
	memset(g_keyDeltas, 0, KEY_MAX);
	memset(m_keyNewDeltas, 0, KEY_MAX);
}

// *** Advance
void InputManager::Advance()
{
	memcpy(g_keyDeltas, m_keyNewDeltas, KEY_MAX);
	memset(m_keyNewDeltas, 0, KEY_MAX);

	m_lmbClicked = m_lmb == true && m_lmbOld == false;
	m_mmbClicked = m_mmb == true && m_mmbOld == false;
	m_rmbClicked = m_rmb == true && m_rmbOld == false;
	m_rawLmbClicked = m_lmbClicked;
	m_lmbUnClicked = m_lmb == false && m_lmbOld == true;
	m_mmbUnClicked = m_mmb == false && m_mmbOld == true;
	m_rmbUnClicked = m_rmb == false && m_rmbOld == true;
	m_lmbOld = m_lmb;
	m_mmbOld = m_mmb;
	m_rmbOld = m_rmb;

	m_mouseVelX = m_mouseX - m_mouseOldX;
	m_mouseVelY = m_mouseY - m_mouseOldY;
	m_mouseVelZ = m_mouseZ - m_mouseOldZ;
	m_mouseOldX = m_mouseX;
	m_mouseOldY = m_mouseY;
	m_mouseOldZ = m_mouseZ;
}


void InputManager::SetMousePos(int x, int y)
{
	m_mouseX = x;
	m_mouseY = y;
	g_windowManager->SetMousePos(x, y);
}

#ifdef TARGET_OS_MACOSX
#define META_KEY_NAME "COMMAND"
#elif defined TARGET_MSVC
#define META_KEY_NAME "WINDOWS"
#else
#define META_KEY_NAME "META" 
#endif 

static char *s_keyNames[] = 
{
	"unknown 0",
	"unknown 1",
	"unknown 2",
	"unknown 3",
	"unknown 4",
	"unknown 5",
	"unknown 6",
	"unknown 7",
	"BACKSPACE",
	"TAB",
	"unknown 10",
	"unknown 11",
	"unknown 12",
	"ENTER",
	"unknown 14",
	"unknown 15",
	"SHIFT",
	"CONTROL",
	"ALT",
	"PAUSE",
	"CAPSLOCK",
	"unknown 21",
	"unknown 22",
	"unknown 23",
	"unknown 24",
	"unknown 25",
	"unknown 26",
	"ESC",
	"unknown 28",
	"unknown 29",
	"unknown 30",
	"unknown 31",
	"SPACE",
	"PGUP",
	"PGDN",
	"END",
	"HOME",
	"LEFT",
	"UP",
	"RIGHT",
	"DOWN",
	"unknown 41",
	"unknown 42",
	"unknown 43",
	"unknown 44",
	"INSERT",
	"DEL",
	"unknown 47",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"unknown 58",
	"unknown 59",
	"unknown 60",
	"unknown 61",
	"unknown 62",
	"unknown 63",
	"unknown 64",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"unknown 91",
	"unknown 92",
	"unknown 93",
	"unknown 94",
	"unknown 95",
	"0_PAD",
	"1_PAD",
	"2_PAD",
	"3_PAD",
	"4_PAD",
	"5_PAD",
	"6_PAD",
	"7_PAD",
	"8_PAD",
	"9_PAD",
	"ASTERISK",
	"PLUS_PAD",
	"unknown 108",
	"MINUS_PAD",
	"DEL_PAD",
	"SLASH_PAD",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"unknown 124",
	"unknown 125",
	"unknown 126",
	"unknown 127",
	"unknown 128",
	"unknown 129",
	"unknown 130",
	"unknown 131",
	"unknown 132",
	"unknown 133",
	"unknown 134",
	"unknown 135",
	"unknown 136",
	"unknown 137",
	"unknown 138",
	"unknown 139",
	"unknown 140",
	"unknown 141",
	"unknown 142",
	"unknown 143",
	"NUMLOCK",
	"SCRLOCK",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"COLON",
	"EQUALS",
	"COMMA",
	"MINUS",
	"STOP",
	"SLASH",
	"QUOTE",	//192
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"OPENBRACE", // 219
	"BACKSLASH",
	"CLOSEBRACE",
	"unknown 222",
	"TILDE",
	META_KEY_NAME 
};

char const *InputManager::GetKeyName(int _keyId)
{
	AppAssert(_keyId >= 0 && _keyId <= KEY_TILDE);
	return s_keyNames[_keyId];
}


int InputManager::GetKeyId(char const *_keyName)
{
	for (int i = 0; i <= KEY_TILDE; ++i)
	{
		if (stricmp(_keyName, s_keyNames[i]) == 0)
		{
			return i;
		}
	}

	return -1;
}

#if defined TARGET_OS_MACOSX || TARGET_OS_LINUX
#include "input_sdl.h"
#else
#include "input_win32.h"
#endif 

InputManager *InputManager::Create()
{
#if defined TARGET_OS_MACOSX || TARGET_OS_LINUX
	return new InputManagerSDL();
#else
	return new InputManagerWin32();
#endif 
}
