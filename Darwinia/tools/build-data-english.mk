data/$(DARWINIA_LANG)_demo.txt: data/$(DARWINIA_LANG).txt
	perl tools/copystrings.pl data/english.txt > data/english_demo.txt

