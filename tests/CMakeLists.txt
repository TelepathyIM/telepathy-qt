include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${QT_INCLUDES})

find_program(SH sh)

set(test_environment "
export abs_top_builddir=${CMAKE_BINARY_DIR}
export abs_top_srcdir=${CMAKE_SOURCE_DIR}
export XDG_DATA_HOME=${CMAKE_BINARY_DIR}/tests
export XDG_DATA_DIRS=${CMAKE_SOURCE_DIR}/tests
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh "${test_environment} $@")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)
function(add_generic_unit_test fancyName name)
    qt4_generate_moc_i(${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    add_executable(test-${name} ${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    target_link_libraries(test-${name} ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY} telepathy-qt4 tp-qt4-tests)
    add_test(${fancyName} ${SH} ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh ${CMAKE_CURRENT_BINARY_DIR}/test-${name})
endfunction(add_generic_unit_test fancyName name)

add_generic_unit_test(KeyFile key-file)
add_generic_unit_test(ManagerFile manager-file)

add_subdirectory(dbus-1/services)
add_subdirectory(lib)
add_subdirectory(dbus)
