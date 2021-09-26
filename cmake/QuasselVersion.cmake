# Set up version-related information
###############################################################################

# Quassel version
set(QUASSEL_MAJOR  0)
set(QUASSEL_MINOR 13)
set(QUASSEL_PATCH 92)
set(QUASSEL_VERSION_STRING "0.14-rc2")

# Get additional version information from Git
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_HEAD)
git_describe(GIT_DESCRIBE --long)

# If in a Git repo we can get the commit-date from a git command
if (GIT_HEAD)
    find_program(GIT_COMMAND NAMES git)
    if (GIT_COMMAND)
        execute_process(
            COMMAND ${GIT_COMMAND} -c log.showsignature=false show -s --format=%ct
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_DATE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
endif()

# If not in a Git repo try to read GIT_HEAD and GIT_DESCRIBE from enviroment
if (NOT GIT_HEAD OR NOT GIT_DESCRIBE)
  if (DEFINED ENV{GIT_HEAD})
      set(GIT_HEAD $ENV{GIT_HEAD})
  endif()
  if (DEFINED ENV{GIT_DESCRIBE})
     set(GIT_DESCRIBE $ENV{GIT_DESCRIBE})
  endif()
endif()

# Sanitize things if we're not in a Git repo
if (NOT GIT_HEAD OR NOT GIT_DESCRIBE)
    set(GIT_HEAD "")
    set(GIT_DESCRIBE "")
    set(GIT_COMMIT_DATE 0)
endif()

# Ensure we have a sensible value for GIT_COMMIT_DATE
if (NOT GIT_COMMIT_DATE)
    set(GIT_COMMIT_DATE 0)
endif()

# Generate version header
configure_file(version.h.in ${CMAKE_BINARY_DIR}/version.h @ONLY)

# Output version, with commit hash if available
if (GIT_HEAD)
    string(SUBSTRING "${GIT_HEAD}" 0 7 extra_version)
    set(extra_version " (git-${extra_version})")
endif()
message(STATUS "Building Quassel ${QUASSEL_VERSION_STRING}${extra_version}")
