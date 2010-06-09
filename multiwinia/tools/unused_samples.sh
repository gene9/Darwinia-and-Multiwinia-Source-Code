#!/bin/bash

CURRENT_DIR=`pwd | sed s_.*/__`
if [ $CURRENT_DIR != "data" -a $CURRENT_DIR != "Data" ]; then
	echo "You must run this script from the Darwinia data directory"
	exit
fi

grep -i "soundname " sounds.txt | sed "s/[ \t]*SOUNDNAME[ \t]*//" > _sampleNames.txt
grep -i "sample " sounds.txt | sed "s/[ \t]*SAMPLE[ \t]*//" >> _sampleNames.txt
sort _sampleNames.txt | uniq | tr [A-Z] [a-z] > _sampleNames2.txt

command ls -l sounds | awk '{ print $9 }' | tr [A-Z] [a-z] | sed "s/\.ogg//" > _samplesOnDisk.txt

diff _samplesOnDisk.txt _sampleNames2.txt --side-by-side | grep "[\|<]"