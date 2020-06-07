# - Find newt
# Find the native newt headers and libraries.
#
# NEWT_INCLUDE_DIRS	- where to find newt.h, etc.
# NEWT_LIBRARIES	- List of libraries when using newt.
# NEWT_FOUND	- True if newt found.

# Look for the header file.
FIND_PATH(NEWT_INCLUDE_DIR NAMES newt.h)

# Look for the library.
FIND_LIBRARY(NEWT_LIBRARY NAMES newt)

# Handle the QUIETLY and REQUIRED arguments and set NEWT_FOUND to TRUE if all listed variables are TRUE.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NEWT DEFAULT_MSG NEWT_LIBRARY NEWT_INCLUDE_DIR)

# Copy the results to the output variables.
IF(NEWT_FOUND)
    SET(NEWT_LIBRARIES ${NEWT_LIBRARY})
    SET(NEWT_INCLUDE_DIRS ${NEWT_INCLUDE_DIR})
ELSE(NEWT_FOUND)
    SET(NEWT_LIBRARIES)
    SET(NEWT_INCLUDE_DIRS)
ENDIF(NEWT_FOUND)
