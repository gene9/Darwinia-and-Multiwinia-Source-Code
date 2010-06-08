#!/bin/bash

CURRENT_DIR=`pwd | sed s_.*/__`
if [ $CURRENT_DIR != "data" -a $CURRENT_DIR != "Data" ]; then
	echo "You must run this script from the Darwinia data directory"
	exit
fi

grep -i "samplegroup " sounds.txt | sed "s/[ \t]*SAMPLEGROUP[ \t]*//" | sort | uniq > _samplegroups.txt
grep -i "soundname " sounds.txt | sed "s/[ \t]*SOUNDNAME[ \t]*//" | sort | uniq > _soundNames.txt
diff _samplegroups.txt _soundNames.txt --side-by-side | grep "[\|<]"