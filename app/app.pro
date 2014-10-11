#-------------------------------------------------
#
# Project created by QtCreator 2013-12-15T10:41:54
#
#-------------------------------------------------

TARGET = datajockey
VERSION = 1.2.git

QT       += core gui sql opengl

isEmpty(PREFIX) {
 PREFIX = /usr/local
}

#target.path = $$(PREFIX)/bin/
target.path = /usr/local/bin/
INSTALLS += target 

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += debug
CONFIG += c++11
CONFIG += link_pkgconfig
PKGCONFIG += sndfile vorbisfile mad taglib
LIBS += $$TOP_DIR/ext/yaml-cpp-0.5.1/build/libyaml-cpp.a -llo
//LIBS += -lboost_program_options-mt -lboost_filesystem-mt -lboost_regex-mt -lboost_system-mt 
RESOURCES = $$TOP_DIR/gui.qrc $$TOP_DIR/db.qrc

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
    mixerpanelwaveformsview.cpp \
    audiolevelview.cpp \
    workfiltermodel.cpp \
    workfiltermodelcollection.cpp \
    renameabletabwidget.cpp \
    midirouter.cpp \
    tagmodel.cpp \
    historymanager.cpp \
    audiofiletag.cpp \
    beatextractor.cpp \
    audiofileinfoextractor.cpp \
    audio/xing.c \
    loopandjumpcontrolview.cpp \
    loopandjumpmanager.cpp

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
    audio/doublelinkedlist.h \
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
    mixerpanelwaveformsview.h \
    audiolevelview.h \
    workfiltermodel.hpp \
    workfiltermodelcollection.hpp \
    renameabletabwidget.h \
    midirouter.h \
    tagmodel.h \
    historymanager.h \
    audiofiletag.h \
    beatextractor.h \
    audiofileinfoextractor.h \
    audio/xing.h \
    loopandjumpcontrolview.h \
    loopandjumpmanager.h

FORMS    += mainwindow.ui \
    playerview.ui \
    mixerpanelview.ui \
    workdetailview.ui \
    tagsview.ui \
    workfilterview.ui \
    loopandjumpcontrolview.ui

INCLUDEPATH += . \
	audio \
	../ext/jackcpp/include/ \
  ../ext/yaml-cpp-0.5.1/include/ \
  ../ext

