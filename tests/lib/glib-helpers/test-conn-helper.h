#ifndef _TelepathyQt_tests_lib_glib_helpers_test_conn_helper_h_HEADER_GUARD_
#define _TelepathyQt_tests_lib_glib_helpers_test_conn_helper_h_HEADER_GUARD_

#include <tests/lib/test.h>

#include <TelepathyQt/Constants>
#include <TelepathyQt/Features>
#include <TelepathyQt/Types>

#include <glib-object.h>

#include <telepathy-glib/telepathy-glib.h>

namespace Tp
{
class PendingContacts;
}

class TestConnHelper : public QObject
{
    Q_OBJECT

public:
    TestConnHelper(Test *parent,
            GType gType, const QString &account, const QString &protocol);
    TestConnHelper(Test *parent,
            GType gType, const char *firstPropertyName, ...);
    TestConnHelper(Test *parent,
            const Tp::ChannelFactoryConstPtr &channelFactory,
            const Tp::ContactFactoryConstPtr &contactFactory,
            GType gType, const QString &account, const QString &protocol);
    TestConnHelper(Test *parent,
            const Tp::ChannelFactoryConstPtr &channelFactory,
            const Tp::ContactFactoryConstPtr &contactFactory,
            GType gType, const char *firstPropertyName, ...);

    virtual ~TestConnHelper();

    GObject *service() const { return mService; }
    Tp::ConnectionPtr client() const { return mClient; }
    QString objectPath() const;

    bool isValid() const;
    bool isReady(const Tp::Features &features = Tp::Features()) const;
    bool enableFeatures(const Tp::Features &features);
    bool connect(const Tp::Features &features = Tp::Features());
    bool disconnect();

    QList<Tp::ContactPtr> contacts(const QStringList &ids,
            const Tp::Features &features = Tp::Features());
    QList<Tp::ContactPtr> contacts(const Tp::UIntList &handles,
            const Tp::Features &features = Tp::Features());
    QList<Tp::ContactPtr> upgradeContacts(const QList<Tp::ContactPtr> &contacts,
            const Tp::Features &features = Tp::Features());

    Tp::ChannelPtr createChannel(const QVariantMap &request);
    Tp::ChannelPtr createChannel(const QString &channelType, const Tp::ContactPtr &target);
    Tp::ChannelPtr createChannel(const QString &channelType,
            Tp::HandleType targetHandleType, uint targetHandle);
    Tp::ChannelPtr createChannel(const QString &channelType,
            Tp::HandleType targetHandleType, const QString &targetID);
    Tp::ChannelPtr ensureChannel(const QVariantMap &request);
    Tp::ChannelPtr ensureChannel(const QString &channelType, const Tp::ContactPtr &target);
    Tp::ChannelPtr ensureChannel(const QString &channelType,
            Tp::HandleType targetHandleType, uint targetHandle);
    Tp::ChannelPtr ensureChannel(const QString &channelType,
            Tp::HandleType targetHandleType, const QString &targetID);

private Q_SLOTS:
    void expectConnInvalidated();
    void expectContactsForIdentifiersFinished(Tp::PendingOperation *op);
    void expectContactsForHandlesFinished(Tp::PendingOperation *op);
    void expectUpgradeContactsFinished(Tp::PendingOperation *op);
    void expectCreateChannelFinished(Tp::PendingOperation *op);
    void expectEnsureChannelFinished(Tp::PendingOperation *op);

private:
    void init(Test *parent,
            const Tp::ChannelFactoryConstPtr &channelFactory,
            const Tp::ContactFactoryConstPtr &contactFactory,
            GType gType, const char *firstPropertyName, ...);
    void init(Test *parent,
            const Tp::ChannelFactoryConstPtr &channelFactory,
            const Tp::ContactFactoryConstPtr &contactFactory,
            GType gType, const char *firstPropertyName, va_list varArgs);

    void expectPendingContactsFinished(Tp::PendingContacts *pc);

    Test *mParent;
    QEventLoop *mLoop;
    GObject *mService;
    Tp::ConnectionPtr mClient;

    // The property retrieved by expectPendingContactsFinished()
    QList<Tp::ContactPtr> mContacts;
    // Property used by expectPendingContactsFinished()
    Tp::Features mContactFeatures;
    // The property retrieved by expectCreate/EnsureChannelFinished()
    Tp::ChannelPtr mChannel;
};

#endif // _TelepathyQt_tests_lib_glib_helpers_test_conn_helper_h_HEADER_GUARD_
