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
   LIBS += -lm -ldl -lz \
           -lboost_system -lboost_date_time -lboost_thread -lboost_log \
           -lboost_filesystem -lboost_log_setup -lboost_timer -lboost_regex

  contains( DAQ_SOURCES, fitter_ceres ) {
    LIBS += -lgflags -lpthread -lglog -lspqr -lcholmod -lccolamd -lcamd -lcolamd -lamd -llapack -lf77blas -latlas \
           -lceres -lgomp -lglog -lcholmod -lrt -llapack -lf77blas -latlas

  }

}

	 
INCLUDEPATH += $$PWD \
               $$PWD/spectrum \
               $$PWD/spectrum/xylib \
               $$PWD/pugixml \
               $$PWD/math \
               /usr/include/eigen3

SOURCES += $$files($$PWD/*.cpp) \
           $$files($$PWD/spectrum/*.cpp) \
           $$files($$PWD/spectrum/xylib/*.cpp) \
           $$files($$PWD/pugixml/*.cpp) \
           $$files($$PWD/math/*.cpp)

HEADERS  += $$files($$PWD/*.h) \
            $$files($$PWD/spectrum/*.h) \
            $$files($$PWD/spectrum/xylib/*.h) \
            $$files($$PWD/pugixml/*.hpp) \
            $$files($$PWD/math/*.h)

include($$PWD/fitting/qpx_fitting.pri)
