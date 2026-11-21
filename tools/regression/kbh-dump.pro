# Entwicklerwerkzeug: Regressionsvergleich der Berechnungen
# Nicht Teil des Programms/Installers.
# Build: tools\regression\build.cmd <qt-bin>  (baut Core-Lib + kbh-dump)

QT += core sql xml
QT -= gui
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = kbh-dump

# Core-Lib aus dem eigenen Build (core.pro-Unterprojekt legt sie in ../bin ab)
CORE_LIB_DIR = $$OUT_PWD/../bin
# Ausgabe neben die App, dort liegen die Qt-DLLs/sqldrivers
isEmpty(DEST): DEST = $$PWD/../../build-win64/bin
DESTDIR = $$DEST

INCLUDEPATH += $$PWD/../../kleiner-brauhelfer-core
DEFINES += KLEINERBRAUHELFERCORE_LIBRARY_STATIC
LIBS += -L$$CORE_LIB_DIR -lkleiner-brauhelfer-core
win32: PRE_TARGETDEPS += $$CORE_LIB_DIR/kleiner-brauhelfer-core.lib
unix: PRE_TARGETDEPS += $$CORE_LIB_DIR/libkleiner-brauhelfer-core.a

SOURCES += main.cpp
