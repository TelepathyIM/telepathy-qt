/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_global_h_HEADER_GUARD_
#define _TelepathyQt4_global_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QtGlobal>

#ifdef BUILDING_TELEPATHY_QT4
#  define TELEPATHY_QT4_EXPORT Q_DECL_EXPORT
#else
#  define TELEPATHY_QT4_EXPORT Q_DECL_IMPORT
#endif

#if !defined(Q_OS_WIN) && defined(QT_VISIBILITY_AVAILABLE)
#  define TELEPATHY_QT4_NO_EXPORT __attribute__((visibility("hidden")))
#endif

#ifndef TELEPATHY_QT4_NO_EXPORT
#  define TELEPATHY_QT4_NO_EXPORT
#endif

/**
 * @def TELEPATHY_QT4_DEPRECATED
 * @ingroup TELEPATHY_QT4_MACROS
 *
 * The TELEPATHY_QT4_DEPRECATED macro can be used to trigger compile-time
 * warnings with newer compilers when deprecated functions are used.
 *
 * For non-inline functions, the macro gets inserted at front of the
 * function declaration, right before the return type:
 *
 * \code
 * TELEPATHY_QT4_DEPRECATED void deprecatedFunctionA();
 * TELEPATHY_QT4_DEPRECATED int deprecatedFunctionB() const;
 * \endcode
 *
 * For functions which are implemented inline,
 * the TELEPATHY_QT4_DEPRECATED macro is inserted at the front, right before the
 * return type, but after "static", "inline" or "virtual":
 *
 * \code
 * TELEPATHY_QT4_DEPRECATED void deprecatedInlineFunctionA() { .. }
 * virtual TELEPATHY_QT4_DEPRECATED int deprecatedInlineFunctionB() { .. }
 * static TELEPATHY_QT4_DEPRECATED bool deprecatedInlineFunctionC() { .. }
 * inline TELEPATHY_QT4_DEPRECATED bool deprecatedInlineFunctionD() { .. }
 * \endcode
 *
 * You can also mark whole structs or classes as deprecated, by inserting the
 * TELEPATHY_QT4_DEPRECATED macro after the struct/class keyword, but before the
 * name of the struct/class:
 *
 * \code
 * class TELEPATHY_QT4_DEPRECATED DeprecatedClass { };
 * struct TELEPATHY_QT4_DEPRECATED DeprecatedStruct { };
 * \endcode
 *
 * \note
 * It does not make much sense to use the TELEPATHY_QT4_DEPRECATED keyword for a
 * Qt signal; this is because usually get called by the class which they belong
 * to, and one would assume that a class author does not use deprecated methods
 * of his own class. The only exception to this are signals which are connected
 * to other signals; they get invoked from moc-generated code. In any case,
 * printing a warning message in either case is not useful.
 * For slots, it can make sense (since slots can be invoked directly) but be
 * aware that if the slots get triggered by a signal, they will get called from
 * moc code as well and thus the warnings are useless.
 *
 * \note
 * TELEPATHY_QT4_DEPRECATED cannot be used for constructors.
 */
#ifndef TELEPATHY_QT4_DEPRECATED
#  ifdef TELEPATHY_QT4_DEPRECATED_WARNINGS
#    define TELEPATHY_QT4_DEPRECATED Q_DECL_DEPRECATED
#  else
#    define TELEPATHY_QT4_DEPRECATED
#  endif
#endif

#endif
