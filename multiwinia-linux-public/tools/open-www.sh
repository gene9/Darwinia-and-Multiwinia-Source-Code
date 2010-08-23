#!/bin/sh

# Launch browser with a given URL. Improvements welcome
# send to john@introversion.co.uk

export PATH=$PATH:/usr/bin:/bin:/usr/local/bin:/opt/kde/bin:/opt/kde3/bin:/usr/local/mozilla/bin:

xdg-open "$@" || \
x-www-browser "$@" || \
www-browser "$@" || \
kfmclient openURL "$@" || \
firefox "$@" || \
mozilla-firefox "$@" || \
mozilla "$@" || \
chromium "$@" || \
google-chrome "$@" || \
galeon  "$@" || \
opera "$@"
