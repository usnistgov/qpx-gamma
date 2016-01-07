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

unix {

   LIBS += -lm -ldl -DBOOST_LOG_DYN_LINK \
           -lboost_system -lboost_date_time -lboost_thread -lboost_log \
           -lboost_program_options -lboost_filesystem \
           -lboost_log_setup -lboost_timer -lz

   QMAKE_CFLAGS   += -fpermissive

   LIBPATH += /usr/local/lib
}


INCLUDEPATH += $$PWD/../engine \
               $$PWD/../engine/pugixml \
               $$PWD/../engine/math

HEADERS  += $$files($$PWD/../engine/*.h) \
            $$files($$PWD/../engine/pugixml/*.h) \
            $$files($$PWD/../engine/math/*.h)

!mac {
  ! include( $$PWD/pixie4/pixie4.pri ) {
      error( "Couldn't find the pixie4.pri file!" )
  }

  ! include( $$PWD/sorter_evt/sorter_evt.pri ) {
     error( "Couldn't find the sorter_evt.pri file!" )
  }
}

! include( $$PWD/VME/vme.pri ) {
    error( "Couldn't find the vme.pri file!" )
}

! include( $$PWD/HV8/hv8.pri ) {
    error( "Couldn't find the hv8.pri file!" )
}

! include( $$PWD/sorter/sorter.pri ) {
    error( "Couldn't find the sorter.pri file!" )
}

! include( $$PWD/simulator2d/simulator2d.pri ) {
    error( "Couldn't find the simulator2d.pri file!" )
}
