#!/bin/sh

# Launch browser with a given URL. Improvements welcome
# send to john@introversion.co.uk

export PATH=$PATH:/usr/bin:/bin:/usr/local/bin:/opt/kde/bin:/opt/kde3/bin:/usr/local/mozilla/bin:

kfmclient openURL "$@" || \
firefox "$@" || \
mozilla-firefox "$@" || \
mozilla "$@" || \
galeon  "$@" || \
opera "$@"
