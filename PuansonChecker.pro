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

SOURCES += main.cpp \
    mainwindow.cpp \
    currentform.cpp \
    settingsdialog.cpp \
    puansonchecker.cpp \
    puansonimage.cpp \
    photocamera.cpp \
    generalsettings.cpp \
    contoursform.cpp \
    imagewindow.cpp \
    checkdetailvalidateddialog.cpp \
    etalonangleresultconfirmdialog.cpp \
    currentangleresultconfirmdialog.cpp \
    detailanglesourcedialog.cpp \
    etalonresearchpropertiesdialog.cpp \
    etalonresearchcreationdialog.cpp \
    currentresearchcreationdialog.cpp \
    currentresearchresultandprotocoldialog.cpp \
    puansonmachine.cpp \
    puansonresearch.cpp

win64 {
SOURCES += nikon/CallBack.cpp \
    nikon/Function.cpp \
    MachineController/MachineControllerImpl.cpp
}

HEADERS  += mainwindow.h \
    currentform.h \
    puansonchecker.h \
    contoursform.h \
    settingsdialog.h \
    puansonimage.h \
    photocamera.h  \
    generalsettings.h \
    imagewindow.h \
    nikon/CtrlSample.h \
    nikon/Maid3.h \
    nikon/Maid3d1.h \
    nikon/NkEndian.h \
    nikon/Nkstdint.h \
    nikon/NkTypes.h \
    types.h \
    checkdetailvalidateddialog.h \
    etalonangleresultconfirmdialog.h \
    currentangleresultconfirmdialog.h \
    detailanglesourcedialog.h \
    etalonresearchpropertiesdialog.h \
    etalonresearchcreationdialog.h \
    currentresearchcreationdialog.h \
    currentresearchresultandprotocoldialog.h \
    puansonmachine.h \
    MachineController/sys/elf32.h \
    MachineController/sys/elf_common.h \
    MachineController/MachineController.h \
    MachineController/MachineControllerImpl.h \
    MachineController/StdAfx.h \
    MachineController/STLink.h \
    MachineController/STLinkConsts.h \
    MachineController/UsbDefs.h \
    MachineController/mach_controller.h \
    puansonresearch.h

FORMS    += mainwindow.ui \
    currentform.ui \
    contoursform.ui \
    settingsdialog.ui \
    checkdetailvalidateddialog.ui \
    etalonangleresultconfirmdialog.ui \
    currentangleresultconfirmdialog.ui \
    detailanglesourcedialog.ui \
    etalonresearchpropertiesdialog.ui \
    etalonresearchcreationdialog.ui \
    currentresearchcreationdialog.ui \
    currentresearchresultandprotocoldialog.ui

win64 {
#DEFINES += QT_NO_DEBUG_OUTPUT

DEFINES += __LITTLE_ENDIAN__ WIN64 _CRT_SECURE_NO_WARNINGS NDEBUG _CONSOLE _WINDOWS
DEFINES -= UNICODE _UNICODE

INCLUDEPATH += install\include
INCLUDEPATH += LibRaw-0.18.8

LIBS += $$PWD\LibRaw-0.18.8\lib\libraw.a
LIBS += $$PWD\install\x64\mingw\bin\libopencv_core330.dll
LIBS += $$PWD\install\x64\mingw\bin\libopencv_highgui330.dll
LIBS += $$PWD\install\x64\mingw\bin\libopencv_imgcodecs330.dll
LIBS += $$PWD\install\x64\mingw\bin\libopencv_imgproc330.dll
LIBS += $$PWD\install\x64\mingw\bin\libopencv_features2d330.dll
LIBS += $$PWD\install\x64\mingw\bin\libopencv_calib3d330.dll
LIBS += -lws2_32 -lwinmm -lwinusb -lsetupapi
# -lodbc32 -lodbccp32

#LIBS += $$PWD\NkdPTP.dll
}
else: linux-g++ {
INCLUDEPATH += /usr/local/include/

LIBS += /usr/local/lib64/libraw.so.16
LIBS += /usr/local/lib/libopencv_core.so
LIBS += /usr/local/lib/libopencv_imgcodecs.so
LIBS += /usr/local/lib/libopencv_imgproc.so
LIBS += /usr/local/lib/libopencv_features2d.so
LIBS += /usr/local/lib/libopencv_calib3d.so
LIBS += /usr/local/lib/libopencv_highgui.so

DISTFILES +=
}
