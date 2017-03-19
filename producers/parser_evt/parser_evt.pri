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
   LIBS += -lrt
}

DEFINES += BINDIR=\\\"/home/mgs/dev/nscldaq/nscldaq-11.0-017/base/dataflow/\\\"

INCLUDEPATH += /usr/include/tcl8.6

INCLUDEPATH += $$PWD \
		$$PWD/nscldaq \
		$$PWD/nscldaq/uri \
		$$PWD/nscldaq/os \
		$$PWD/nscldaq/libtcl \
		$$PWD/nscldaq/dataflow \
		$$PWD/nscldaq/portmanager

SOURCES += $$files($$PWD/*.cpp) \
    $$files($$PWD/nscldaq/*.c) \
    $$files($$PWD/nscldaq/*.cpp) \
		$$files($$PWD/nscldaq/uri/*.cpp) \
		$$files($$PWD/nscldaq/os/*.cpp) \
		$$files($$PWD/nscldaq/libtcl/*.cpp) \
		$$files($$PWD/nscldaq/dataflow/*.cpp) \
		$$files($$PWD/nscldaq/portmanager/*.cpp)


HEADERS  += $$files($$PWD/*.h) \
		$$files($$PWD/nscldaq/*.h) \
		$$files($$PWD/nscldaq/uri/*.h) \
		$$files($$PWD/nscldaq/os/*.h) \
		$$files($$PWD/nscldaq/libtcl/*.h) \
		$$files($$PWD/nscldaq/dataflow/*.h) \
		$$files($$PWD/nscldaq/portmanager/*.h)
