# - Find FTGL
# Find the native FTGL includes and library
#
#  FTGL_INCLUDE_DIR - where to find FTGL/ftgl.h
#  FTGL_LIBRARIES   - List of libraries when using ftgl.
#  FTGL_FOUND       - True if ftgl found.

IF (FTGL_INCLUDE_DIR AND FTGL_LIBRARIES)
  # Already in cache, be silent
  SET(FTGL_FIND_QUIETLY TRUE)
ENDIF (FTGL_INCLUDE_DIR AND FTGL_LIBRARIES)

FIND_PATH(FTGL_INCLUDE_DIR FTGL/ftgl.h)

FIND_LIBRARY(FTGL_LIBRARIES NAMES ftgl )
MARK_AS_ADVANCED( FTGL_LIBRARIES FTGL_INCLUDE_DIR )

# handle the QUIETLY and REQUIRED arguments and set FTGL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FTGL DEFAULT_MSG FTGL_LIBRARIES FTGL_INCLUDE_DIR)

