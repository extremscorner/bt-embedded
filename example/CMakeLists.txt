if(OpenObex_FOUND)
    list(APPEND EXTRA_LIBS PkgConfig::OpenObex)
    add_definitions(-DWITH_OPENOBEX)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "NintendoWii")
    list(APPEND EXTRA_EXAMPLE_LIBS fat)
endif()

add_executable(bte-example
    main.c
    terminal.c
)

target_include_directories(bte-example PRIVATE
    ..
)

target_link_libraries(bte-example PRIVATE
    bt-embedded
    ${EXTRA_EXAMPLE_LIBS}
)

add_executable(bte-obex
    obex.c
    terminal.c
)

target_include_directories(bte-obex PRIVATE
    ..
)

target_link_libraries(bte-obex PRIVATE
    ${EXTRA_LIBS}
    bt-embedded
    ${EXTRA_EXAMPLE_LIBS}
)
