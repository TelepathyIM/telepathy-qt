/*
 * manager.c - an example connection manager
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "connection-manager.h"

#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>

#include <telepathy-glib/telepathy-glib.h>

#include "conn.h"

G_DEFINE_TYPE (ExampleContactListConnectionManager,
    example_contact_list_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

struct _ExampleContactListConnectionManagerPrivate
{
  int dummy;
};

static void
example_contact_list_connection_manager_init (
    ExampleContactListConnectionManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CONTACT_LIST_CONNECTION_MANAGER,
      ExampleContactListConnectionManagerPrivate);
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
      example_contact_list_normalize_contact (NULL, id, NULL, error));

  if (g_value_get_string (value) == NULL)
    return FALSE;

  return TRUE;
}

#include "_gen/param-spec-struct.h"

static gpointer
alloc_params (void)
{
  return g_slice_new0 (ExampleParams);
}

static void
free_params (gpointer p)
{
  ExampleParams *params = p;

  g_free (params->account);

  g_slice_free (ExampleParams, params);
}

static const TpCMProtocolSpec example_protocols[] = {
  { "example", example_contact_list_example_params,
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
  ExampleContactListConnection *conn;

  conn = EXAMPLE_CONTACT_LIST_CONNECTION
      (g_object_new (EXAMPLE_TYPE_CONTACT_LIST_CONNECTION,
          "account", params->account,
          "simulation-delay", params->simulation_delay,
          "protocol", proto,
          NULL));

  return (TpBaseConnection *) conn;
}

static void
example_contact_list_connection_manager_class_init (
    ExampleContactListConnectionManagerClass *klass)
{
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  g_type_class_add_private (klass,
      sizeof (ExampleContactListConnectionManagerPrivate));

  base_class->new_connection = new_connection;
  base_class->cm_dbus_name = "example_contact_list";
  base_class->protocol_params = example_protocols;
}
