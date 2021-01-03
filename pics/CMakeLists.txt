# pics/ contains non-themed icons and other graphical resources

if (WANT_MONO OR WANT_QTCLIENT)
    quassel_add_resource(Pics PREFIX pics PATTERNS
        kde-vdg.png
        quassel-64.svg
        quassel-eye.png
        quassel-logo.png
        quassel-oxygen-32.png
        qt-logo-32.png
    )
endif()

# On macOS, the individual icons should be converted into an iconset
add_custom_command(
    COMMENT "Creating iconset for macOS"
    OUTPUT quassel.icns
    COMMAND iconutil ARGS -c icns -o ${CMAKE_CURRENT_BINARY_DIR}/quassel.icns ${CMAKE_CURRENT_SOURCE_DIR}/quassel.iconset
)
add_custom_target(MacOsIcons DEPENDS quassel.icns)
