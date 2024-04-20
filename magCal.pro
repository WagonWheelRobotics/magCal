QT += core gui gui-private
QT += network
QT += serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

#DEFINES += USE_MAP_VIEW
DEFINES += USE_3D_VIEW      # if you remove 3D view, you must edit mainwindow.ui to remove entity tree
DEFINES += USE_PLOT_VIEW
#DEFINES += USE_IMAGE_VIEW


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


#DEFINES += EXAMPLE_CODE_IMAGE=\\\"c:/work/test.png\\\"  # effective when USE_IMAGE_VIEW is defined

#DEFINES += EXAMPLE_CODE_3D  # effective when USE_3D_VIEW is defined

#DEFINES += EXAMPLE_CODE_QCP # effective when USE_PLOT_VIEW is defined
#DEFINES += EXAMPLE_CODE_QCP_STATIC_PLOT    # comment out for realtime

# EDL (Part of Cloud compare) is GPL, effective when USE_3D_VIEW is defined
DEFINES += USE_EDL

# QCustomPlot is GPL, effective when USE_PLOT_VIEW is defined
DEFINES += USE_QCUSTOMPLOT

#QGeoView is LGPL-3.0, effective when USE_MAP_VIEW is defined
DEFINES += USE_QGEOVIEW

include(thirdParty/ceres/ceres.pri)
include(utils/utils.pri)
contains(DEFINES,USE_3D_VIEW):include(glView/glView.pri)
contains(DEFINES,USE_IMAGE_VIEW):include(imageView/imageView.pri)
contains(DEFINES,USE_PLOT_VIEW):include(plotView/plotView.pri)
contains(DEFINES,USE_MAP_VIEW):include(mapView/mapView.pri)

SOURCES += \
    calibOptionsDialog.cpp \
    main.cpp \
    MainWindow.cpp \
    solver.cpp

HEADERS += \
    MainWindow.h \
    calibOptionsDialog.h

FORMS += \
    MainWindow.ui \
    calibOptionsDialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

VERSION = 1.0

DEFINES += APP_NAME=\\\"$$TARGET\\\"
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
DEFINES += APP_VERSION_QT=\\\"$$QT_VERSION\\\"
DEFINES += "COMPANY_NAME=\"\\\"Wagon Wheel Robotics\\\"\""
DEFINES += "COPYRIGHT_YEAR=\"\\\"2021\\\"\""
