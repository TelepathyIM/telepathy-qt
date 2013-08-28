set(TEST_PYTHON ${PYTHON_EXECUTABLE})
set(abs_top_srcdir ${CMAKE_SOURCE_DIR})
configure_file(account-manager.service.in ${CMAKE_CURRENT_BINARY_DIR}/account-manager.service)
configure_file(spurious.service.in ${CMAKE_CURRENT_BINARY_DIR}/spurious.service)

# Add Nemo install target
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/spurious.service ${CMAKE_CURRENT_BINARY_DIR}/account-manager.service DESTINATION ${NEMO_TESTS_DIR_DBUS}/services/)

