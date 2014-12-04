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
RESOURCES = $$TOP_DIR/db.qrc

macx {
  QMAKE_LIBDIR += ../ext/vamp/osx/
  INCLUDEPATH += ../ext/vamp/ /opt/local/include/
  LIBS += -lsndfile -lvorbisfile -lmad -ltag
  LIBS += -lvamp-hostsdk
  LIBS += -L/usr/local/lib/ -L/opt/local/lib/
  #QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.6.sdk
  #QMAKE_MAC_SDK.macosx.path = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk/
  QMAKE_MAC_SDK = macosx10.9
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
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
