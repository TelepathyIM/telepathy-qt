/*
 * simple-manager.c - an simple connection manager
 *
 * Copyright (C) 2007-2008 Collabora Ltd.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "simple-manager.h"

#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>

#include "simple-conn.h"

G_DEFINE_TYPE (SimpleConnectionManager,
    simple_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

/* type definition stuff */

static void
simple_connection_manager_init (SimpleConnectionManager *self)
{
}

/* private data */

typedef struct {
    gchar *account;
} SimpleParams;

static const TpCMParamSpec simple_params[] = {
  { "account", DBUS_TYPE_STRING_AS_STRING, G_TYPE_STRING,
    TP_CONN_MGR_PARAM_FLAG_REQUIRED | TP_CONN_MGR_PARAM_FLAG_REGISTER, NULL,
    G_STRUCT_OFFSET (SimpleParams, account),
    tp_cm_param_filter_string_nonempty, NULL },

  { NULL }
};

static gpointer
alloc_params (void)
{
  return g_slice_new0 (SimpleParams);
}

static void
free_params (gpointer p)
{
  SimpleParams *params = p;

  g_free (params->account);

  g_slice_free (SimpleParams, params);
}

static const TpCMProtocolSpec simple_protocols[] = {
  { "simple", simple_params, alloc_params, free_params },
  { NULL, NULL }
};

static TpBaseConnection *
new_connection (TpBaseConnectionManager *self,
                const gchar *proto,
                TpIntSet *params_present,
                gpointer parsed_params,
                GError **error)
{
  SimpleParams *params = parsed_params;
  SimpleConnection *conn = SIMPLE_CONNECTION
      (g_object_new (SIMPLE_TYPE_CONNECTION,
          "account", params->account,
          "protocol", proto,
          NULL));

  return (TpBaseConnection *) conn;
}

static void
simple_connection_manager_class_init (SimpleConnectionManagerClass *klass)
{
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  base_class->new_connection = new_connection;
  base_class->cm_dbus_name = "simple";
  base_class->protocol_params = simple_protocols;
}
