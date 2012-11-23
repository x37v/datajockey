######################################################################
# Automatically generated by qmake (2.01a) Sun Feb 19 14:48:57 2012
######################################################################

TEMPLATE = app
TARGET = datajockey
LIBS += -lsndfile -lvorbisfile -lmad -ljack -llilv-0 -lyaml-cpp -loscpack -lboost_regex-mt -ltag -lvamp-hostsdk -lboost_program_options-mt -lboost_filesystem-mt
DENORMAL_FLAGS = -msse -mfpmath=sse -ffast-math
QT += dbus sql opengl

VERSION = 0.2.git

CONFIG += DEBUG
//CONFIG += RELEASE

#profiling:
#QMAKE_CXXFLAGS_DEBUG += -pg
#QMAKE_LFLAGS_DEBUG += -pg

DEPENDPATH += . \
              include/ \
              include/audio \
              include/controller \
              include/view \
              include/old \
              include/util \
              ext/jackcpp/include/ \
              src/ \
              src/audio \
              src/controller \
              src/view \
              src/old \
              src/util \
              ext/jackcpp/src/
MOC_DIR = moc/
OBJECTS_DIR = obj/
INCLUDEPATH += . \
   include/ \
   include/audio \
   include/controller \
   include/model \
   include/view \
   include/old \
   include/util \
   ext/jackcpp/include/ \
	/usr/include/rasqal/ \
   /usr/local/include/oscpack/ \
   /usr/local/include/lilv-0/

QMAKE_CXXFLAGS_DEBUG += -DDEBUG

QMAKE_CXXFLAGS += $$DENORMAL_FLAGS -fexceptions -DDJ_VERSION=$$VERSION
QMAKE_CFLAGS += $$DENORMAL_FLAGS -fexceptions
RESOURCES = gui.qrc db.qrc

# Input
HEADERS += include/application.hpp \
           include/config.hpp \
           include/defines.hpp \
           include/audio/annotation.hpp \
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
           include/audio/stretcherrate.hpp \
           include/audio/timepoint.hpp \
           include/audio/transport.hpp \
           include/audio/types.hpp \
           include/controller/loaderthread.hpp \
           include/controller/midimapper.hpp \
           include/controller/oscsender.hpp \
           include/model/audiomodel.hpp \
           include/model/db.hpp \
           include/model/workrelationmodel.hpp \
           include/util/audiofiletag.hpp \
           include/util/beatextractor.hpp \
           include/util/importer.hpp \
           include/view/appmainwindow.hpp \
           include/view/audiolevel.hpp \
           include/view/midimappingview.hpp \
           include/view/mixerpanel.hpp \
           include/view/playerview.hpp \
           include/view/waveformviewgl.hpp \
           include/old/defaultworkfilters.hpp \
           include/old/oscreceiver.hpp \
           include/old/tageditor.hpp \
           include/old/tagmodel.hpp \
           include/old/tagview.hpp \
           include/old/treeitem.h \
           include/old/treemodel.h \
           include/old/workdbview.hpp \
           include/old/workdetailview.hpp \
           include/old/workfilterlist.hpp \
           include/old/workfilterlistview.hpp \
           include/old/workfiltermodel.hpp \
           #include/old/workpreviewer.hpp \
           include/old/worktablemodel.hpp \
           include/old/worktagmodelfilter.hpp \
           ext/jackcpp/include/jackaudioio.hpp \
           ext/jackcpp/include/jackmidiport.hpp \
           ext/jackcpp/include/jackringbuffer.hpp \

SOURCES += src/application.cpp \
           src/config.cpp \
           src/defines.cpp \
           src/audio/annotation.cpp \
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
           src/audio/stretcherrate.cpp \
           src/audio/timepoint.cpp \
           src/audio/transport.cpp \
           src/controller/loaderthread.cpp \
           src/controller/midimapper.cpp \
           src/controller/oscsender.cpp \
           src/model/audiomodel.cpp \
           src/model/db.cpp \
           src/model/workrelationmodel.cpp \
           src/util/audiofiletag.cpp \
           src/util/beatextractor.cpp \
           src/util/importer.cpp \ 
           src/view/appmainwindow.cpp \
           src/view/audiolevel.cpp \
           src/view/midimappingview.cpp \
           src/view/mixerpanel.cpp \
           src/view/playerview.cpp \
           src/view/waveformviewgl.cpp \
           src/old/defaultworkfilters.cpp \
           src/old/oscreceiver.cpp \
           src/old/tageditor.cpp \
           src/old/tagmodel.cpp \
           src/old/tagview.cpp \
           src/old/treeitem.cpp \
           src/old/treemodel.cpp \
           src/old/workdbview.cpp \
           src/old/workdetailview.cpp \
           src/old/workfilterlist.cpp \
           src/old/workfilterlistview.cpp \
           src/old/workfiltermodel.cpp \
           #src/old/workpreviewer.cpp \
           src/old/worktablemodel.cpp \
           src/old/worktagmodelfilter.cpp \
           ext/jackcpp/src/jackaudioio.cpp \
           ext/jackcpp/src/jackmidiport.cpp

