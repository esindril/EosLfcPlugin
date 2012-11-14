# Try to find LFC
# Once done, this will define
#
# LFC_FOUND       - system has LFC
# LFC_INCLUDE_DIR - the LFC include directory
# LFC_LIB_DIR     - the LFC library directory
#
# LFC_DIR may be defined as a hint for where to look

FIND_PATH(LFC_INCLUDE_DIR lfc_api.h
  HINTS
  ${LFC_DIR}
  $ENV{LFC_DIR}
  /usr
  /usr/local
  /opt/xrootd/
  PATH_SUFFIXES include/lfc

)

FIND_LIBRARY(LFC_LIB lfc
  HINTS
  ${LFC_DIR}
  $ENV{LFC_DIR}
  /usr
  /usr/local
  PATH_SUFFIXES lib46
)

GET_FILENAME_COMPONENT( LFC_LIB_DIR ${LFC_LIB} PATH )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(lfc DEFAULT_MSG LFC_LIB_DIR LFC_INCLUDE_DIR )
