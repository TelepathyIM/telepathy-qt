/*
 * params-cm.h - source for TpTestsParamConnectionManager
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

#include "params-cm.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>

G_DEFINE_TYPE (TpTestsParamConnectionManager,
    tp_tests_param_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

struct _TpTestsParamConnectionManagerPrivate
{
  int dummy;
};

static void
tp_tests_param_connection_manager_init (
    TpTestsParamConnectionManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TP_TESTS_TYPE_PARAM_CONNECTION_MANAGER,
      TpTestsParamConnectionManagerPrivate);
}

enum {
    TP_TESTS_PARAM_STRING,
    TP_TESTS_PARAM_INT16,
    TP_TESTS_PARAM_INT32,
    TP_TESTS_PARAM_UINT16,
    TP_TESTS_PARAM_UINT32,
    TP_TESTS_PARAM_INT64,
    TP_TESTS_PARAM_UINT64,
    TP_TESTS_PARAM_BOOLEAN,
    TP_TESTS_PARAM_DOUBLE,
    TP_TESTS_PARAM_ARRAY_STRINGS,
    TP_TESTS_PARAM_ARRAY_BYTES,
    TP_TESTS_PARAM_OBJECT_PATH,
    TP_TESTS_PARAM_LC_STRING,
    TP_TESTS_PARAM_UC_STRING,
    NUM_PARAM
};

static gboolean
filter_string_ascii_case (const TpCMParamSpec *param_spec,
    GValue *value,
    GError **error)
{
  const gchar *s = g_value_get_string (value);
  guint i;

  for (i = 0; s[i] != '\0'; i++)
    {
      int c = s[i];           /* just to avoid -Wtype-limits */

      if (c < 0 || c > 127)   /* char might be signed or unsigned */
        {
          g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
              "%s must be ASCII", param_spec->name);
          return FALSE;
        }
    }

  if (GINT_TO_POINTER (param_spec->filter_data))
    g_value_take_string (value, g_ascii_strup (s, -1));
  else
    g_value_take_string (value, g_ascii_strdown (s, -1));

  return TRUE;
}

static TpCMParamSpec param_example_params[] = {
  { "a-string", "s", G_TYPE_STRING,
    TP_CONN_MGR_PARAM_FLAG_HAS_DEFAULT, "the default string",
    G_STRUCT_OFFSET (TpTestsCMParams, a_string), NULL, NULL, NULL },
  { "a-int16", "n", G_TYPE_INT,
    TP_CONN_MGR_PARAM_FLAG_HAS_DEFAULT, GINT_TO_POINTER (42),
    G_STRUCT_OFFSET (TpTestsCMParams, a_int16), NULL, NULL, NULL },
  { "a-int32", "i", G_TYPE_INT,
    TP_CONN_MGR_PARAM_FLAG_HAS_DEFAULT, GINT_TO_POINTER (42),
    G_STRUCT_OFFSET (TpTestsCMParams, a_int32), NULL, NULL, NULL },
  { "a-uint16", "q", G_TYPE_UINT, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_uint16), NULL, NULL, NULL },
  { "a-uint32", "u", G_TYPE_UINT, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_uint32), NULL, NULL, NULL },
  { "a-int64", "x", G_TYPE_INT64, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_int64), NULL, NULL, NULL },
  { "a-uint64", "t", G_TYPE_UINT64, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_uint64), NULL, NULL, NULL },
  { "a-boolean", "b", G_TYPE_BOOLEAN, TP_CONN_MGR_PARAM_FLAG_REQUIRED, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_boolean), NULL, NULL, NULL },
  { "a-double", "d", G_TYPE_DOUBLE, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_double), NULL, NULL, NULL },
  { "a-array-of-strings", "as", 0, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_array_of_strings), NULL, NULL, NULL },
  { "a-array-of-bytes", "ay", 0, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_array_of_bytes), NULL, NULL, NULL },
  { "a-object-path", "o", 0, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, a_object_path), NULL, NULL, NULL },

  /* demo of a filter */
  { "lc-string", "s", G_TYPE_STRING, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, lc_string),
    filter_string_ascii_case, GINT_TO_POINTER (FALSE), NULL },
  { "uc-string", "s", G_TYPE_STRING, 0, NULL,
    G_STRUCT_OFFSET (TpTestsCMParams, uc_string),
    filter_string_ascii_case, GINT_TO_POINTER (TRUE), NULL },
  { NULL }
};

static TpTestsCMParams *params = NULL;

static gpointer
alloc_params (void)
{
  params = g_slice_new0 (TpTestsCMParams);

  return params;
}

static void
free_params (gpointer p)
{
  /* CM user is responsible to free params so he can check their values */
  params = (TpTestsCMParams *) p;
  params->would_have_been_freed = TRUE;
}

static const TpCMProtocolSpec example_protocols[] = {
  { "example", param_example_params,
    alloc_params, free_params },
  { NULL, NULL }
};

static TpBaseConnection *
new_connection (TpBaseConnectionManager *self,
                const gchar *proto,
                TpIntSet *params_present,
                gpointer parsed_params,
                GError **error)
{
  g_set_error (error, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED,
      "No connection for you");
  return NULL;
}

static void
tp_tests_param_connection_manager_class_init (
    TpTestsParamConnectionManagerClass *klass)
{
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  g_type_class_add_private (klass,
      sizeof (TpTestsParamConnectionManagerPrivate));

  param_example_params[TP_TESTS_PARAM_ARRAY_STRINGS].gtype = G_TYPE_STRV;
  param_example_params[TP_TESTS_PARAM_ARRAY_BYTES].gtype =
      DBUS_TYPE_G_UCHAR_ARRAY;
  param_example_params[TP_TESTS_PARAM_OBJECT_PATH].gtype =
      DBUS_TYPE_G_OBJECT_PATH;

  base_class->new_connection = new_connection;
  base_class->cm_dbus_name = "params_cm";
  base_class->protocol_params = example_protocols;
}

TpTestsCMParams *
tp_tests_param_connection_manager_steal_params_last_conn (void)
{
  TpTestsCMParams *p = params;

  params = NULL;
  return p;
}

void
tp_tests_param_connection_manager_free_params (TpTestsCMParams *p)
{
  g_free (p->a_string);
  g_strfreev (p->a_array_of_strings);
  if (p->a_array_of_bytes != NULL)
    g_array_free (p->a_array_of_bytes, TRUE);
  g_free (p->a_object_path);

  g_slice_free (TpTestsCMParams, p);
}
