#-------------------------------------------------
#
# Project created by QtCreator 2019-07-18T13:46:19
#
#-------------------------------------------------

QT       += core gui
QT       += serialport
QT       += printsupport
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtSerialMonitor
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += QCUSTOMPLOT_USE_OPENGL

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        codeeditor.cpp \
        filereader.cpp \
        highlighter.cpp \
        infodialog.cpp \
        logger.cpp \
        main.cpp \
        mainwindow.cpp \
        networkudp.cpp \
        parser.cpp \
        qcustomplot.cpp \
        serial.cpp

HEADERS += \
        ../../../../../../Downloads/QCustomPlot.tar/qcustomplot/qcustomplot.h \
        codeeditor.h \
        config.h \
        filereader.h \
        highlighter.h \
        infodialog.h \
        logger.h \
        mainwindow.h \
        networkudp.h \
        parser.h \
        qcustomplot.h \
        serial.h

FORMS += \
        infodialog.ui \
        mainwindow.ui

win32:RC_ICONS += QtSM.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    QtSM.ico \
    TODO.txt

RESOURCES += \
    3dres.qrc

