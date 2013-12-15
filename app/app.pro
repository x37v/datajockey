#-------------------------------------------------
#
# Project created by QtCreator 2013-12-15T10:41:54
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = app
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    playerview.cpp \
    mixerpanelview.cpp \
    workdetailview.cpp \
    tagsview.cpp

HEADERS  += mainwindow.h \
    playerview.h \
    mixerpanelview.h \
    workdetailview.h \
    tagsview.h

FORMS    += mainwindow.ui \
    playerview.ui \
    mixerpanelview.ui \
    workdetailview.ui \
    tagsview.ui
