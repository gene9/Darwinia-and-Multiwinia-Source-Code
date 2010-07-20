#!/bin/bash

grep g_prefsManager `find . -name "*.cpp"` | sed s/[^\"]*.// | sed s/\".*// | sort | uniq
