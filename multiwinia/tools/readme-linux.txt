DARWINIA by Introversion Software
Copyright (c) 2005, All Rights Reserved

System requirements:

Enabled 3D graphics acceleration in X

glibc 2.2.4 or higher 
  (Darwinia requires glibc 2.2 and supplied libgcc_s.so.1 requires glibc 2.2.4)

libstdc++.so.5 

Advanced Configuration and Troubleshooting
==========================================

The demo and full game uses ~/.darwinia/demo or ~/.darwinia/full
respectively to store user preferences and save-games. In that
directory you should find a file called preferences.txt which contains
various settings. Some of the more important ones are described below:

  RenderLandscapeMode
     try setting it to 1 or 2 if you are experiencing
     polygon flicker, especially with the ATI fglrx drivers. This
     may also speed up rendering for those with fast graphics cards.

  ManuallyScaleTextures
     try setting it to 1 if the game crashes at the beginning
     and the blackbox.txt mentions gluBuild2DMipmaps

  XineramaHack
     try setting it to 0 if the mouse has a constant shift in any
     direction. This will disable guessing the Xinerama mouse offset.

Running the full game
=====================

In order to play the full game, you need to download the full Linux version from 
http://www.darwinia.co.uk/support/linux.html and supplement the data files from the 
Windows CD-ROM.

Sound
=====

The default sound mode is OSS. If you prefer to use ALSA, make sure that
the SDL_AUDIODRIVER environment variable is set to alsa:

 $ export SDL_AUDIODRIVER=alsa


Acknowledgements
================

Nathan Parslow (Bruce) for assistance with Linux installer.
All Linux beta testers for feedback, bug reports and suggestions.
Loki Software and contributors for SDL and Loki Setup tool.
