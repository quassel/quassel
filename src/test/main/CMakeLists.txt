quassel_add_module(Test::Main EXPORT NOINSTALL)

target_sources(${TARGET} PRIVATE
    main.cpp
    testglobal.h
)

target_link_libraries(${TARGET}
    PUBLIC
        GTest::GTest
        Qt5::Core
        Quassel::Common
)
