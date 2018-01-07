QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += static

TARGET = Memory
TEMPLATE = app

SOURCES += main.cpp\
           memory.cpp \
           memoryview.cpp \
           tile.cpp \
           MemoryAI.cpp \
           newgamedialog.cpp \
           MTriple.cpp \
           tileimagehandler.cpp

HEADERS  += memory.h \
    memoryview.h \
    tile.h \
    MemoryAI.h \
    newgamedialog.h \
    tileimagehandler.h 

RESOURCES = memoryrc.qrc

RC_FILE = memoryicon.rc
