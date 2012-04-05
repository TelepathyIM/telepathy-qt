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

#ifndef _TelepathyQt_functors_h_HEADER_GUARD_
#define _TelepathyQt_functors_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>

namespace Tp
{

struct TP_QT_EXPORT BaseFunctor
{
};

template <class R >
struct PtrFunctor0 : public BaseFunctor
{
    typedef R (*FunctionType)();
    typedef R ResultType;

    PtrFunctor0(FunctionType fn) : fn(fn) {}

    ResultType operator()() const { return fn(); }

    FunctionType fn;
};

template <class R, class T >
struct MemberFunctor0 : public BaseFunctor
{
    typedef R (T::*FunctionType)();
    typedef R ResultType;

    MemberFunctor0(T *object, FunctionType fn) : object(object), fn(fn) {}

    ResultType operator()() const { return (object->*(fn))(); }

    T *object;
    FunctionType fn;
};

template <class R , class Arg1>
struct PtrFunctor1 : public BaseFunctor
{
    typedef R (*FunctionType)(Arg1);
    typedef R ResultType;

    PtrFunctor1(FunctionType fn) : fn(fn) {}

    ResultType operator()(Arg1 a1) const { return fn(a1); }

    FunctionType fn;
};

template <class R, class T , class Arg1>
struct MemberFunctor1 : public BaseFunctor
{
    typedef R (T::*FunctionType)(Arg1);
    typedef R ResultType;

    MemberFunctor1(T *object, FunctionType fn) : object(object), fn(fn) {}

    ResultType operator()(Arg1 a1) const { return (object->*(fn))(a1); }

    T *object;
    FunctionType fn;
};

template <class R , class Arg1, class Arg2>
struct PtrFunctor2 : public BaseFunctor
{
    typedef R (*FunctionType)(Arg1, Arg2);
    typedef R ResultType;

    PtrFunctor2(FunctionType fn) : fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2) const { return fn(a1, a2); }

    FunctionType fn;
};

template <class R, class T , class Arg1, class Arg2>
struct MemberFunctor2 : public BaseFunctor
{
    typedef R (T::*FunctionType)(Arg1, Arg2);
    typedef R ResultType;

    MemberFunctor2(T *object, FunctionType fn) : object(object), fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2) const { return (object->*(fn))(a1, a2); }

    T *object;
    FunctionType fn;
};

template <class R , class Arg1, class Arg2, class Arg3>
struct PtrFunctor3 : public BaseFunctor
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3);
    typedef R ResultType;

    PtrFunctor3(FunctionType fn) : fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3) const { return fn(a1, a2, a3); }

    FunctionType fn;
};

template <class R, class T , class Arg1, class Arg2, class Arg3>
struct MemberFunctor3 : public BaseFunctor
{
    typedef R (T::*FunctionType)(Arg1, Arg2, Arg3);
    typedef R ResultType;

    MemberFunctor3(T *object, FunctionType fn) : object(object), fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3) const { return (object->*(fn))(a1, a2, a3); }

    T *object;
    FunctionType fn;
};

template <class R , class Arg1, class Arg2, class Arg3, class Arg4>
struct PtrFunctor4 : public BaseFunctor
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3, Arg4);
    typedef R ResultType;

    PtrFunctor4(FunctionType fn) : fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) const { return fn(a1, a2, a3, a4); }

    FunctionType fn;
};

template <class R, class T , class Arg1, class Arg2, class Arg3, class Arg4>
struct MemberFunctor4 : public BaseFunctor
{
    typedef R (T::*FunctionType)(Arg1, Arg2, Arg3, Arg4);
    typedef R ResultType;

    MemberFunctor4(T *object, FunctionType fn) : object(object), fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) const { return (object->*(fn))(a1, a2, a3, a4); }

    T *object;
    FunctionType fn;
};

template <class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct PtrFunctor5 : public BaseFunctor
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5);
    typedef R ResultType;

    PtrFunctor5(FunctionType fn) : fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5) const { return fn(a1, a2, a3, a4, a5); }

    FunctionType fn;
};

template <class R, class T , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct MemberFunctor5 : public BaseFunctor
{
    typedef R (T::*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5);
    typedef R ResultType;

    MemberFunctor5(T *object, FunctionType fn) : object(object), fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5) const { return (object->*(fn))(a1, a2, a3, a4, a5); }

    T *object;
    FunctionType fn;
};

template <class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
struct PtrFunctor6 : public BaseFunctor
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
    typedef R ResultType;

    PtrFunctor6(FunctionType fn) : fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6) const { return fn(a1, a2, a3, a4, a5, a6); }

    FunctionType fn;
};

template <class R, class T , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
struct MemberFunctor6 : public BaseFunctor
{
    typedef R (T::*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
    typedef R ResultType;

    MemberFunctor6(T *object, FunctionType fn) : object(object), fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6) const { return (object->*(fn))(a1, a2, a3, a4, a5, a6); }

    T *object;
    FunctionType fn;
};

template <class R , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
struct PtrFunctor7 : public BaseFunctor
{
    typedef R (*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);
    typedef R ResultType;

    PtrFunctor7(FunctionType fn) : fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6, Arg7 a7) const { return fn(a1, a2, a3, a4, a5, a6, a7); }

    FunctionType fn;
};

template <class R, class T , class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
struct MemberFunctor7 : public BaseFunctor
{
    typedef R (T::*FunctionType)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);
    typedef R ResultType;

    MemberFunctor7(T *object, FunctionType fn) : object(object), fn(fn) {}

    ResultType operator()(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6, Arg7 a7) const { return (object->*(fn))(a1, a2, a3, a4, a5, a6, a7); }

    T *object;
    FunctionType fn;
};

// convenience methods

// ptrFun

template <class R>
inline PtrFunctor0<R>
ptrFun(R (*fn)() )
{ return PtrFunctor0<R>(fn); }

template <class R, class Arg1>
inline PtrFunctor1<R, Arg1>
ptrFun(R (*fn)(Arg1) )
{ return PtrFunctor1<R, Arg1>(fn); }

template <class R, class Arg1, class Arg2>
inline PtrFunctor2<R, Arg1, Arg2>
ptrFun(R (*fn)(Arg1, Arg2) )
{ return PtrFunctor2<R, Arg1, Arg2>(fn); }

template <class R, class Arg1, class Arg2, class Arg3>
inline PtrFunctor3<R, Arg1, Arg2, Arg3>
ptrFun(R (*fn)(Arg1, Arg2, Arg3) )
{ return PtrFunctor3<R, Arg1, Arg2, Arg3>(fn); }

template <class R, class Arg1, class Arg2, class Arg3, class Arg4>
inline PtrFunctor4<R, Arg1, Arg2, Arg3, Arg4>
ptrFun(R (*fn)(Arg1, Arg2, Arg3, Arg4) )
{ return PtrFunctor4<R, Arg1, Arg2, Arg3, Arg4>(fn); }

template <class R, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
inline PtrFunctor5<R, Arg1, Arg2, Arg3, Arg4, Arg5>
ptrFun(R (*fn)(Arg1, Arg2, Arg3, Arg4, Arg5) )
{ return PtrFunctor5<R, Arg1, Arg2, Arg3, Arg4, Arg5>(fn); }

template <class R, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
inline PtrFunctor6<R, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>
ptrFun(R (*fn)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) )
{ return PtrFunctor6<R, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>(fn); }

template <class R, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
inline PtrFunctor7<R, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>
ptrFun(R (*fn)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7) )
{ return PtrFunctor7<R, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>(fn); }

// memFun

template <class R, class T>
inline MemberFunctor0<R, T>
memFun(/**/ T *obj, R (T::*fn)() )
{ return MemberFunctor0<R, T>(obj, fn); }

template <class R, class T, class Arg1>
inline MemberFunctor1<R, T, Arg1>
memFun(/**/ T *obj, R (T::*fn)(Arg1) )
{ return MemberFunctor1<R, T, Arg1>(obj, fn); }

template <class R, class T, class Arg1, class Arg2>
inline MemberFunctor2<R, T, Arg1, Arg2>
memFun(/**/ T *obj, R (T::*fn)(Arg1, Arg2) )
{ return MemberFunctor2<R, T, Arg1, Arg2>(obj, fn); }

template <class R, class T, class Arg1, class Arg2, class Arg3>
inline MemberFunctor3<R, T, Arg1, Arg2, Arg3>
memFun(/**/ T *obj, R (T::*fn)(Arg1, Arg2, Arg3) )
{ return MemberFunctor3<R, T, Arg1, Arg2, Arg3>(obj, fn); }

template <class R, class T, class Arg1, class Arg2, class Arg3, class Arg4>
inline MemberFunctor4<R, T, Arg1, Arg2, Arg3, Arg4>
memFun(/**/ T *obj, R (T::*fn)(Arg1, Arg2, Arg3, Arg4) )
{ return MemberFunctor4<R, T, Arg1, Arg2, Arg3, Arg4>(obj, fn); }

template <class R, class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
inline MemberFunctor5<R, T, Arg1, Arg2, Arg3, Arg4, Arg5>
memFun(/**/ T *obj, R (T::*fn)(Arg1, Arg2, Arg3, Arg4, Arg5) )
{ return MemberFunctor5<R, T, Arg1, Arg2, Arg3, Arg4, Arg5>(obj, fn); }

template <class R, class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
inline MemberFunctor6<R, T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>
memFun(/**/ T *obj, R (T::*fn)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) )
{ return MemberFunctor6<R, T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>(obj, fn); }

template <class R, class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
inline MemberFunctor7<R, T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>
memFun(/**/ T *obj, R (T::*fn)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7) )
{ return MemberFunctor7<R, T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>(obj, fn); }

}

#endif
