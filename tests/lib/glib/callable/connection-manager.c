/*
 * manager.c - an example connection manager
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

#include "connection-manager.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>

#include "conn.h"

G_DEFINE_TYPE (ExampleCallableConnectionManager,
    example_callable_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

struct _ExampleCallableConnectionManagerPrivate
{
  int dummy;
};

static void
example_callable_connection_manager_init (
    ExampleCallableConnectionManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALLABLE_CONNECTION_MANAGER,
      ExampleCallableConnectionManagerPrivate);
}

typedef struct {
    gchar *account;
    guint simulation_delay;
} ExampleParams;

static gboolean
account_param_filter (const TpCMParamSpec *paramspec,
                      GValue *value,
                      GError **error)
{
  const gchar *id = g_value_get_string (value);

  g_value_take_string (value,
      example_callable_normalize_contact (NULL, id, NULL, error));

  if (g_value_get_string (value) == NULL)
    return FALSE;

  return TRUE;
}

#include "_gen/param-spec-struct.h"

static gpointer
alloc_params (void)
{
  ExampleParams *params = g_slice_new0 (ExampleParams);

  params->simulation_delay = 1000;
  return params;
}

static void
free_params (gpointer p)
{
  ExampleParams *params = p;

  g_free (params->account);

  g_slice_free (ExampleParams, params);
}

static const TpCMProtocolSpec example_protocols[] = {
  { "example", example_callable_example_params,
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
  ExampleParams *params = parsed_params;
  ExampleCallableConnection *conn;

  conn = EXAMPLE_CALLABLE_CONNECTION
      (g_object_new (EXAMPLE_TYPE_CALLABLE_CONNECTION,
          "account", params->account,
          "simulation-delay", params->simulation_delay,
          "protocol", proto,
          NULL));

  return (TpBaseConnection *) conn;
}

static void
example_callable_connection_manager_class_init (
    ExampleCallableConnectionManagerClass *klass)
{
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  g_type_class_add_private (klass,
      sizeof (ExampleCallableConnectionManagerPrivate));

  base_class->new_connection = new_connection;
  base_class->cm_dbus_name = "example_callable";
  base_class->protocol_params = example_protocols;
}
