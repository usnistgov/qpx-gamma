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

TARGET   = cpx
TEMPLATE = app

CONFIG += debug_and_release c++11

INSTALLS += target

CONFIG(debug, debug|release) {
   TARGET = $$join(TARGET,,,d)
   DEFINES += "CPX_DBG_"
}


CONFIG -= warn_off warn_on

#not interested in warnings on PLX and XIA code
QMAKE_CFLAGS_DEBUG += -O0 -w
QMAKE_CXXFLAGS_DEBUG += -O0 -Wextra

QMAKE_CFLAGS_RELEASE += -w

QMAKE_CXXFLAGS  += -DBOOST_LOG_DYN_LINK

unix {
       DEFINES += "XIA_LINUX"
       DEFINES += "PLX_LINUX"
       QMAKE_CC = g++
       LIBS += dependencies/PLX/PlxApi.a -lm -ldl -DBOOST_LOG_DYN_LINK \
               -lboost_system -lboost_date_time -lboost_thread -lboost_log \
               -lboost_program_options -lboost_filesystem \
               -lboost_log_setup -lboost_timer

	target.path = /usr/local/bin/
		
	LIBPATH += /usr/local/lib
		
	QMAKE_CFLAGS   += -fpermissive
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3
   }
	 
INCLUDEPATH += pixiecpp \
               dependencies \
               dependencies/PLX \
               dependencies/XIA \
               dependencies/xylib \
               dependencies/tinyxml2

SOURCES += cpx.cpp \
           dependencies/custom_logger.cpp \
           dependencies/isotope.cpp \
           $$files(dependencies/XIA/*.c) \
           $$files(dependencies/tinyxml2/*.cpp) \
           $$files(dependencies/xylib/*.cpp) \
           $$files(pixiecpp/*.cpp)

HEADERS  += dependencies/custom_logger.h \
            dependencies/isotope.h \
            $$files(dependencies/XIA/*.h) \
            $$files(dependencies/PLX/*.h) \
            $$files(dependencies/tinyxml2/*.h) \
            $$files(dependencies/xylib/*.h) \
            $$files(pixiecpp/*.h)
