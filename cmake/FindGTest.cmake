# It is recommended that projects build GTest/GMock themselves, rather
# than relying on system-provided libraries. This is to ensure that the
# same build configuration is used for both test cases and the
# libraries.
#
# This find module includes bundled sources to build the GTest and GMock
# libraries. Since including the provided CMake build system as a
# subproject would not allow for proper target installation,
# and linking GTest as a static library poses a special kind of hell,
# we rely on GTest build system internals to setup targets under our
# control.
#
# This means that this find module will only work properly as long as
# the layout and the internals of the provided GTest/GMock sources
# do not change in incompatible ways. Thus, if the bundled sources
# are updated, this find script may have to be adapted, too.
#
# This find module defines one alias target GMock::GMock to depend on.
########################################################################

include(FindPackageHandleStandardArgs)

set(GTEST_VERSION "1.8.1")

set(GTEST_BASE_DIR "${CMAKE_SOURCE_DIR}/3rdparty/googletest-${GTEST_VERSION}")
set(GTEST_SRC_DIR "${GTEST_BASE_DIR}/googletest")
set(GTEST_INC_DIR "${GTEST_BASE_DIR}/googletest/include")
set(GMOCK_SRC_DIR "${GTEST_BASE_DIR}/googlemock")
set(GMOCK_INC_DIR "${GTEST_BASE_DIR}/googlemock/include")

# We don't install our tests
set(INSTALL_GTEST OFF)

# Link against the runtime dynamically on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Here's where it gets shaky... include GTest's CMake macros, which
# we'll use to setup the target with the expected compile flags.
include(${GTEST_SRC_DIR}/cmake/internal_utils.cmake)

# Provided by GTest, configure compile flags
config_compiler_and_linker()

# Create target using GTest's macro and compile flags
set(TARGET gtest)
cxx_shared_library(${TARGET}
    ${cxx_strict}
    ${GTEST_SRC_DIR}/src/gtest-all.cc
    ${GMOCK_SRC_DIR}/src/gmock-all.cc
)

# Alias to follow our convention of namespaced targets
add_library(GTest::GTest ALIAS gtest)

# Set include dirs
target_include_directories(${TARGET} SYSTEM BEFORE
    PUBLIC
        ${GTEST_INC_DIR}
        ${GMOCK_INC_DIR}
    PRIVATE
        ${GTEST_SRC_DIR}
        ${GMOCK_SRC_DIR}
)

# Ensure symbols are exported
target_compile_definitions(${TARGET} INTERFACE -DGTEST_LINKED_AS_SHARED_LIBRARY=1)

# Add support for find_package
find_package_handle_standard_args(GTest DEFAULT_MSG GTEST_INC_DIR)
