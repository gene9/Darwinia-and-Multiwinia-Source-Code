#include "lib/debug_utils.h"

#include "lib/input/inputdriver_sdl_key.h"
#include "lib/input/sdl_eventhandler.h"
#include "lib/input/keynames.h"
#include "lib/language_table.h"
#include "app.h"
#include "eclipse.h"

#include <SDL/SDL.h>

using std::string;

static int ConvertSDLKeyIdToWin32KeyId(SDLKey _keyCode)
{
	int keyCode = (int) _keyCode;

	// Specific mappings
	switch (keyCode) {
		case SDLK_COMMA: return KEY_COMMA;
		case SDLK_PERIOD: return KEY_STOP;
		case SDLK_SLASH: return KEY_SLASH;
		case SDLK_QUOTE: return KEY_QUOTE;
		case SDLK_LEFTBRACKET: return KEY_OPENBRACE;
		case SDLK_RIGHTBRACKET: return KEY_CLOSEBRACE;
		case SDLK_MINUS: return KEY_MINUS;
		case SDLK_EQUALS: return KEY_EQUALS;
		case SDLK_SEMICOLON: return KEY_COLON;
		case SDLK_LEFT: return KEY_LEFT;
		case SDLK_RIGHT: return KEY_RIGHT;
		case SDLK_UP: return KEY_UP;
		case SDLK_DOWN: return KEY_DOWN;
		case SDLK_DELETE: return KEY_DEL;
		case SDLK_CAPSLOCK: return KEY_CAPSLOCK;
		case SDLK_BACKSLASH: return KEY_BACKSLASH;
		case SDLK_BACKQUOTE: return KEY_TILDE;
		
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			return KEY_SHIFT;
			
		case SDLK_LCTRL:
		case SDLK_RCTRL:
			return KEY_CONTROL;
		
		case SDLK_LMETA:
		case SDLK_RMETA:
			return KEY_META;
		
		case SDLK_LALT:
		case SDLK_RALT:
			return KEY_ALT;
	}

	if (keyCode >= 97 && keyCode <= 122)		keyCode -= 32;	// a-z
	else if (keyCode >= 8 && keyCode <= 57)		keyCode -= 0;	// 0-9 + BACKSPACE + TAB
	else if (keyCode >= 282 && keyCode <= 293)	keyCode -= 170; // Function keys
	else keyCode = 0;

	return keyCode;
}


SDLKeyboardInputDriver::SDLKeyboardInputDriver()
{
	setName("SDLKeyboard");
	getSDLEventHandler()->AddEventProcessor(this);
	//On linux, we re-create the window on resolution change.
	//I therefore enable Unicode in window_manager_sdl.cpp, not here.
	#ifndef TARGET_OS_LINUX
	SDL_EnableUNICODE(1);
	#endif
	
	memset(m_keys, 0, KEY_MAX * sizeof(bool));
	memset(m_keyDeltas, 0, KEY_MAX * sizeof(int));
	memset(m_keyNewDeltas, 0, KEY_MAX * sizeof(int));
}

int SDLKeyboardInputDriver::HandleSDLEvent(const SDL_Event & event)
{
	switch (event.type)
	{
		// Note that there are two paths for keystroke input into the game. One can either
		// poll the keyboard state (m_keys) or read from the character stream (EclUpdateKeyboard()).
		// The first uses method uses raw Windows key codes, but the second takes ASCII/Unicode values.
		case SDL_KEYDOWN:
		{
			uint16_t unicode = event.key.keysym.unicode;
			int keyCode = ConvertSDLKeyIdToWin32KeyId(event.key.keysym.sym);
			if (unicode == SDLK_DELETE)
				unicode = SDLK_BACKSPACE;
			m_keys[keyCode] = true;
			m_keyNewDeltas[keyCode] = 1;
			
			SDLMod modifiers = event.key.keysym.mod;
			if (0 < unicode && unicode <= 255)
			{
#ifdef TARGET_OS_MACOSX
				// Use the Command key, not Control, since that's what Mac users expect
				int commandKeyMask = KMOD_META;
#else
				int commandKeyMask = KMOD_CTRL;
#endif
				EclUpdateKeyboard(unicode, modifiers & KMOD_SHIFT, modifiers & commandKeyMask,
								  modifiers & KMOD_ALT, unicode);
			}
			return 0;
		}
			
		case SDL_KEYUP:
		{
			int keyCode = ConvertSDLKeyIdToWin32KeyId(event.key.keysym.sym);
			m_keys[keyCode] = false;
			m_keyNewDeltas[keyCode] = -1;
			return 0;
		}
	}
	
	return -1; // unhandled
}

bool SDLKeyboardInputDriver::getFirstActiveInput( InputSpec &spec, bool instant )
{
	// Check for pressed keys
	for ( unsigned i = 0; i <= KEY_META; ++i ) {
		if ( 1 == m_keyDeltas[ i ] && // key was pressed
		     strstr( getKeyNames()[ i ], " " ) == NULL ) { // Key is bindable
			spec.control_id = i;
			if ( instant )
				spec.condition = COND_DOWN;
			else
				spec.condition = COND_PRESSED;
			spec.type = INPUT_TYPE_BOOL;
			return true;
		}
	}

	// We found no active inputs
	return false;
}

void SDLKeyboardInputDriver::Advance()
{
	// KLUDGE: Poll for key-up events, since we occasionally miss some. It's frequent enough
	// to be a problem, but hard enough to reproduce that it's not worth tracking down properly.
	// It may be an OS-level issue in any case.
	int sdlKeyMax;
	Uint8 *sdlKeyStates = SDL_GetKeyState(&sdlKeyMax);
	for (int sdlKey = 0; sdlKey < sdlKeyMax; sdlKey++)
	{
		if (m_keys[sdlKey] && !sdlKeyStates[sdlKey])
		{
			int keyCode = ConvertSDLKeyIdToWin32KeyId((SDLKey)sdlKeyStates[sdlKey]);
			m_keys[keyCode] = false;
			m_keyNewDeltas[keyCode] = -1;
		}
	}
	
	memcpy(m_keyDeltas, m_keyNewDeltas, sizeof(int) * KEY_MAX);
	memset(m_keyNewDeltas, 0, sizeof(int) * KEY_MAX);
}

void SDLKeyboardInputDriver::PollForEvents()
{
	// not implemented yet
}

bool SDLKeyboardInputDriver::acceptDriver( string const &name )
{
	return ( name == "key" );
}

control_id_t SDLKeyboardInputDriver::getControlID( string const &name )
{
	for ( control_id_t i = 0; i <= KEY_META; ++i ) {
		if ( stricmp( name.c_str() , getKeyNames()[i] ) == 0 )
			return i;
	}

	if ( stricmp( name.c_str(), "any" ) == 0 ) return KEY_ANY;

	return -1;
}

bool SDLKeyboardInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	int button = spec.control_id;
	AppDebugAssert( 0 <= button && button < KEY_MAX );
	details.type = INPUT_TYPE_BOOL;

	switch ( spec.condition ) {
		case COND_DOWN:    return ( m_keyDeltas[button] == 1 );
		case COND_UP:      return ( m_keyDeltas[button] == -1 );
		case COND_PRESSED: return m_keys[button];
		default:           return false; // We should never get here!
	}
}

bool SDLKeyboardInputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	bool keyInRange = ( 0 <= spec.control_id && spec.control_id <= KEY_META );
	if ( keyInRange ) {
		desc.noun = "control_";
		desc.noun += getKeyNames()[ spec.control_id ];
		if ( ISLANGUAGEPHRASE( desc.noun.c_str() ) )
			desc.noun = RAWLANGUAGEPHRASE( desc.noun.c_str() );
		else
			desc.noun = getKeyNames()[ spec.control_id ];
	} else
		desc.noun = "control_key_unknown";

	std::string prefCond;
	if ( getDefaultVerb( spec.condition, INPUT_TYPE_BOOL, desc.verb ) &&
		 getDefaultPrefsString( spec.condition, INPUT_TYPE_BOOL, prefCond ) ) {
		desc.pref = "key ";
		desc.pref += desc.noun + " " + prefCond;
		return keyInRange;
	} else {
		desc.pref = "unknown_key_pref_string";
		return false;
	}
}

inputtype_t SDLKeyboardInputDriver::getControlType( control_id_t control_id )
{
	return INPUT_TYPE_BOOL;
}

SDLKeyboardInputDriver::~SDLKeyboardInputDriver()
{
	getSDLEventHandler()->RemoveEventProcessor(this);
}
