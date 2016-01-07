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


BADDIRS = "debug release ui"

UI_DIR = $$PWD/ui

QMAKE_CLEAN += -r $$BADDIRS \
               $$files(qpx*.log) \
               $$files(qpx.pro.user*) \
               install

CONFIG -= warn_off warn_on
CONFIG += static

QMAKE_CFLAGS_RELEASE += -w
QMAKE_CXXFLAGS_DEBUG += -O0 -Wextra

QMAKE_CXXFLAGS += -DBOOST_LOG_DYN_LINK

unix {
  !android {
     QMAKE_CC = g++
  }
      
  target.path = /usr/local/bin/
  icon.path = /usr/share/icons/
  desktop.path = /usr/share/applications/

  LIBPATH += /usr/local/lib

  !mac {
    CONFIG -= c++11
    QMAKE_CXXFLAGS += -std=c++11
  }
}

android {
         INCLUDEPATH += ../crystax-ndk-10.2.1/sources/boost/1.58.0/include
         equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
             LIBPATH += ../crystax-ndk-10.2.1/sources/boost/1.58.0/libs/armeabi-v7a
            }
        }

mac {
     CONFIG += c++11
     QMAKE_LFLAGS += -lc++
     LIBPATH += /usr/local/boost_1_59_0/stage/lib /opt/local/lib
     INCLUDEPATH += /usr/local/boost_1_59_0
     QMAKE_CXXFLAGS += -stdlib=libc++ -Wno-c++11-narrowing
}

win32 {
       LIBPATH += D:\dev\boost_1_57_0\stage\lib
       INCLUDEPATH += D:/dev/boost_1_57_0
      }	

! include( $$PWD/engine/engine.pri ) {
    error( "Couldn't find the engine.pri file!" )
}

! include( $$PWD/hardware/hardware.pri ) {
    error( "Couldn't find the harware.pri file!" )
}
