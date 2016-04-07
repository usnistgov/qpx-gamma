#-------------------------------------------------------------------------------
#
#  This software was developed at the National Institute of Standards and
#  Technology (NIST) by employees of the Federal Government in the course
#  of their official duties. Pursuant to title 17 Section 105 of the
#  United States Code, this software is not subject to copyright protection
#  and is in the public domain. NIST assumes no responsibility whatsoever for
#  its use by other parties, and makes no guarantees, expressed or implied,
#  about its quality, reliability, or any other characteristic.
# 
#  This software can be redistributed and/or modified freely provided that
#  any derivative works bear some notice that they are derived from it, and
#  any modified versions bear some notice that they have been modified.
#
#  Author(s):
#       Martin Shetty (NIST)
#
#  Description:
#       Project file for qpx-gamma
# 
#-------------------------------------------------------------------------------

CONFIG += debug_and_release

! include( $$PWD/../common.pri ) {
    error( "Couldn't find the common.pri file!" )
}

TARGET   = $$PWD/../qpx
TEMPLATE = app

CONFIG(debug, debug|release) {
   TARGET = $$join(TARGET,,,d)
   DEFINES += "QPX_DBG_"
}

INSTALLS += target icon desktop

QT += core gui multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

unix {
   LIBS += -lm -ldl -lz
}

INCLUDEPATH += $$PWD \
               $$PWD/daq \
               $$PWD/analysis \
               $$PWD/plots \
               $$PWD/plots/qcustomplot \
               $$PWD/widgets \
               $$PWD/widgets/qtcolorpicker

SOURCES += $$files($$PWD/*.cpp) \
           $$files($$PWD/daq/*.cpp) \
           $$files($$PWD/analysis/*.cpp) \
           $$files($$PWD/plots/*.cpp) \
           $$files($$PWD/plots/qcustomplot/*.cpp) \
           $$files($$PWD/widgets/*.cpp) \
           $$files($$PWD/widgets/qtcolorpicker/*.cpp)

HEADERS  += $$files($$PWD/*.h) \
            $$files($$PWD/daq/*.h) \
            $$files($$PWD/analysis/*.h) \
            $$files($$PWD/plots/*.h) \
            $$files($$PWD/plots/qcustomplot/*.h) \
            $$files($$PWD/widgets/*.h) \
            $$files($$PWD/widgets/qtcolorpicker/*.h)

FORMS   += $$files($$PWD/*.ui) \
           $$files($$PWD/plots/*.ui) \
           $$files($$PWD/analysis/*.ui) \
           $$files($$PWD/widgets/*.ui) \
           $$files($$PWD/daq/*.ui)

RESOURCES += $$files($$PWD/resources/*.qrc)

DISTFILES +=
