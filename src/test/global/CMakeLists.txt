quassel_add_module(Test::Global EXPORT NOINSTALL)

target_sources(${TARGET} PRIVATE
    printers.cpp
    testglobal.h
)

target_link_libraries(${TARGET}
    PUBLIC
        GTest::GTest
        ${QT_FULL}::Core
        ${QT_FULL}::Test
        Quassel::Common
)
