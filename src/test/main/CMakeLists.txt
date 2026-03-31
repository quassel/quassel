quassel_add_module(Test::Main EXPORT NOINSTALL)

target_sources(${TARGET} PRIVATE
    main.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        ${QUASSEL_QT_CORE_TARGET}
        Quassel::Test::Global
)
