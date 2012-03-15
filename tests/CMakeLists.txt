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

# Add targets for lcov reports
add_custom_target(lcov-reset lcov --directory ${CMAKE_BINARY_DIR} --zerocounters
                             COMMAND find ${CMAKE_BINARY_DIR} -name '*.gcda' -exec rm -f '{}' ';' || true
                             COMMENT "Cleaning lcov files")

add_custom_target(lcov-check make test || true
                             COMMAND lcov --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/lcov.info &&
                                     mkdir ${CMAKE_BINARY_DIR}/lcov.html || true && genhtml --title ${PACKAGE_NAME}
                                     --output-directory ${CMAKE_BINARY_DIR}/lcov.html ${CMAKE_BINARY_DIR}/lcov.info
                             COMMENT "Generating lcov report in file://${CMAKE_BINARY_DIR}/lcov.html/index.html"
                             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
add_dependencies(lcov-check lcov-reset)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh "${test_environment} $@")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_COVERAGE_FLAGS}")

tpqt_add_generic_unit_test(Capabilities capabilities telepathy-qt-test-backdoors)
tpqt_add_generic_unit_test(Callbacks callbacks)
tpqt_add_generic_unit_test(ChannelClassSpec channel-class-spec)
tpqt_add_generic_unit_test(Features features)
tpqt_add_generic_unit_test(KeyFile key-file telepathy-qt-test-backdoors)
tpqt_add_generic_unit_test(ManagerFile manager-file telepathy-qt-test-backdoors)
tpqt_add_generic_unit_test(Presence presence)
tpqt_add_generic_unit_test(Profile profile)
tpqt_add_generic_unit_test(Ptr ptr)
tpqt_add_generic_unit_test(RCCSpec rccspec)
tpqt_add_generic_unit_test(FileTransferChannelCreationProperties file-transfer-channel-creation-properties)

add_subdirectory(dbus-1)
add_subdirectory(dbus)
add_subdirectory(lib)
