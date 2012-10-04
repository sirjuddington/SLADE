# - Find FreeImage
# Find the native FreeImage includes and library
#
#  FREEIMAGE_INCLUDE_DIR - where to find FreeImage.h
#  FREEIMAGE_LIBRARIES   - List of libraries when using FreeImage.
#  FREEIMAGE_FOUND       - True if FreeImage found.


IF (FREEIMAGE_INCLUDE_DIR AND FREEIMAGE_LIBRARIES)
  # Already in cache, be silent
  SET(FreeImage_FIND_QUIETLY TRUE)
ENDIF (FREEIMAGE_INCLUDE_DIR AND FREEIMAGE_LIBRARIES)

FIND_PATH(FREEIMAGE_INCLUDE_DIR FreeImage.h)

FIND_LIBRARY(FREEIMAGE_LIBRARIES NAMES freeimage )
MARK_AS_ADVANCED( FREEIMAGE_LIBRARIES FREEIMAGE_INCLUDE_DIR )

# handle the QUIETLY and REQUIRED arguments and set FREEIMAGE_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FreeImage DEFAULT_MSG FREEIMAGE_LIBRARIES FREEIMAGE_INCLUDE_DIR)

