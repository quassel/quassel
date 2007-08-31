qtopia_project(qtopia app)

TARGET=quasseltopia
CONFIG+=qtopia_main no_quicklaunch

# Find files
INCLUDEPATH+=../src/qtopia ../src/client ../src/common ../src/contrib/qxt


# Include .pro from src dirs
include(../src/contrib/qxt/qxt.pro)
include(../src/common/common.pro)
include(../src/qtopia/qtopia.pro)
include(../src/client/client.pro)

# SXE permissions required
pkg.domain=none

