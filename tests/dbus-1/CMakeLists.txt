set(abs_top_builddir ${CMAKE_BINARY_DIR})
configure_file(session.conf.in ${CMAKE_CURRENT_BINARY_DIR}/session.conf)

add_subdirectory(services)
