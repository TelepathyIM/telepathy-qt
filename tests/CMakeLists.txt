find_program(SH sh)

set(test_environment "
export abs_top_builddir=${CMAKE_BINARY_DIR}
export abs_top_srcdir=${CMAKE_SOURCE_DIR}
export XDG_DATA_HOME=${CMAKE_SOURCE_DIR}/tests
export XDG_DATA_DIRS=${CMAKE_BINARY_DIR}/tests
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh "${test_environment} $@")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)

tpqt4_add_generic_unit_test(KeyFile key-file)
tpqt4_add_generic_unit_test(ManagerFile manager-file)

add_subdirectory(dbus-1)
add_subdirectory(lib)
add_subdirectory(dbus)
