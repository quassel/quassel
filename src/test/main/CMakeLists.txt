quassel_add_module(Test::Main EXPORT NOINSTALL)

target_sources(${TARGET} PRIVATE
    main.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        ${QT_FULL}::Core
        Quassel::Test::Global
)
