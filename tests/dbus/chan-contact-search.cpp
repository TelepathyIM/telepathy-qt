#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/ContactSearchChannel>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/echo/conn.h>
#include <tests/lib/glib/contact-search-chan.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestContactSearchChan : public Test
{
    Q_OBJECT

public:
    TestContactSearchChan(QObject *parent = 0)
        : Test(parent),
          mConnService(0), mBaseConnService(0),
          mChan1Service(0)
    { }

protected Q_SLOTS:
    void onSearchStateChanged(Tp::ChannelContactSearchState state, const QString &errorName,
        const Tp::ContactSearchChannel::SearchStateChangeDetails &details);
    void onSearchResultReceived(const Tp::ContactSearchChannel::SearchResult &result);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testContactSearch();
    void testContactSearchEmptyResult();

    void cleanup();
    void cleanupTestCase();

private:
    ExampleEchoConnection *mConnService;
    TpBaseConnection *mBaseConnService;
    TpHandleRepoIface *mContactRepo;

    QString mConnName;
    QString mConnPath;
    ConnectionPtr mConn;
    ContactSearchChannelPtr mChan;
    ContactSearchChannelPtr mChan1;
    ContactSearchChannelPtr mChan2;

    QString mChan1Path;
    TpTestsContactSearchChannel *mChan1Service;
    QString mChan2Path;
    TpTestsContactSearchChannel *mChan2Service;

    ContactSearchChannel::SearchResult mSearchResult;

    struct SearchStateChangeInfo
    {
        SearchStateChangeInfo(ChannelContactSearchState state, const QString &errorName,
                const Tp::ContactSearchChannel::SearchStateChangeDetails &details)
            : state(state), errorName(errorName), details(details)
        {
        }

        ChannelContactSearchState state;
        QString errorName;
        ContactSearchChannel::SearchStateChangeDetails details;
    };
    QList<SearchStateChangeInfo> mSearchStateChangeInfoList;
};

void TestContactSearchChan::onSearchStateChanged(Tp::ChannelContactSearchState state,
        const QString &errorName,
        const Tp::ContactSearchChannel::SearchStateChangeDetails &details)
{
    mSearchStateChangeInfoList.append(SearchStateChangeInfo(state, errorName, details));
    mLoop->exit(0);
}

void TestContactSearchChan::onSearchResultReceived(
        const Tp::ContactSearchChannel::SearchResult &result)
{
    QCOMPARE(mChan->searchState(), ChannelContactSearchStateInProgress);
    mSearchResult = result;
    mLoop->exit(0);
}

void TestContactSearchChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contact-search-chan");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_ECHO_CONNECTION(g_object_new(
            EXAMPLE_TYPE_ECHO_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL));
    QVERIFY(mConnService != 0);
    mBaseConnService = TP_BASE_CONNECTION(mConnService);
    QVERIFY(mBaseConnService != 0);

    QVERIFY(tp_base_connection_register(mBaseConnService,
                "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Connection::StatusConnected));

    QByteArray chan1Path;
    mChan1Path = mConnPath + QLatin1String("/ContactSearchChannel/1");
    chan1Path = mChan1Path.toAscii();
    mChan1Service = TP_TESTS_CONTACT_SEARCH_CHANNEL(g_object_new(
                TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL,
                "connection", mConnService,
                "object-path", chan1Path.data(),
                NULL));

    QByteArray chan2Path;
    mChan2Path = mConnPath + QLatin1String("/ContactSearchChannel/2");
    chan2Path = mChan2Path.toAscii();
    mChan2Service = TP_TESTS_CONTACT_SEARCH_CHANNEL(g_object_new(
                TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL,
                "connection", mConnService,
                "object-path", chan2Path.data(),
                NULL));
}

void TestContactSearchChan::init()
{
    initImpl();
    mSearchResult.clear();
    mSearchStateChangeInfoList.clear();
}

void TestContactSearchChan::testContactSearch()
{
    mChan1 = ContactSearchChannel::create(mConn, mChan1Path, QVariantMap());
    mChan = mChan1;
    QVERIFY(connect(mChan1->becomeReady(ContactSearchChannel::FeatureCore),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan1->isReady(), true);

    QCOMPARE(mChan1->searchState(), ChannelContactSearchStateNotStarted);
    QCOMPARE(mChan1->limit(), static_cast<uint>(0));
    QCOMPARE(mChan1->availableSearchKeys().isEmpty(), false);
    QCOMPARE(mChan1->availableSearchKeys().count(), 1);
    QCOMPARE(mChan1->availableSearchKeys().first(), QLatin1String("employer"));
    QCOMPARE(mChan1->server(), QLatin1String("characters.shakespeare.lit"));

    QVERIFY(connect(mChan1.data(),
                SIGNAL(searchStateChanged(Tp::ChannelContactSearchState, const QString &,
                        const Tp::ContactSearchChannel::SearchStateChangeDetails &)),
                SLOT(onSearchStateChanged(Tp::ChannelContactSearchState, const QString &,
                        const Tp::ContactSearchChannel::SearchStateChangeDetails &))));
    QVERIFY(connect(mChan1.data(),
                SIGNAL(searchResultReceived(const Tp::ContactSearchChannel::SearchResult &)),
                SLOT(onSearchResultReceived(const Tp::ContactSearchChannel::SearchResult &))));

    mChan1->search(QLatin1String("employer"), QLatin1String("Collabora"));
    while (mChan1->searchState() != ChannelContactSearchStateCompleted) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mSearchStateChangeInfoList.count(), 2);
    SearchStateChangeInfo info = mSearchStateChangeInfoList.at(0);
    QCOMPARE(info.state, ChannelContactSearchStateInProgress);
    QCOMPARE(info.errorName, QLatin1String(""));
    QCOMPARE(info.details.hasDebugMessage(), true);
    QCOMPARE(info.details.debugMessage(), QLatin1String("in progress"));

    info = mSearchStateChangeInfoList.at(1);
    QCOMPARE(info.state, ChannelContactSearchStateCompleted);
    QCOMPARE(info.errorName, QLatin1String(""));
    QCOMPARE(info.details.hasDebugMessage(), true);
    QCOMPARE(info.details.debugMessage(), QLatin1String("completed"));

    QCOMPARE(mSearchResult.isEmpty(), false);
    QCOMPARE(mSearchResult.size(), 3);

    QStringList expectedIds;
    expectedIds << QLatin1String("oggis") << QLatin1String("andrunko") <<
        QLatin1String("wjt");
    expectedIds.sort();
    QStringList expectedFns;
    expectedFns << QLatin1String("Olli Salli") << QLatin1String("Andre Moreira Magalhaes") <<
        QLatin1String("Will Thompson");
    expectedFns.sort();
    QStringList ids;
    QStringList fns;
    for (ContactSearchChannel::SearchResult::const_iterator it = mSearchResult.constBegin();
                                                            it != mSearchResult.constEnd();
                                                            ++it)
    {
        QCOMPARE(it.key().isNull(), false);
        ids << it.key()->id();
        QCOMPARE(it.value().isValid(), true);
        QCOMPARE(it.value().allFields().isEmpty(), false);
        Q_FOREACH (const ContactInfoField &contactInfo, it.value().allFields()) {
            QCOMPARE(contactInfo.fieldName, QLatin1String("fn"));
            fns.append(contactInfo.fieldValue.first());
        }
    }
    ids.sort();
    QCOMPARE(ids, expectedIds);
    fns.sort();
    QCOMPARE(fns, expectedFns);

    mChan1.reset();
}

void TestContactSearchChan::testContactSearchEmptyResult()
{
    mChan2 = ContactSearchChannel::create(mConn, mChan2Path, QVariantMap());
    mChan = mChan2;
    QVERIFY(connect(mChan2->becomeReady(ContactSearchChannel::FeatureCore),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan2->isReady(), true);

    QCOMPARE(mChan2->searchState(), ChannelContactSearchStateNotStarted);
    QCOMPARE(mChan2->limit(), static_cast<uint>(0));
    QCOMPARE(mChan2->availableSearchKeys().isEmpty(), false);
    QCOMPARE(mChan2->availableSearchKeys().count(), 1);
    QCOMPARE(mChan2->availableSearchKeys().first(), QLatin1String("employer"));
    QCOMPARE(mChan2->server(), QLatin1String("characters.shakespeare.lit"));

    QVERIFY(connect(mChan2.data(),
                SIGNAL(searchStateChanged(Tp::ChannelContactSearchState, const QString &,
                        const Tp::ContactSearchChannel::SearchStateChangeDetails &)),
                SLOT(onSearchStateChanged(Tp::ChannelContactSearchState, const QString &,
                        const Tp::ContactSearchChannel::SearchStateChangeDetails &))));
    QVERIFY(connect(mChan2.data(),
                SIGNAL(searchResultReceived(const Tp::ContactSearchChannel::SearchResult &)),
                SLOT(onSearchResultReceived(const Tp::ContactSearchChannel::SearchResult &))));

    ContactSearchMap searchTerms;
    searchTerms.insert(QLatin1String("employer"), QLatin1String("FooBar"));
    mChan2->search(searchTerms);
    while (mChan2->searchState() != ChannelContactSearchStateCompleted) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mSearchResult.isEmpty(), true);

    QCOMPARE(mSearchStateChangeInfoList.count(), 2);
    SearchStateChangeInfo info = mSearchStateChangeInfoList.at(0);
    QCOMPARE(info.state, ChannelContactSearchStateInProgress);
    QCOMPARE(info.errorName, QLatin1String(""));
    QCOMPARE(info.details.hasDebugMessage(), true);
    QCOMPARE(info.details.debugMessage(), QLatin1String("in progress"));

    info = mSearchStateChangeInfoList.at(1);
    QCOMPARE(info.state, ChannelContactSearchStateCompleted);
    QCOMPARE(info.errorName, QLatin1String(""));
    QCOMPARE(info.details.hasDebugMessage(), true);
    QCOMPARE(info.details.debugMessage(), QLatin1String("completed"));

    mChan2.reset();
}

void TestContactSearchChan::cleanup()
{
    cleanupImpl();
}

void TestContactSearchChan::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                            SIGNAL(invalidated(Tp::DBusProxy *,
                                               const QString &, const QString &)),
                            mLoop,
                            SLOT(quit())));
            QCOMPARE(mLoop->exec(), 0);
        }
    }

    if (mChan1Service != 0) {
        g_object_unref(mChan1Service);
        mChan1Service = 0;
    }

    if (mChan2Service != 0) {
        g_object_unref(mChan2Service);
        mChan2Service = 0;
    }

    if (mConnService != 0) {
        mBaseConnService = 0;
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactSearchChan)
#include "_gen/chan-contact-search.cpp.moc.hpp"
