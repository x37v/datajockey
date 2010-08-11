#-ffast-math avoids denormals i think
INCLUDES = -Iinclude -I/usr/include/rubberband 
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
		src/schedulenode.cpp \
		src/soundfile.cpp \
		src/timepoint.cpp \
		src/transport.cpp \

OBJ = ${SRC:.cpp=.o}

first: datajockey

#the master depends on the scheduler
src/master.o: src/scheduler.o

.cpp.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $*.o $<

datajockey: ${OBJ}
	g++ -o $@ ${CFLAGS} ${OBJ} ${LIBS}

clean:
	rm -f ${OBJ} datajockey
