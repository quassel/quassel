quassel_add_module(Test::Global EXPORT NOINSTALL)

target_sources(${TARGET} PRIVATE
    testglobal.h
)

target_link_libraries(${TARGET}
    PUBLIC
        GTest::GTest
        Qt5::Core
        Qt5::Test
        Quassel::Common
)
