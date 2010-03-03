/*
 * manager.c - an example connection manager
 *
 * Copyright (C) 2007 Collabora Ltd.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "connection-manager.h"

#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>

#include "conn.h"

G_DEFINE_TYPE (ExampleEchoConnectionManager,
    example_echo_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

/* type definition stuff */

static void
example_echo_connection_manager_init (ExampleEchoConnectionManager *self)
{
}

/* private data */

typedef struct {
    gchar *account;
} ExampleParams;

static const TpCMParamSpec example_params[] = {
  { "account", DBUS_TYPE_STRING_AS_STRING, G_TYPE_STRING,
    TP_CONN_MGR_PARAM_FLAG_REQUIRED | TP_CONN_MGR_PARAM_FLAG_REGISTER, NULL,
    G_STRUCT_OFFSET (ExampleParams, account),
    tp_cm_param_filter_string_nonempty, NULL },

  { NULL }
};

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
  { "example", example_params, alloc_params, free_params },
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
  ExampleEchoConnection *conn = EXAMPLE_ECHO_CONNECTION
      (g_object_new (EXAMPLE_TYPE_ECHO_CONNECTION,
          "account", params->account,
          "protocol", proto,
          NULL));

  return (TpBaseConnection *) conn;
}

static void
example_echo_connection_manager_class_init (
    ExampleEchoConnectionManagerClass *klass)
{
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  base_class->new_connection = new_connection;
  base_class->cm_dbus_name = "example_echo";
  base_class->protocol_params = example_protocols;
}
