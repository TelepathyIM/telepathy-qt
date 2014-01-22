#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo/conn.h>
#include <tests/lib/glib/contact-search-chan.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactSearchChannel>
#include <TelepathyQt/PendingReady>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestContactSearchChan : public Test
{
    Q_OBJECT

public:
    TestContactSearchChan(QObject *parent = 0)
        : Test(parent),
          mConn(0),
          mChan1Service(0), mChan2Service(0), mSearchReturned(false)
    { }

protected Q_SLOTS:
    void onSearchStateChanged(Tp::ChannelContactSearchState state, const QString &errorName,
        const Tp::ContactSearchChannel::SearchStateChangeDetails &details);
    void onSearchResultReceived(const Tp::ContactSearchChannel::SearchResult &result);
    void onSearchReturned(Tp::PendingOperation *op);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testContactSearch();
    void testContactSearchEmptyResult();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    TpHandleRepoIface *mContactRepo;

    ContactSearchChannelPtr mChan;
    ContactSearchChannelPtr mChan1;
    ContactSearchChannelPtr mChan2;

    QString mChan1Path;
    TpTestsContactSearchChannel *mChan1Service;
    QString mChan2Path;
    TpTestsContactSearchChannel *mChan2Service;

    ContactSearchChannel::SearchResult mSearchResult;
    bool mSearchReturned;

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

void TestContactSearchChan::onSearchReturned(Tp::PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    QVERIFY(mChan->searchState() != ChannelContactSearchStateNotStarted);
    mSearchReturned = true;
    mLoop->exit(0);
}

void TestContactSearchChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contact-search-chan");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);

    QByteArray chan1Path;
    mChan1Path = mConn->objectPath() + QLatin1String("/ContactSearchChannel/1");
    chan1Path = mChan1Path.toLatin1();
    mChan1Service = TP_TESTS_CONTACT_SEARCH_CHANNEL(g_object_new(
                TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL,
                "connection", mConn->service(),
                "object-path", chan1Path.data(),
                NULL));

    QByteArray chan2Path;
    mChan2Path = mConn->objectPath() + QLatin1String("/ContactSearchChannel/2");
    chan2Path = mChan2Path.toLatin1();
    mChan2Service = TP_TESTS_CONTACT_SEARCH_CHANNEL(g_object_new(
                TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL,
                "connection", mConn->service(),
                "object-path", chan2Path.data(),
                NULL));
}

void TestContactSearchChan::init()
{
    initImpl();
    mSearchResult.clear();
    mSearchStateChangeInfoList.clear();
    mSearchReturned = false;
}

void TestContactSearchChan::testContactSearch()
{
    mChan1 = ContactSearchChannel::create(mConn->client(), mChan1Path, QVariantMap());
    mChan = mChan1;
    // becomeReady with no args should implicitly enable ContactSearchChannel::FeatureCore
    QVERIFY(connect(mChan1->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan1->isReady(ContactSearchChannel::FeatureCore), true);

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

    QVERIFY(connect(mChan1->search(QLatin1String("employer"), QLatin1String("Collabora")),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onSearchReturned(Tp::PendingOperation *))));
    while (!mSearchReturned) {
        QCOMPARE(mLoop->exec(), 0);
    }
    while (mChan1->searchState() != ChannelContactSearchStateCompleted) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mSearchReturned, true);

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
    mChan2 = ContactSearchChannel::create(mConn->client(), mChan2Path, QVariantMap());
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
    QVERIFY(connect(mChan2->search(searchTerms),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onSearchReturned(Tp::PendingOperation *))));
    while (!mSearchReturned) {
        QCOMPARE(mLoop->exec(), 0);
    }
    while (mChan2->searchState() != ChannelContactSearchStateCompleted) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mSearchReturned, true);

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
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    if (mChan1Service != 0) {
        g_object_unref(mChan1Service);
        mChan1Service = 0;
    }

    if (mChan2Service != 0) {
        g_object_unref(mChan2Service);
        mChan2Service = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactSearchChan)
#include "_gen/contact-search-chan.cpp.moc.hpp"
