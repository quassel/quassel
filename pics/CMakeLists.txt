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
