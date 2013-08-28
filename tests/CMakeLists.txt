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

# Add Nemo install targets
if (NOT "${Qt5Core_VERSION_MAJOR}" STREQUAL "")
    set(NEMO_TESTS_DIR "/opt/tests/telepathy-qt${Qt5Core_VERSION_MAJOR}")
else()
    set(NEMO_TESTS_DIR "/opt/tests/telepathy-qt${QT_VERSION_MAJOR}")
endif()

set(NEMO_TESTS_DIR_BIN "${NEMO_TESTS_DIR}/bin")
set(NEMO_TESTS_DIR_LIB "${NEMO_TESTS_DIR}/lib")
set(NEMO_TESTS_DIR_DATA "${NEMO_TESTS_DIR}/data")
set(NEMO_TESTS_DIR_DBUS "${NEMO_TESTS_DIR}/dbus-1")
configure_file(mktests.sh.in ${CMAKE_CURRENT_SOURCE_DIR}/mktests.sh)
configure_file(runDbusTest.sh.in ${CMAKE_CURRENT_SOURCE_DIR}/runDbusTest.sh)
configure_file(runTest.sh.in ${CMAKE_CURRENT_SOURCE_DIR}/runTest.sh)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/unit-tests.list "${_telepathy_qt_test_cases}")
INSTALL(TARGETS ${_telepathy_qt_test_cases} RUNTIME DESTINATION ${NEMO_TESTS_DIR_BIN})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/runTest.sh ${CMAKE_CURRENT_SOURCE_DIR}/runDbusTest.sh DESTINATION ${NEMO_TESTS_DIR_BIN} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/INSIGNIFICANT ${CMAKE_CURRENT_SOURCE_DIR}/tests.xml DESTINATION ${NEMO_TESTS_DIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/unit-tests.list DESTINATION ${NEMO_TESTS_DIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/test-key-file-format-error.ini ${CMAKE_CURRENT_SOURCE_DIR}/test-key-file.ini DESTINATION ${NEMO_TESTS_DIR_DATA}/tests)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/telepathy DESTINATION ${NEMO_TESTS_DIR_DATA}/tests)

add_subdirectory(dbus-1)
add_subdirectory(dbus)
add_subdirectory(lib)
