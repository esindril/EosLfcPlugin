# ----------------------------------------------------------------------
# File: CMakeLists.txt
# Author: Elvin-Alin Sindrilaru - CERN
# ----------------------------------------------------------------------

# ************************************************************************
# * EOS - the CERN Disk Storage System                                   *
# * Copyright (C) 2011 CERN/Switzerland                                  *
# *                                                                      *
# * This program is free software: you can redistribute it and/or modify *
# * it under the terms of the GNU General Public License as published by *
# * the Free Software Foundation, either version 3 of the License, or    *
# * (at your option) any later version.                                  *
# *                                                                      *
# * This program is distributed in the hope that it will be useful,      *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
# * GNU General Public License for more details.                         *
# *                                                                      *
# * You should have received a copy of the GNU General Public License    *
# * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
# ************************************************************************

include_directories( ./  ${LFC_INCLUDE_DIR} ${XROOTD_INCLUDE_DIR} )
link_directories( ${LFC_LIB_DIR} )

add_library( EosLfcPlugin MODULE
	     LfcString.cc            LfcString.hh
	     LfcCache.cc             LfcCache.hh
	     EosLfcPlugin.cc         EosLfcPlugin.hh
	     )

add_library( EosLfcOfsPlugin MODULE 
	     EosLfcOfsPlugin.cc         EosLfcOfsPlugin.hh
)  

target_link_libraries( EosLfcPlugin ${LFC_LIB} )
target_link_libraries( EosLfcOfsPlugin XrdOfs XrdServer )

if (Linux)
  set_target_properties ( EosLfcPlugin EosLfcOfsPlugin PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
  )
endif(Linux)

install ( TARGETS EosLfcPlugin EosLfcOfsPlugin
          LIBRARY DESTINATION ${LIB_INSTALL_DIR}
          ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

