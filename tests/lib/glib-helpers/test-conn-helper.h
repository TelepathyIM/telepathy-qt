#ifndef _TelepathyQt4_tests_lib_glib_helpers_test_conn_helper_h_HEADER_GUARD_
#define _TelepathyQt4_tests_lib_glib_helpers_test_conn_helper_h_HEADER_GUARD_

#include <tests/lib/test.h>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Features>
#include <TelepathyQt4/Types>

#include <glib-object.h>

#include <telepathy-glib/telepathy-glib.h>

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
    bool disconnectWithDBusError(const char *errorName, GHashTable *details,
            Tp::ConnectionStatusReason reason);

private Q_SLOTS:
    void expectConnInvalidated();

private:
    void init(Test *parent,
            const Tp::ChannelFactoryConstPtr &channelFactory,
            const Tp::ContactFactoryConstPtr &contactFactory,
            GType gType, const char *firstPropertyName, ...);
    void init(Test *parent,
            const Tp::ChannelFactoryConstPtr &channelFactory,
            const Tp::ContactFactoryConstPtr &contactFactory,
            GType gType, const char *firstPropertyName, va_list varArgs);

    Test *mParent;
    QEventLoop *mLoop;
    GObject *mService;
    Tp::ConnectionPtr mClient;
};

#endif // _TelepathyQt4_tests_lib_glib_helpers_test_conn_helper_h_HEADER_GUARD_
