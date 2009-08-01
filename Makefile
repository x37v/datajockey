first:
	g++ -g -Iinclude -I/usr/local/include/rubberband src/* -lsndfile -lvorbisfile -lmad -ljack -ljackcpp -lrubberband -lyamlcpp -lsyck 
