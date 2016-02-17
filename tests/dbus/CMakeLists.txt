file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

tpqt_setup_dbus_test_environment()

if(HAVE_TEST_PYTHON)
    tpqt_add_dbus_unit_test(DBusProperties dbus-properties "")
endif(HAVE_TEST_PYTHON)

if(ENABLE_TP_GLIB_TESTS)
    include_directories(${CMAKE_SOURCE_DIR}/tests/lib/glib
                        ${TELEPATHY_GLIB_INCLUDE_DIR}
                        ${GLIB2_INCLUDE_DIR}
                        ${DBUS_INCLUDE_DIR})

    add_definitions(-DQT_NO_KEYWORDS)

    if (${QT_VERSION_MAJOR} EQUAL 4)
        if (NOT ${QT_VERSION_MINOR} EQUAL 6)
            # >= qt 4.7
            # FIXME - Re-enable racy tests once https://bugs.freedesktop.org/show_bug.cgi?id=43356 is
            #         fixed
            SET(ENABLE_TESTS_WITH_RACES_IN_QT_4_6 TRUE)
            SET(ENABLE_TESTS_WITH_ISSUES_IN_QT_5 TRUE)
        endif (NOT ${QT_VERSION_MINOR} EQUAL 6)
    else (${QT_VERSION_MAJOR} EQUAL 4)
        # Currently none - this variable is here in case some issues arise
        SET(ENABLE_TESTS_WITH_ISSUES_IN_QT_5 FALSE)
        SET(ENABLE_TESTS_WITH_RACES_IN_QT_4_6 TRUE)
    endif (${QT_VERSION_MAJOR} EQUAL 4)

    if(HAVE_TEST_PYTHON)
        tpqt_add_dbus_unit_test(AccountBasics account-basics tp-glib-tests tp-qt-tests-glib-helpers)
        tpqt_add_dbus_unit_test(AccountSet account-set tp-glib-tests tp-qt-tests-glib-helpers)
        tpqt_add_dbus_unit_test(AccountChannelDispatcher account-channel-dispatcher tp-glib-tests tp-qt-tests-glib-helpers)
        tpqt_add_dbus_unit_test(Client client tp-glib-tests tp-qt-tests-glib-helpers)
        tpqt_add_dbus_unit_test(ClientFactories client-factories tp-glib-tests)
    endif(HAVE_TEST_PYTHON)

    tpqt_add_dbus_unit_test(AccountConnectionFactory account-connection-factory tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(CallChannel call-channel tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(CaptchaAuthentication captcha-authentication tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ChannelBasics chan-basics tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ChannelConference chan-conference tp-glib-tests future-example-cm-conference tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ChannelGroup chan-group tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ConnectionManagerBasics cm-basics tp-glib-tests)
    tpqt_add_dbus_unit_test(ConnectionAddressing conn-addressing tp-glib-tests future-example-conn-addressing tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ConnectionBasics conn-basics tp-glib-tests)
    tpqt_add_dbus_unit_test(ConnectionCapabilities conn-capabilities tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ConnectionIntrospectCornercases conn-introspect-cornercases tp-glib-tests)
    tpqt_add_dbus_unit_test(ConnectionRequests conn-requests tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ConnectionRosterLegacy conn-roster-legacy tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ConnectionRoster conn-roster example-cm-contactlist2 tp-qt-tests-glib-helpers
        ${GLIB2_LIBRARIES} ${GOBJECT_LIBRARIES} ${DBUS_GLIB_LIBRARIES} ${TELEPATHY_GLIB_LIBRARIES})
    tpqt_add_dbus_unit_test(ConnectionRosterGroupsLegacy conn-roster-groups-legacy tp-glib-tests)
    tpqt_add_dbus_unit_test(ConnectionRosterGroups conn-roster-groups example-cm-contactlist2
        ${GLIB2_LIBRARIES} ${GOBJECT_LIBRARIES} ${DBUS_GLIB_LIBRARIES} ${TELEPATHY_GLIB_LIBRARIES})
    tpqt_add_dbus_unit_test(ContactFactory contact-factory tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ContactMessenger contact-messenger tp-glib-tests)
    tpqt_add_dbus_unit_test(ContactSearchChannel contact-search-chan tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(Contacts contacts tp-glib-tests)
    tpqt_add_dbus_unit_test(ContactsAvatar contacts-avatar tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ContactsCapabilities contacts-capabilities tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ContactsClientTypes contacts-client-types tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ContactsInfo contacts-info tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(ContactsLocation contacts-location tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(DBusProxyFactory dbus-proxy-factory tp-glib-tests telepathy-qt-test-backdoors)
    tpqt_add_dbus_unit_test(Handles handles tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(Properties properties tp-glib-tests tp-qt-tests-glib-helpers)
    tpqt_add_dbus_unit_test(SimpleObserver simple-observer tp-glib-tests)
    tpqt_add_dbus_unit_test(StatefulProxy stateful-proxy tp-glib-tests)
    tpqt_add_dbus_unit_test(StreamedMediaChannel streamed-media-chan tp-glib-tests tp-qt-tests-glib-helpers)

    if (ENABLE_TESTS_WITH_RACES_IN_QT_4_6)
        tpqt_add_dbus_unit_test(TextChannel text-chan tp-glib-tests tp-qt-tests-glib-helpers)
        tpqt_add_dbus_unit_test(StreamTubeHandlers stream-tube-handlers tp-glib-tests tp-qt-tests-glib-helpers)
        if(ENABLE_TP_GLIB_GIO_TESTS)
            tpqt_add_dbus_unit_test(StreamTubeChannel stream-tube-chan tp-glib-tests tp-qt-tests-glib-helpers)
        endif(ENABLE_TP_GLIB_GIO_TESTS)
    endif (ENABLE_TESTS_WITH_RACES_IN_QT_4_6)

    if(NOT (${QT_VERSION_MAJOR} EQUAL 4 AND ${QT_VERSION_MINOR} LESS 8))
        message(STATUS "Enabling Qt 4.8+ tests")
        tpqt_add_dbus_unit_test(DBusTubeChannel dbus-tube-chan tp-glib-tests tp-qt-tests-glib-helpers)
    endif(NOT (${QT_VERSION_MAJOR} EQUAL 4 AND ${QT_VERSION_MINOR} LESS 8))


endif(ENABLE_TP_GLIB_TESTS)

tpqt_add_dbus_unit_test(CmProtocol cm-protocol)
tpqt_add_dbus_unit_test(ProfileManager profile-manager)
tpqt_add_dbus_unit_test(Types types)

if(ENABLE_SERVICE_SUPPORT)
    tpqt_add_dbus_unit_test(BaseConnectionManager base-cm telepathy-qt${QT_VERSION_MAJOR}-service)
    tpqt_add_dbus_unit_test(BaseProtocol base-protocol telepathy-qt${QT_VERSION_MAJOR}-service)
    tpqt_add_dbus_unit_test(BaseChannelFileTransferType base-filetransfer telepathy-qt${QT_VERSION_MAJOR}-service)
endif(ENABLE_SERVICE_SUPPORT)

# Make check target. In case of check, output on failure and put it into a log
# This target has to stay here for catching all of the tests
add_custom_target(check ctest --output-on-failure -O test.log
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
add_dependencies(check check-local ${_telepathy_qt_test_cases})
