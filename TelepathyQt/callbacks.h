/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#ifndef _TelepathyQt_callbacks_h_HEADER_GUARD_
#define _TelepathyQt_callbacks_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Functors>
#include <TelepathyQt/Global>

namespace Tp
{

struct TP_QT_EXPORT AbstractFunctorCaller
{
    typedef void *(*HookType)(void*);

    AbstractFunctorCaller(HookType invokeMethodHook) : invokeMethodHook(invokeMethodHook) {}
    virtual ~AbstractFunctorCaller() {}

    virtual AbstractFunctorCaller *clone() const = 0;

    HookType invokeMethodHook;

private:
    AbstractFunctorCaller(const AbstractFunctorCaller &other);
    AbstractFunctorCaller &operator=(const AbstractFunctorCaller &other);
};

template <class T, class Functor>
struct BaseFunctorCaller : public AbstractFunctorCaller
{
    BaseFunctorCaller(const Functor &functor, AbstractFunctorCaller::HookType invokeMethodHook)
        : AbstractFunctorCaller(invokeMethodHook),
          functor(functor) {}
    virtual ~BaseFunctorCaller() {}

    virtual AbstractFunctorCaller *clone() const { return new T(functor); }

    Functor functor;
};

struct TP_QT_EXPORT BaseCallback
{
    BaseCallback() : caller(0) {}
    /* takes ownership of caller */
    BaseCallback(AbstractFunctorCaller *caller) : caller(caller) {}
    BaseCallback(const BaseCallback &other) : caller(other.caller->clone()) {}
    virtual ~BaseCallback() { delete caller; }

    bool isValid() const { return caller != 0; }

    BaseCallback &operator=(const BaseCallback &other)
    {
        if (caller == other.caller) return *this;
        delete caller;
        caller = other.caller->clone();
        return *this;
    }

protected:
    /* may be null */
    AbstractFunctorCaller *caller;
};

template <class Functor, class R >
struct FunctorCaller0 : public BaseFunctorCaller<FunctorCaller0<Functor, R >, Functor>
{
    typedef R ResultType;
    typedef R (*InvokeType)(AbstractFunctorCaller* );

    explicit FunctorCaller0(const Functor &functor) : BaseFunctorCaller<FunctorCaller0, Functor>(functor, reinterpret_cast<AbstractFunctorCaller::HookType>(&FunctorCaller0<Functor, R >::invoke)) {}

    static ResultType invoke(AbstractFunctorCaller *call )
    {
        typedef FunctorCaller0<Functor, R > Type;
        Type *typed = static_cast<Type*>(call);
        return (typed->functor)();
    }
};

template <class R >
struct Callback0 : public BaseCallback
{
    typedef R (*FunctionType)();
    typedef R ResultType;

    Callback0() {}
    template <class Functor>
    Callback0(const Functor &functor) : BaseCallback(new FunctorCaller0<Functor, R >(functor)) {}

    ResultType operator()() const
    {
        if (caller) {
            typedef R (*InvokeType)(AbstractFunctorCaller* );
            InvokeType invokeMethod = reinterpret_cast<InvokeType>(caller->invokeMethodHook);
            return invokeMethod(caller );
        }
        return ResultType();
    }
};

template <class Functor, class R , class Arg1>
struct FunctorCaller1 : public BaseFunctorCaller<FunctorCaller1<Functor, R , Arg1>, Functor>
{
    typedef R ResultType;
    typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1);

    explicit FunctorCaller1(const Functor &functor) : BaseFunctorCaller<FunctorCaller1, Functor>(functor, reinterpret_cast<AbstractFunctorCaller::HookType>(&FunctorCaller1<Functor, R , Arg1>::invoke)) {}

    static ResultType invoke(AbstractFunctorCaller *call , Arg1 a1)
    {
        typedef FunctorCaller1<Functor, R , Arg1> Type;
        Type *typed = static_cast<Type*>(call);
        return (typed->functor)(a1);
    }
};

template <class R , class Arg1>
struct Callback1 : public BaseCallback
{
    typedef R (*FunctionType)(Arg1);
    typedef R ResultType;

    Callback1() {}
    template <class Functor>
    Callback1(const Functor &functor) : BaseCallback(new FunctorCaller1<Functor, R , Arg1>(functor)) {}

    ResultType operator()(Arg1 a1) const
    {
        if (caller) {
            typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1);
            InvokeType invokeMethod = reinterpret_cast<InvokeType>(caller->invokeMethodHook);
            return invokeMethod(caller , a1);
        }
        return ResultType();
    }
};

template <class Functor, class R , class Arg1, class Arg2>
struct FunctorCaller2 : public BaseFunctorCaller<FunctorCaller2<Functor, R , Arg1, Arg2>, Functor>
{
    typedef R ResultType;
    typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2);

    explicit FunctorCaller2(const Functor &functor) : BaseFunctorCaller<FunctorCaller2, Functor>(functor, reinterpret_cast<AbstractFunctorCaller::HookType>(&FunctorCaller2<Functor, R , Arg1, Arg2>::invoke)) {}

    static ResultType invoke(AbstractFunctorCaller *call , Arg1 a1, Arg2 a2)
    {
        typedef FunctorCaller2<Functor, R , Arg1, Arg2> Type;
        Type *typed = static_cast<Type*>(call);
        return (typed->functor)(a1, a2);
    }
};

template <class R , class Arg1, class Arg2>
struct Callback2 : public BaseCallback
{
    typedef R (*FunctionType)(Arg1, Arg2);
    typedef R ResultType;

    Callback2() {}
    template <class Functor>
    Callback2(const Functor &functor) : BaseCallback(new FunctorCaller2<Functor, R , Arg1, Arg2>(functor)) {}

    ResultType operator()(Arg1 a1, Arg2 a2) const
    {
        if (caller) {
            typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2);
            InvokeType invokeMethod = reinterpret_cast<InvokeType>(caller->invokeMethodHook);
            return invokeMethod(caller , a1, a2);
        }
        return ResultType();
    }
};

template <class Functor, class R , class Arg1, class Arg2, class Arg3>
struct FunctorCaller3 : public BaseFunctorCaller<FunctorCaller3<Functor, R , Arg1, Arg2, Arg3>, Functor>
{
    typedef R ResultType;
    typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3);

    explicit FunctorCaller3(const Functor &functor) : BaseFunctorCaller<FunctorCaller3, Functor>(functor, reinterpret_cast<AbstractFunctorCaller::HookType>(&FunctorCaller3<Functor, R , Arg1, Arg2, Arg3>::invoke)) {}

    static ResultType invoke(AbstractFunctorCaller *call , Arg1 a1, Arg2 a2, Arg3 a3)
    {
        typedef FunctorCaller3<Functor, R , Arg1, Arg2, Arg3> Type;
        Type *typed = static_cast<Type*>(call);
        return (typed->functor)(a1, a2, a3);
    }
};

template <class R , class Arg1, class Arg2, class Arg3>
struct Callback3 : public BaseCallback
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3);
    typedef R ResultType;

    Callback3() {}
    template <class Functor>
    Callback3(const Functor &functor) : BaseCallback(new FunctorCaller3<Functor, R , Arg1, Arg2, Arg3>(functor)) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3) const
    {
        if (caller) {
            typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3);
            InvokeType invokeMethod = reinterpret_cast<InvokeType>(caller->invokeMethodHook);
            return invokeMethod(caller , a1, a2, a3);
        }
        return ResultType();
    }
};

template <class Functor, class R , class Arg1, class Arg2, class Arg3, class Arg4>
struct FunctorCaller4 : public BaseFunctorCaller<FunctorCaller4<Functor, R , Arg1, Arg2, Arg3, Arg4>, Functor>
{
    typedef R ResultType;
    typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3, Arg4);

    explicit FunctorCaller4(const Functor &functor) : BaseFunctorCaller<FunctorCaller4, Functor>(functor, reinterpret_cast<AbstractFunctorCaller::HookType>(&FunctorCaller4<Functor, R , Arg1, Arg2, Arg3, Arg4>::invoke)) {}

    static ResultType invoke(AbstractFunctorCaller *call , Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4)
    {
        typedef FunctorCaller4<Functor, R , Arg1, Arg2, Arg3, Arg4> Type;
        Type *typed = static_cast<Type*>(call);
        return (typed->functor)(a1, a2, a3, a4);
    }
};

template <class R , class Arg1, class Arg2, class Arg3, class Arg4>
struct Callback4 : public BaseCallback
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3, Arg4);
    typedef R ResultType;

    Callback4() {}
    template <class Functor>
    Callback4(const Functor &functor) : BaseCallback(new FunctorCaller4<Functor, R , Arg1, Arg2, Arg3, Arg4>(functor)) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) const
    {
        if (caller) {
            typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3, Arg4);
            InvokeType invokeMethod = reinterpret_cast<InvokeType>(caller->invokeMethodHook);
            return invokeMethod(caller , a1, a2, a3, a4);
        }
        return ResultType();
    }
};

template <class Functor, class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct FunctorCaller5 : public BaseFunctorCaller<FunctorCaller5<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5>, Functor>
{
    typedef R ResultType;
    typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3, Arg4, Arg5);

    explicit FunctorCaller5(const Functor &functor) : BaseFunctorCaller<FunctorCaller5, Functor>(functor, reinterpret_cast<AbstractFunctorCaller::HookType>(&FunctorCaller5<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5>::invoke)) {}

    static ResultType invoke(AbstractFunctorCaller *call , Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5)
    {
        typedef FunctorCaller5<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5> Type;
        Type *typed = static_cast<Type*>(call);
        return (typed->functor)(a1, a2, a3, a4, a5);
    }
};

template <class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct Callback5 : public BaseCallback
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5);
    typedef R ResultType;

    Callback5() {}
    template <class Functor>
    Callback5(const Functor &functor) : BaseCallback(new FunctorCaller5<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5>(functor)) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5) const
    {
        if (caller) {
            typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3, Arg4, Arg5);
            InvokeType invokeMethod = reinterpret_cast<InvokeType>(caller->invokeMethodHook);
            return invokeMethod(caller , a1, a2, a3, a4, a5);
        }
        return ResultType();
    }
};

template <class Functor, class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
struct FunctorCaller6 : public BaseFunctorCaller<FunctorCaller6<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>, Functor>
{
    typedef R ResultType;
    typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);

    explicit FunctorCaller6(const Functor &functor) : BaseFunctorCaller<FunctorCaller6, Functor>(functor, reinterpret_cast<AbstractFunctorCaller::HookType>(&FunctorCaller6<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>::invoke)) {}

    static ResultType invoke(AbstractFunctorCaller *call , Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6)
    {
        typedef FunctorCaller6<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> Type;
        Type *typed = static_cast<Type*>(call);
        return (typed->functor)(a1, a2, a3, a4, a5, a6);
    }
};

template <class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
struct Callback6 : public BaseCallback
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
    typedef R ResultType;

    Callback6() {}
    template <class Functor>
    Callback6(const Functor &functor) : BaseCallback(new FunctorCaller6<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>(functor)) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6) const
    {
        if (caller) {
            typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
            InvokeType invokeMethod = reinterpret_cast<InvokeType>(caller->invokeMethodHook);
            return invokeMethod(caller , a1, a2, a3, a4, a5, a6);
        }
        return ResultType();
    }
};

template <class Functor, class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
struct FunctorCaller7 : public BaseFunctorCaller<FunctorCaller7<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>, Functor>
{
    typedef R ResultType;
    typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);

    explicit FunctorCaller7(const Functor &functor) : BaseFunctorCaller<FunctorCaller7, Functor>(functor, reinterpret_cast<AbstractFunctorCaller::HookType>(&FunctorCaller7<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>::invoke)) {}

    static ResultType invoke(AbstractFunctorCaller *call , Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6, Arg7 a7)
    {
        typedef FunctorCaller7<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7> Type;
        Type *typed = static_cast<Type*>(call);
        return (typed->functor)(a1, a2, a3, a4, a5, a6, a7);
    }
};

template <class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
struct Callback7 : public BaseCallback
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);
    typedef R ResultType;

    Callback7() {}
    template <class Functor>
    Callback7(const Functor &functor) : BaseCallback(new FunctorCaller7<Functor, R , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>(functor)) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6, Arg7 a7) const
    {
        if (caller) {
            typedef R (*InvokeType)(AbstractFunctorCaller* , Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);
            InvokeType invokeMethod = reinterpret_cast<InvokeType>(caller->invokeMethodHook);
            return invokeMethod(caller , a1, a2, a3, a4, a5, a6, a7);
        }
        return ResultType();
    }
};

}

#endif
