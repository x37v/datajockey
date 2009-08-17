#-ffast-math avoids denormals i think
INCLUDES = -Iinclude -I/usr/local/include/rubberband 
LIBS = -lsndfile -lvorbisfile -lmad -ljack -ljackcpp -lrubberband -lyamlcpp -lsyck 
CFLAGS = -g -ffast-math ${INCLUDES}

SRC = \
		src/audiobuffer.cpp \
		src/audioio.cpp \
		src/beatbuffer.cpp \
		src/command.cpp \
		src/main.cpp \
		src/master.cpp \
		src/player.cpp \
		src/scheduler.cpp \
		src/soundfile.cpp \
		src/timepoint.cpp \
		src/transport.cpp \

OBJ = ${SRC:.cpp=.o}

.cpp.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $*.o $<

first: ${OBJ}
	g++ ${CFLAGS} ${OBJ} ${LIBS}

clean:
	rm ${OBJ}
