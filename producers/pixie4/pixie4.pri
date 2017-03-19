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

QMAKE_CLEAN += PIXIEmsg*.txt

#not interested in warnings on PLX and XIA code
QMAKE_CFLAGS_DEBUG   += -O0 -w
QMAKE_CFLAGS_RELEASE += -w

unix {
  ARCH = $$system(uname -m)
  DEFINES += "XIA_LINUX"
  DEFINES += "PLX_LINUX"

  !android {
    QMAKE_CC = g++
  }
      
  contains ( ARCH, x86_64): {
    LIBS += $$PWD/PLX/Library/64bit/PlxApi.a
  }

  contains ( ARCH, i686): {
    LIBS += $$PWD/PLX/Library/32bit/PlxApi.a
  }
   
  LIBS += -ldl
  QMAKE_CFLAGS += -fpermissive -DBOOST_LOG_DYN_LINK
}

win32 {
  LIBS += $$PWD/PLX/Library/PlxApi.lib
}	


INCLUDEPATH += \
  $$PWD \
  $$PWD/XIA \
  $$PWD/PLX/Include

SOURCES += \
  $$files($$PWD/*.cpp) \
  $$files($$PWD/XIA/*.c)

HEADERS += \
  $$files($$PWD/*.h) \
  $$files($$PWD/XIA/*.h) \
  $$files($$PWD/PLX/Include/*.h)
