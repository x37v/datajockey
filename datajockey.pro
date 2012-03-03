######################################################################
# Automatically generated by qmake (2.01a) Sun Feb 19 14:48:57 2012
######################################################################

TEMPLATE = app
TARGET = datajockey
LIBS += -lsndfile -lvorbisfile -lmad -ljack -ljackcpp -lrubberband -lslv2
QT += dbus
DEPENDPATH += . \
              include/audio \
              include/controller \
              include/view \
              src/audio \
              src/controller \
              src/view
MOC_DIR = moc/
OBJECTS_DIR = obj/
INCLUDEPATH += . \
   include/ \
   include/audio \
   include/controller \
   include/view \
   /usr/include/rubberband /usr/include/rasqal/

# Input
HEADERS += include/defines.hpp \
           include/audio/audiobuffer.hpp \
           include/audio/audioio.hpp \
           include/audio/beatbuffer.hpp \
           include/audio/command.hpp \
           include/audio/doublelinkedlist.hpp \
           include/audio/master.hpp \
           include/audio/player.hpp \
           include/audio/scheduledataparser.hpp \
           include/audio/schedulenode.hpp \
           include/audio/scheduler.hpp \
           include/audio/soundfile.hpp \
           include/audio/stretcher.hpp \
           include/audio/stretcher_rate.hpp \
           include/audio/timepoint.hpp \
           include/audio/transport.hpp \
           include/audio/types.hpp \
           include/controller/audiobufferreference.hpp \
           include/controller/audiocontroller.hpp \
           include/controller/audioloaderthread.hpp \
           include/controller/playermapper.hpp \
           include/view/player_view.hpp \
           include/view/waveformitem.hpp
SOURCES += src/defines.cpp \
           src/audio/audiobuffer.cpp \
           src/audio/audioio.cpp \
           src/audio/beatbuffer.cpp \
           src/audio/command.cpp \
           src/audio/doublelinkedlist.cpp \
           src/audio/master.cpp \
           src/audio/player.cpp \
           src/audio/schedulenode.cpp \
           src/audio/scheduler.cpp \
           src/audio/soundfile.cpp \
           src/audio/stretcher.cpp \
           src/audio/stretcher_rate.cpp \
           src/audio/timepoint.cpp \
           src/audio/transport.cpp \
           src/controller/audiobufferreference.cpp \
           src/controller/audiocontroller.cpp \
           src/controller/audioloaderthread.cpp \
           src/controller/playermapper.cpp \
           src/view/main.cpp \
           src/view/player_view.cpp \
           src/view/waveformitem.cpp