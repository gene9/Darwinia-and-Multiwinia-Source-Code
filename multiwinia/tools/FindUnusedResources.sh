#!/bin/bash

CURRENT_DIR=`pwd | sed s_.*/__`
if [ $CURRENT_DIR != "Darwinia" -a $CURRENT_DIR != "darwinia" ]; then
	echo "You must run this script from the ROOT of the Darwinia project"
	exit
fi

# =======================================================
# Find all the resource names that are referenced by code
# =======================================================

echo "Listing resource names that are used"

cd code
rm -f FURtemp*

# Find all the calls of GetShape(), GetSound() etc
for i in shape sound texture; do
	grep -i -h "get$i[ \t]*(" `find . -name "*.cpp"` `find . -name "*.h"` > FURtemp1
	
	# Remove the bogus result that comes from the "Get Resource" function
	# declarations and definitions (They don't contain resource names
	# delimited by quotes).
	grep -h \" FURtemp1 > FURtemp2
	
	# Remove the part of the line that precedes the resource name
	sed "s/^.*Get$i[\t \(\"]*//i" FURtemp2 > FURtemp3
	
	# Remove any directory name present in the filename
	sed "s/.*[\/\\]//" FURtemp3 >> FURtemp4
done

# Remove the part of the line after the resource name
sed "s/\".*$//" FURtemp4 > FURtemp5
	
sort -f FURtemp5 | uniq -i > FURtemp6

cd -


# ========================================================
# Find all the resource file names present in the data dir
# ========================================================

echo "Listing resource files in the data directory"

cd data
rm -f FURtemp*

# Find all the resource file names
for i in BMP SHP OGG JPG WAV bmp shp ogg jpg wav; do
	find . -name "*.$i" >> FURtemp
done

# Strip the path data from the filenames
sed s_.*/__ FURtemp > FURtemp2

sort -f FURtemp2 | uniq -i > FURtemp3

cd -


# =====================================================
# Find all the differences between the two lists
# =====================================================

echo
diff -i code/FURtemp6 data/FURtemp3 | grep -E "[<>]" | sort | sed "s/</Missing:/" | sed "s/>/Reduntant:/" > FURtemp
grep -v -E -f tools/FindUnusedResourcesExceptions.txt FURtemp

rm -f FURtemp code/FURtemp* data/FURtemp*
