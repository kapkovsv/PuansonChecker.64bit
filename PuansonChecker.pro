#-------------------------------------------------
#
# Project created by QtCreator 2017-11-13T11:23:44
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT       += concurrent xml

TARGET = PuansonChecker
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    currentform.cpp \
    puansonchecker.cpp \
    contoursform.cpp \
    imagewindow.cpp \
    settingsdialog.cpp \
    photocamera.cpp \
    nikon/CallBack.cpp \
    nikon/Function.cpp \
    puansonimage.cpp \
    generalsettings.cpp


HEADERS  += mainwindow.h \
    currentform.h \
    puansonchecker.h \
    contoursform.h \
    imagewindow.h \
    settingsdialog.h \
    photocamera.h \
    nikon/CtrlSample.h \
    nikon/Maid3.h \
    nikon/Maid3d1.h \
    nikon/NkEndian.h \
    nikon/Nkstdint.h \
    nikon/NkTypes.h \
    puansonimage.h \
    generalsettings.h

FORMS    += mainwindow.ui \
    currentform.ui \
    contoursform.ui \
    settingsdialog.ui

#DEFINES += QT_NO_DEBUG_OUTPUT

DEFINES += __LITTLE_ENDIAN__ WIN32 _CRT_SECURE_NO_WARNINGS NDEBUG _CONSOLE _WINDOWS
DEFINES -= UNICODE

INCLUDEPATH += C:\opencv\opencv\build\install\include
INCLUDEPATH += C:\Users\1\Documents\LibRaw-0.18.5\LibRaw-0.18.5

LIBS += C:\Users\1\Documents\LibRaw-0.18.5\LibRaw-0.18.5\lib\libraw.a
LIBS += C:\opencv\opencv\build\install\x86\mingw\bin\libopencv_core330.dll
LIBS += C:\opencv\opencv\build\install\x86\mingw\bin\libopencv_highgui330.dll
LIBS += C:\opencv\opencv\build\install\x86\mingw\bin\libopencv_imgcodecs330.dll
LIBS += C:\opencv\opencv\build\install\x86\mingw\bin\libopencv_imgproc330.dll
LIBS += -lws2_32 -lwinmm
# -lodbc32 -lodbccp32

#LIBS += C:\Users\1\Documents\PuansonChecker\NkdPTP.dll
