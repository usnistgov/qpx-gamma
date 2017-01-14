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
#       Project include file for fitting functions
# 
#-------------------------------------------------------------------------------

unix {
   LIBS += -lm -ldl -lz

  !mac {
    LIBS += -lboost_system
  }

  mac {
    LIBS += -lboost_system-mt
  }
}

!CONFIG(debug, debug|release) {
   DEFINES += "NDEBUG"
}

INCLUDEPATH += $$PWD

SOURCES += $$files($$PWD/*.cpp)

HEADERS  += $$files($$PWD/*.h)
