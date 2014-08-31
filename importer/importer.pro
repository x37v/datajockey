#-------------------------------------------------
#
# Project created by QtCreator 2014-03-02T15:43:41
#
#-------------------------------------------------

QT       += core sql

QT       -= gui

TARGET = importer
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += debug
CONFIG += c++11
CONFIG += link_pkgconfig

TEMPLATE = app


LIBS += $$TOP_DIR/ext/yaml-cpp-0.5.1-build/libyaml-cpp.a
INCLUDEPATH += /usr/local/include/

macx {
  QMAKE_LIBDIR += ../ext/vamp/osx/
  LIBS += -lvamp-hostsdk
  #QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.6.sdk
  INCLUDEPATH += ../ext/vamp/
  LIBS += -lsndfile -lvorbisfile -lmad -ltag
  LIBS += -L/usr/local/lib/
}

unix:!macx {
  PKGCONFIG += sndfile vorbisfile mad taglib
  PKGCONFIG += vamp-hostsdk
}

DENORMAL_FLAGS = -msse -mfpmath=sse -ffast-math
QMAKE_CXXFLAGS += $$DENORMAL_FLAGS -fexceptions -DDJ_VERSION=$$VERSION

MOC_DIR = moc/
OBJECTS_DIR = obj/

SOURCES += main.cpp \
    ../app/audiofileinfoextractor.cpp \
    ../app/audiofiletag.cpp \
    ../app/beatextractor.cpp \
    ../app/config.cpp \
    ../app/defines.cpp \
    ../app/audio/annotation.cpp \
    ../app/audio/audiobuffer.cpp \
    ../app/audio/soundfile.cpp \
    fileprocessor.cpp \
    ../app/db.cpp \
    ../app/audio/xing.c

HEADERS += \
    ../app/audiofileinfoextractor.h \
    ../app/audiofiletag.h \
    ../app/beatextractor.h \
    ../app/config.hpp \
    ../app/defines.hpp \
    ../app/audio/annotation.hpp \
    ../app/audio/audiobuffer.hpp \
    ../app/audio/soundfile.hpp \
    fileprocessor.h \
    ../app/db.h \
    ../app/audio/xing.h

INCLUDEPATH += . \
  ../app/ \
  ../app/audio \
  ../ext/yaml-cpp-0.5.1/include/
