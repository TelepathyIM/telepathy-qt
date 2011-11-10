/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
 * @license LGPL 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _TelepathyQt_method_invocation_context_h_HEADER_GUARD_
#define _TelepathyQt_method_invocation_context_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <QtDBus>
#include <QtCore>

namespace Tp
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace MethodInvocationContextTypes
{

struct Nil
{
};

template<int Index,
         typename T1, typename T2, typename T3, typename T4,
         typename T5, typename T6, typename T7, typename T8>
struct Select
{
    typedef Select<Index - 1, T2, T3, T4, T5, T6, T7, T8, Nil> Next;
    typedef typename Next::Type Type;
};
template<typename T1, typename T2, typename T3, typename T4,
         typename T5, typename T6, typename T7, typename T8>
struct Select<0, T1, T2, T3, T4, T5, T6, T7, T8>
{
    typedef T1 Type;
};

template<typename T1, typename T2, typename T3, typename T4,
         typename T5, typename T6, typename T7, typename T8>
struct ForEach
{
    typedef ForEach<T2, T3, T4, T5, T6, T7, T8, Nil> Next;
    enum { Total = Next::Total + 1 };
};
template<>
struct ForEach<Nil, Nil, Nil, Nil, Nil, Nil, Nil, Nil>
{
    enum { Total = 0 };
};

}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

template<typename T1 = MethodInvocationContextTypes::Nil, typename T2 = MethodInvocationContextTypes::Nil,
         typename T3 = MethodInvocationContextTypes::Nil, typename T4 = MethodInvocationContextTypes::Nil,
         typename T5 = MethodInvocationContextTypes::Nil, typename T6 = MethodInvocationContextTypes::Nil,
         typename T7 = MethodInvocationContextTypes::Nil, typename T8 = MethodInvocationContextTypes::Nil>
class MethodInvocationContext : public RefCounted
{
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    template<int Index>
    struct Select : MethodInvocationContextTypes::Select<Index, T1, T2, T3, T4, T5, T6, T7, T8>
    {
    };

    typedef MethodInvocationContextTypes::ForEach<T1, T2, T3, T4, T5, T6, T7, T8> ForEach;
    enum { Count = ForEach::Total };
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
    MethodInvocationContext(const QDBusConnection &bus, const QDBusMessage &message)
        : mBus(bus), mMessage(message), mFinished(false)
    {
        mMessage.setDelayedReply(true);
    }

    virtual ~MethodInvocationContext()
    {
        if (!mFinished) {
            setFinishedWithError(QString(), QString());
        }
    }

    bool isFinished() const { return mFinished; }
    bool isError() const { return !mErrorName.isEmpty(); }
    QString errorName() const { return mErrorName; }
    QString errorMessage() const { return mErrorMessage; }

    void setFinished(const T1 &t1 = T1(), const T2 &t2 = T2(), const T3 &t3 = T3(),
                     const T4 &t4 = T4(), const T5 &t5 = T5(), const T6 &t6 = T6(),
                     const T7 &t7 = T7(), const T8 &t8 = T8())
    {
        if (mFinished) {
            return;
        }

        mFinished = true;

        setReplyValue(0, qVariantFromValue(t1));
        setReplyValue(1, qVariantFromValue(t2));
        setReplyValue(2, qVariantFromValue(t3));
        setReplyValue(3, qVariantFromValue(t4));
        setReplyValue(4, qVariantFromValue(t5));
        setReplyValue(5, qVariantFromValue(t6));
        setReplyValue(6, qVariantFromValue(t7));
        setReplyValue(7, qVariantFromValue(t8));

        if (mReply.isEmpty()) {
            mBus.send(mMessage.createReply());
        } else {
            mBus.send(mMessage.createReply(mReply));
        }
        onFinished();
    }

    void setFinishedWithError(const QString &errorName,
            const QString &errorMessage)
    {
        if (mFinished) {
            return;
        }

        mFinished = true;

        if (errorName.isEmpty()) {
            mErrorName = QLatin1String("org.freedesktop.Telepathy.Qt.ErrorHandlingError");
        } else {
            mErrorName = errorName;
        }
        mErrorMessage = errorMessage;

        mBus.send(mMessage.createErrorReply(mErrorName, mErrorMessage));
        onFinished();
    }

    template<int Index> inline
    typename Select<Index>::Type argumentAt() const
    {
        Q_ASSERT(Index >= 0 && Index < Count);
        return qdbus_cast<typename Select<Index>::Type>(mReply.value(Index));
    }

protected:
    virtual void onFinished() {}

private:
    Q_DISABLE_COPY(MethodInvocationContext)

    void setReplyValue(int index, const QVariant &value)
    {
        if (index >= Count) {
            return;
        }
        mReply.insert(index, value);
    }

    QDBusConnection mBus;
    QDBusMessage mMessage;
    bool mFinished;
    QList<QVariant> mReply;
    QString mErrorName;
    QString mErrorMessage;
};

} // Tp

Q_DECLARE_METATYPE(Tp::MethodInvocationContextTypes::Nil)

#endif
