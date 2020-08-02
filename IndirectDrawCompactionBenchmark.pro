TEMPLATE = app
TARGET = IndirectDrawCompactionBenchmark
INCLUDEPATH += .
QT =
CONFIG += link_pkgconfig
PKGCONFIG += sdl2 glew
QMAKE_CFLAGS += -Wno-padded

# Input
SOURCES += main.c
