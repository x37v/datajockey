#-ffast-math avoids denormals i think
first:
	g++ -g -ffast-math -Iinclude -I/usr/local/include/rubberband src/* -lsndfile -lvorbisfile -lmad -ljack -ljackcpp -lrubberband -lyamlcpp -lsyck 
