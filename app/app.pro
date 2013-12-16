#-------------------------------------------------
#
# Project created by QtCreator 2013-12-15T10:41:54
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = app
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    playerview.cpp \
    mixerpanelview.cpp \
    workdetailview.cpp \
    tagsview.cpp \
    workfilterview.cpp \
    db.cpp \
    workstableview.cpp

HEADERS  += mainwindow.h \
    playerview.h \
    mixerpanelview.h \
    workdetailview.h \
    tagsview.h \
    workfilterview.h \
    db.h \
    workstableview.h

FORMS    += mainwindow.ui \
    playerview.ui \
    mixerpanelview.ui \
    workdetailview.ui \
    tagsview.ui \
    workfilterview.ui
