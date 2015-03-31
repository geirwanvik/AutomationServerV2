#-------------------------------------------------
#
# Project created by QtCreator 2015-03-07T17:06:37
#
#-------------------------------------------------

QT       += core sql network
QT       -= gui

TARGET = AutomationServerV2

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    manager.cpp \
    telldusapi.cpp \
    tcpserver.cpp

HEADERS += \
    manager.h \
    telldusapi.h \
    singleton.h \
    common.h \
    tcpserver.h

LIBS    += "/usr/local/lib/libtelldus-core.so"

