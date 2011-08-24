#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/glib/stream-tube-chan.h>

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/StreamTubeChannel>
#include <TelepathyQt4/StreamTubeClient>
#include <TelepathyQt4/StreamTubeServer>

#include <telepathy-glib/telepathy-glib.h>

using namespace Tp;

namespace
{

void destroySocketControlList(gpointer data)
{
    g_array_free((GArray *) data, TRUE);
}

// TODO: turn into something which supports everything, or everything except port/credentials ACs
GHashTable *createSupportedSocketTypesHash(TpSocketAddressType addressType,
        TpSocketAccessControl accessControl)
{
    GHashTable *ret;
    GArray *tab;

    ret = g_hash_table_new_full(NULL, NULL, NULL, destroySocketControlList);

    tab = g_array_sized_new(FALSE, FALSE, sizeof(TpSocketAccessControl), 1);
    g_array_append_val(tab, accessControl);

    g_hash_table_insert(ret, GUINT_TO_POINTER(addressType), tab);

    return ret;
}

}

class TestStreamTubeHandlers : public Test
{
    Q_OBJECT

public:
    TestStreamTubeHandlers(QObject *parent = 0)
        : Test(parent)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void cleanup();
    void cleanupTestCase();

private:

    void createTubeChannel(bool requested, TpSocketAddressType addressType,
            TpSocketAccessControl accessControl, bool withContact);

    TestConnHelper *mConn;
    TpTestsStreamTubeChannel *mChanService;
    StreamTubeChannelPtr mChan;
};

// TODO: turn into creating one (of possibly many) channels, which support everything, or everything
// except Port and Creds ACs
void TestStreamTubeHandlers::createTubeChannel(bool requested,
        TpSocketAddressType addressType,
        TpSocketAccessControl accessControl,
        bool withContact)
{
    mLoop->processEvents();
    tp_clear_object(&mChanService);

    /* Create service-side tube channel object */
    QString chanPath = QString(QLatin1String("%1/Channel")).arg(mConn->objectPath());

    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    TpHandleRepoIface *roomRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_ROOM);
    TpHandle handle;
    GType type;
    if (withContact) {
        handle = tp_handle_ensure(contactRepo, "bob", NULL, NULL);
        type = TP_TESTS_TYPE_CONTACT_STREAM_TUBE_CHANNEL;
    } else {
        handle = tp_handle_ensure(roomRepo, "#test", NULL, NULL);
        type = TP_TESTS_TYPE_ROOM_STREAM_TUBE_CHANNEL;
    }

    TpHandle alfHandle = tp_handle_ensure(contactRepo, "alf", NULL, NULL);

    GHashTable *sockets = createSupportedSocketTypesHash(addressType, accessControl);

    mChanService = TP_TESTS_STREAM_TUBE_CHANNEL(g_object_new(
            type,
            "connection", mConn->service(),
            "handle", handle,
            "requested", requested,
            "object-path", chanPath.toLatin1().constData(),
            "supported-socket-types", sockets,
            "initiator-handle", alfHandle,
            NULL));

    /* Create client-side tube channel object */
    GHashTable *props;
    g_object_get(mChanService, "channel-properties", &props, NULL);

    g_hash_table_unref(props);
    g_hash_table_unref(sockets);

    if (withContact)
        tp_handle_unref(contactRepo, handle);
    else
        tp_handle_unref(roomRepo, handle);
}

void TestStreamTubeHandlers::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("stream-tube-handlers");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_SIMPLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestStreamTubeHandlers::init()
{
    initImpl();
}

void TestStreamTubeHandlers::cleanup()
{
    cleanupImpl();

    if (mChan && mChan->isValid()) {
        qDebug() << "waiting for the channel to become invalidated";

        QVERIFY(connect(mChan.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                mLoop,
                SLOT(quit())));
        tp_base_channel_close(TP_BASE_CHANNEL(mChanService));
        QCOMPARE(mLoop->exec(), 0);
    }

    mChan.reset();

    if (mChanService != 0) {
        g_object_unref(mChanService);
        mChanService = 0;
    }

    mLoop->processEvents();
}

void TestStreamTubeHandlers::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStreamTubeHandlers)
#include "_gen/stream-tube-handlers.cpp.moc.hpp"
