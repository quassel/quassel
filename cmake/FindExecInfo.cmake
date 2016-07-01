# Set up execinfo

# The problem with this library is that it is built-in in the Linux glib,
# while on systems like FreeBSD, it is installed separately and thus needs to be linked to.
# Therefore, we search for the header to see if the it's available in the first place.
# If it is available, we try to locate the library to figure out whether it is built-in or not.

find_path(EXECINFO_INCLUDES "execinfo.h")

if(EXECINFO_INCLUDES)
  # We found the header file's include dir.

  # Now determine if it's built-in or not, by searching the library file.
  find_library(EXECINFO_LIBRARIES "execinfo")

  if(NOT EXECINFO_LIBRARIES)
    # Built-in, no further action is needed
    set(EXECINFO_LIBRARIES "")
    message(STATUS "Found execinfo (built-in)")
  else()
    # It's an external library.
    message(STATUS "Found execinfo: ${EXECINFO_LIBRARIES}")
  endif()

  set(EXECINFO_FOUND true)

endif()
