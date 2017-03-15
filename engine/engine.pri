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
#       Project include file file for engine
# 
#-------------------------------------------------------------------------------

unix {
  LIBS += -lm -ldl -lz

  !mac {
    LIBS += -lboost_system -lboost_date_time -lboost_thread -lboost_log \
            -lboost_filesystem -lboost_log_setup -lboost_timer -lboost_regex
  }

  mac {
    LIBS += -lboost_system-mt -lboost_date_time-mt -lboost_thread-mt -lboost_log-mt \
            -lboost_filesystem-mt -lboost_log_setup-mt -lboost_timer-mt -lboost_regex-mt
  }
}

contains( DAQ_SOURCES, hdf5 ) {
  DEFINES += "H5_ENABLED"
unix {
#  LIBPATH += /usr/local/lib
  LIBS += -lhdf5_cpp

  !mac {
    H5SERIAL = $$system(ldconfig -p | grep hdf5_serial)
  }

  contains (H5SERIAL, libhdf5_serial.so) {
    LIBS += -lhdf5_serial -lhdf5_serial_hl
    INCLUDEPATH += /usr/include/hdf5/serial
  }

  !contains (H5SERIAL, libhdf5_serial.so) {
    LIBS += -lhdf5 -lhdf5_hl_cpp
}
}
INCLUDEPATH += $$PWD/h5 \
               $$PWD/h5/h5cc

SOURCES += $$files($$PWD/h5/*.cpp) \
           $$files($$PWD/h5/h5cc/*.cpp)

HEADERS  += $$files($$PWD/h5*.h) \
            $$files($$PWD/h5/h5cc/*.h)
}

	 
INCLUDEPATH += $$PWD \
               $$PWD/pugixml \
               $$PWD/math \
               /usr/include/eigen3

SOURCES += $$files($$PWD/*.cpp) \
           $$files($$PWD/pugixml/*.cpp) \
           $$files($$PWD/math/*.cpp)

HEADERS  += $$files($$PWD/*.h) \
            $$files($$PWD/*.hpp) \
            $$files($$PWD/pugixml/*.hpp) \
            $$files($$PWD/math/*.h)

include($$PWD/fitting/qpx_fitting.pri)
