find_program(SH sh)

set(test_environment "
export abs_top_builddir=${CMAKE_BINARY_DIR}
export abs_top_srcdir=${CMAKE_SOURCE_DIR}
export XDG_DATA_HOME=${CMAKE_SOURCE_DIR}/tests
export XDG_DATA_DIRS=${CMAKE_BINARY_DIR}/tests
")

# Add targets for callgrind and valgrind tests
add_custom_target(check-valgrind)
add_custom_target(check-callgrind)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh "${test_environment} $@")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_COVERAGE_FLAGS}")

tpqt4_add_generic_unit_test(KeyFile key-file)
tpqt4_add_generic_unit_test(ManagerFile manager-file)

add_subdirectory(dbus-1)
add_subdirectory(dbus)
add_subdirectory(lib)
