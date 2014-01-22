#include <QtCore/QDebug>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Connection>
#include <TelepathyQt/Debug>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/debug.h>
#include <telepathy-glib/svc-generic.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestProperties : public Test
{
    Q_OBJECT

public:
    TestProperties(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testPropertiesMonitoring();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsContactsConnection *mConnService;
    Client::ConnectionInterface *mConn;
};

void TestProperties::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("properties");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);
}

void TestProperties::init()
{
    initImpl();

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "contacts", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = new Client::ConnectionInterface(mConnName, mConnPath,
            this);

    QVERIFY(mConn->isValid());
}

void TestProperties::testPropertiesMonitoring()
{
    QCOMPARE(mConn->isMonitoringProperties(), false);
    mConn->setMonitorProperties(true);

    QSignalSpy spy(mConn, SIGNAL(propertiesChanged(QVariantMap,QStringList)));
    connect(mConn, SIGNAL(propertiesChanged(QVariantMap,QStringList)),
            mLoop, SLOT(quit()));

    GHashTable *changed = tp_asv_new(
                "test-prop", G_TYPE_STRING, "I am actually different than I used to be.",
                "test-again", G_TYPE_UINT, 0xff0000ffU,
                NULL
                );

    tp_svc_dbus_properties_emit_properties_changed (mConnService,
            "a.random.interface", changed, NULL);

    tp_svc_dbus_properties_emit_properties_changed (mConnService,
            mConn->interface().toLatin1().data(), changed, NULL);

    QCOMPARE(spy.count(), 0);

    mLoop->exec();

    QCOMPARE(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).type() == QVariant::Map);
    QVERIFY(arguments.at(1).type() == QVariant::StringList);

    QVariantMap resultMap = arguments.at(0).toMap();
    QCOMPARE(resultMap.size(), 2);
    QCOMPARE(resultMap[QLatin1String("test-prop")].toString(), QLatin1String("I am actually different than I used to be."));
    QCOMPARE(resultMap[QLatin1String("test-again")].toUInt(), 0xff0000ffU);

    QCOMPARE(spy.count(), 0);

    g_hash_table_destroy (changed);
}

void TestProperties::cleanup()
{
    if (mConn) {
        mConn->deleteLater();
        mConn = 0;
    }
    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }
    cleanupImpl();
}

void TestProperties::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestProperties)
#include "_gen/properties.cpp.moc.hpp"
