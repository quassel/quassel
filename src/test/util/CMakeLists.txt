quassel_add_module(Test::Util EXPORT NOINSTALL)

target_sources(${TARGET} PRIVATE
    integrationtestsupport.cpp
    invocationspy.cpp
    mockedpeer.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        Boost::boost
        Quassel::Common
        Quassel::Client
        Quassel::Core
        Quassel::Test::Global
)
