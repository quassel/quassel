quassel_add_module(Test::Global EXPORT NOINSTALL)

target_sources(${TARGET} PRIVATE
    printers.cpp
    testglobal.h
)

target_link_libraries(${TARGET}
    PUBLIC
        GTest::GTest
        ${QUASSEL_QT_CORE_TARGET}
        ${QUASSEL_QT_TEST_TARGET}
        Quassel::Common
)
