/**
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2009-2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_Farstream_global_h_HEADER_GUARD_
#define _TelepathyQt_Farstream_global_h_HEADER_GUARD_

#ifndef IN_TP_QT_FARSTREAM_HEADER
#error IN_TP_QT_FARSTREAM_HEADER
#endif

#include <QtGlobal>

#ifdef BUILDING_TP_QT_FARSTREAM
#  define TP_QT_FS_EXPORT Q_DECL_EXPORT
#else
#  define TP_QT_FS_EXPORT Q_DECL_IMPORT
#endif

#if !defined(Q_OS_WIN) && defined(QT_VISIBILITY_AVAILABLE)
#  define TP_QT_FS_NO_EXPORT __attribute__((visibility("hidden")))
#endif

#ifndef TP_QT_FS_NO_EXPORT
#  define TP_QT_FS_NO_EXPORT
#endif

#endif
