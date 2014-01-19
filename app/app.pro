#-------------------------------------------------
#
# Project created by QtCreator 2013-12-15T10:41:54
#
#-------------------------------------------------

TARGET = app
TARGET = datajockey
VERSION = 1.2.git

QT       += core gui sql opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += debug
CONFIG += c++11
CONFIG += link_pkgconfig
PKGCONFIG += sndfile vorbisfile mad yaml-cpp taglib
LIBS += -loscpack
//LIBS += -lboost_program_options-mt -lboost_filesystem-mt -lboost_regex-mt -lboost_system-mt 
LIBS += -lboost_program_options -lboost_filesystem -lboost_regex -lboost_system

macx {
  QMAKE_LIBDIR += ../ext/vamp/osx/
  LIBS += -lvamp-hostsdk -ljack
  #QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.6.sdk
  INCLUDEPATH += ../ext/vamp/
}

unix:!macx {
  PKGCONFIG += jack lilv-0 vamp-hostsdk
  DEFINES += USE_LV2
}

DENORMAL_FLAGS = -msse -mfpmath=sse -ffast-math
QMAKE_CXXFLAGS += $$DENORMAL_FLAGS -fexceptions -DDJ_VERSION=$$VERSION

MOC_DIR = moc/
OBJECTS_DIR = obj/
UI_DIR = ui/

SOURCES += main.cpp\
        mainwindow.cpp \
    playerview.cpp \
    mixerpanelview.cpp \
    workdetailview.cpp \
    tagsview.cpp \
    workfilterview.cpp \
    db.cpp \
    workstableview.cpp \
    audiomodel.cpp \
    audio/transport.cpp \
    audio/timepoint.cpp \
    audio/stretcherrate.cpp \
    audio/stretcher.cpp \
    audio/soundfile.cpp \
    audio/scheduler.cpp \
    audio/schedulenode.cpp \
    audio/player.cpp \
    audio/master.cpp \
    audio/envelope.cpp \
    audio/doublelinkedlist.cpp \
    audio/command.cpp \
    audio/audioio.cpp \
    audio/audiobuffer.cpp \
    audio/annotation.cpp \
    ../ext/jackcpp/src/jackmidiport.cpp \
    ../ext/jackcpp/src/jackblockingaudioio.cpp \
    ../ext/jackcpp/src/jackaudioio.cpp \
    defines.cpp \
    config.cpp \
    audioloader.cpp \
    loaderthread.cpp \
    waveformgl.cpp \
    mixerpanelwaveformsview.cpp

HEADERS  += mainwindow.h \
    playerview.h \
    mixerpanelview.h \
    workdetailview.h \
    tagsview.h \
    workfilterview.h \
    db.h \
    workstableview.h \
    audiomodel.h \
    audio/types.hpp \
    audio/transport.hpp \
    audio/timepoint.hpp \
    audio/stretcherrate.hpp \
    audio/stretcher.hpp \
    audio/soundfile.hpp \
    audio/scheduler.hpp \
    audio/schedulenode.hpp \
    audio/scheduledataparser.hpp \
    audio/player.hpp \
    audio/master.hpp \
    audio/envelope.hpp \
    audio/doublelinkedlist.hpp \
    audio/command.hpp \
    audio/audioio.hpp \
    audio/audiobuffer.hpp \
    audio/annotation.hpp \
    ../ext/jackcpp/include/jackringbuffer.hpp \
    ../ext/jackcpp/include/jackmidiport.hpp \
    ../ext/jackcpp/include/jackblockingaudioio.hpp \
    ../ext/jackcpp/include/jackaudioio.hpp \
    defines.hpp \
    config.hpp \
    audioloader.h \
    loaderthread.h \
    waveformgl.h \
    mixerpanelwaveformsview.h

FORMS    += mainwindow.ui \
    playerview.ui \
    mixerpanelview.ui \
    workdetailview.ui \
    tagsview.ui \
    workfilterview.ui

INCLUDEPATH += . \
	audio \
	../ext/jackcpp/include/
