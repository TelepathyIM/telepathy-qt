/*
 * params-cm.h - header for TpTestsParamConnectionManager
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
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

#ifndef __TP_TESTS_PARAM_CONNECTION_MANAGER_H__
#define __TP_TESTS_PARAM_CONNECTION_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection-manager.h>

G_BEGIN_DECLS

typedef struct _TpTestsParamConnectionManager
    TpTestsParamConnectionManager;
typedef struct _TpTestsParamConnectionManagerPrivate
    TpTestsParamConnectionManagerPrivate;

typedef struct _TpTestsParamConnectionManagerClass
    TpTestsParamConnectionManagerClass;
typedef struct _TpTestsParamConnectionManagerClassPrivate
    TpTestsParamConnectionManagerClassPrivate;

struct _TpTestsParamConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;

    TpTestsParamConnectionManagerClassPrivate *priv;
};

struct _TpTestsParamConnectionManager {
    TpBaseConnectionManager parent;

    TpTestsParamConnectionManagerPrivate *priv;
};

GType tp_tests_param_connection_manager_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_PARAM_CONNECTION_MANAGER \
  (tp_tests_param_connection_manager_get_type ())
#define TP_TESTS_PARAM_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_PARAM_CONNECTION_MANAGER, \
                              TpTestsParamConnectionManager))
#define TP_TESTS_PARAM_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_PARAM_CONNECTION_MANAGER, \
                           TpTestsParamConnectionManagerClass))
#define IS_PARAM_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_PARAM_CONNECTION_MANAGER))
#define IS_PARAM_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_PARAM_CONNECTION_MANAGER))
#define TP_TESTS_PARAM_CONNECTION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_PARAM_CONNECTION_MANAGER, \
                              TpTestsParamConnectionManagerClass))

typedef struct {
    gchar *a_string;
    gint a_int16;
    gint a_int32;
    guint a_uint16;
    guint a_uint32;
    gint64 a_int64;
    guint64 a_uint64;
    gboolean a_boolean;
    gdouble a_double;
    GStrv a_array_of_strings;
    GArray *a_array_of_bytes;
    gchar *a_object_path;
    gchar *lc_string;
    gchar *uc_string;
    gboolean would_have_been_freed;
} TpTestsCMParams;

TpTestsCMParams * tp_tests_param_connection_manager_steal_params_last_conn (
    void);
void tp_tests_param_connection_manager_free_params (TpTestsCMParams *params);

G_END_DECLS

#endif /* #ifndef __TP_TESTS_PARAM_CONNECTION_MANAGER_H__ */
