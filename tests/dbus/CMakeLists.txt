file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/runDbusTest.sh "
${test_environment}
sh ${CMAKE_SOURCE_DIR}/tools/with-session-bus.sh \\
    --config-file=${CMAKE_BINARY_DIR}/tests/dbus-1/session.conf -- $@
")
set(with_session_bus ${SH} ${CMAKE_CURRENT_BINARY_DIR}/runDbusTest.sh)

function(add_dbus_unit_test fancyName name libraries)
    qt4_generate_moc_i(${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    add_executable(test-${name} ${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    target_link_libraries(test-${name} ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY} telepathy-qt4 tp-qt4-tests ${libraries})
    add_test(${fancyName} ${with_session_bus} ${CMAKE_CURRENT_BINARY_DIR}/test-${name})
endfunction(add_dbus_unit_test fancyName name libraries)

if(HAVE_TEST_PYTHON)
    add_dbus_unit_test(AccountBasics account-basics "")
endif(HAVE_TEST_PYTHON)

if(ENABLE_TP_GLIB_TESTS)
    include_directories(${TELEPATHY_GLIB_INCLUDE_DIR}
                        ${GLIB2_INCLUDE_DIR}
                        ${DBUS_INCLUDE_DIR})

    if(HAVE_TEST_PYTHON)
        add_dbus_unit_test(AccountChannelDispatcher account-channel-dispatcher tp-glib-tests)
        add_dbus_unit_test(Client client tp-glib-tests)
    endif(HAVE_TEST_PYTHON)

    add_dbus_unit_test(ChannelBasics chan-basics tp-glib-tests)
    add_dbus_unit_test(ChannelConference chan-conference tp-glib-tests)
    add_dbus_unit_test(ChannelGroup chan-group tp-glib-tests)
    add_dbus_unit_test(ConnectionManagerBasics cm-basics tp-glib-tests)
    add_dbus_unit_test(ConnectionBasics conn-basics tp-glib-tests)
    add_dbus_unit_test(ConnectionCapabilities conn-capabilities tp-glib-tests)
    add_dbus_unit_test(ConnectionRequests conn-requests tp-glib-tests)
    add_dbus_unit_test(ConnectionRoster conn-roster tp-glib-tests)
    add_dbus_unit_test(ConnectionRosterGroups conn-roster-groups tp-glib-tests)
    add_dbus_unit_test(Contacts contacts tp-glib-tests)
    add_dbus_unit_test(ContactsCapabilities contacts-capabilities tp-glib-tests)
    add_dbus_unit_test(ContactsLocation contacts-location tp-glib-tests)
    add_dbus_unit_test(Handles handles tp-glib-tests)
    add_dbus_unit_test(StatefulProxy stateful-proxy tp-glib-tests)
    add_dbus_unit_test(StreamedMediaChannel streamed-media-chan tp-glib-tests)
    add_dbus_unit_test(StreamedMediaChannelCall streamed-media-chan-call tp-glib-tests)
    add_dbus_unit_test(TextChannel text-chan tp-glib-tests)
endif(ENABLE_TP_GLIB_TESTS)
