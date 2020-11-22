quassel_add_module(Test::Util EXPORT NOINSTALL)

target_sources(${TARGET} PRIVATE
    invocationspy.cpp
    mockedpeer.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        Quassel::Common
        Quassel::Test::Global
)
