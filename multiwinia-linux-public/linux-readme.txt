Multiwinia Linux Port
---------------------

This port uses CMake for its build system. You will have to edit the
CMakeLists.txt file to set your architecture.

Then create a directory under "targets" (called, eg, linux64-cmake) and
run (from that directory):

cmake ../..		# Add -DCMAKE_BUILD_TYPE=Debug for debug symbols
make			# I'd recommend using make -jN to spread compilation
			# load over multiple cores/processors.

The game should compile with gcc 4.4.3 and clang 1.1

Copy the resultant "Multiwinia" binary and "tools/open-www.sh" to a directory
with the data files from a retail (or otherwise) copy of Multiwinia.

The game (should) auto-detect most settings on first run, including system
language and screen resolution and stores them in ~/.Multiwinia/multiwinia
(or ~/.Multiwinia/debug if compiled with TARGET_DEBUG defined).

You should be able to change the screen resolution and settings in-game without
requiring a restart.

The sound system is currently rather fragile. If it segfaults on startup and
you're using the SDL ALSA backend try either setting $AUDIODEV to the correct
ALSA device, or recompiling without INVOKE_CALLBACK_FROM_SOUND_THREAD defined
in code/lib/universal_include.h (The sound will probably crackle like hell)

I've changed a lot of include paths and other things in the code, including
unrar and binreloc for 64-bit compatibility. Hopefully this doesn't break other
systems, but I don't at present have a windows system to test on.

Known Bugs/ToDo List:

(I'll put this on the wiki, where it'll be updated)

* Fix the sound system mess
* open-www.sh starts chromium instead of firefox with x-www-browser, why?
* Add Paste support on Authkey screen
* Alt+Tab in fullscreen mode (requires SDL hackery)
* Improve unrar performance on 64bit
* Make sure the Win32/Mac versions still build.
* Implement better fallbacks in system language detection
* Implement locale detection for keyboard layouts.
* Improve CMake script (autodetect architecture, et al)
* Make preferences path nicer (adhere to XDG?)
* Clean up the billion compile warnings
* Fix soul bootloader's skewed text
* Fix darwinia mode.
* Make editor compile and run.
* Add SDL joystick support and linux evdev XInput support
* Compatability with windows dev version.
* Compatability with release (sync, net-encrypt, etc)
* Fix the ~millions of undiscovered bugs.
* And other fun things I've forgotten.

If you want to contact me, either PM "multimania" or email me at
david@ingeniumdigital.com.
