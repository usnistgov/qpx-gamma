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
#       Project file for cpx
# 
#-------------------------------------------------------------------------------

CONFIG += debug_and_release

! include( $$PWD/../common.pri ) {
    error( "Couldn't find the common.pri file!" )
}

TARGET   = $$PWD/../cpx
TEMPLATE = app

INSTALLS += target

CONFIG(debug, debug|release) {
   TARGET = $$join(TARGET,,,d)
   DEFINES += "QPX_DBG_"
}
	 
INCLUDEPATH += $$PWD

SOURCES += $$PWD/cpx.cpp

HEADERS += $$PWD/cpx.h
