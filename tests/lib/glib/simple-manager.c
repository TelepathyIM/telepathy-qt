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
#include "util.h"

G_DEFINE_TYPE (TpTestsSimpleConnectionManager,
    tp_tests_simple_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

/* type definition stuff */

static void
tp_tests_simple_connection_manager_init (TpTestsSimpleConnectionManager *self)
{
}

/* private data */

typedef struct {
    gchar *account;
} TpTestsSimpleParams;

static const TpCMParamSpec simple_params[] = {
  { "account", DBUS_TYPE_STRING_AS_STRING, G_TYPE_STRING,
    TP_CONN_MGR_PARAM_FLAG_REQUIRED | TP_CONN_MGR_PARAM_FLAG_REGISTER, NULL,
    G_STRUCT_OFFSET (TpTestsSimpleParams, account),
    tp_cm_param_filter_string_nonempty, NULL },

  { NULL }
};

static gpointer
alloc_params (void)
{
  return g_slice_new0 (TpTestsSimpleParams);
}

static void
free_params (gpointer p)
{
  TpTestsSimpleParams *params = p;

  g_free (params->account);

  g_slice_free (TpTestsSimpleParams, params);
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
  TpTestsSimpleParams *params = parsed_params;
  TpTestsSimpleConnection *conn = TP_TESTS_SIMPLE_CONNECTION (
      tp_tests_object_new_static_class (TP_TESTS_TYPE_SIMPLE_CONNECTION,
          "account", params->account,
          "protocol", proto,
          NULL));

  return (TpBaseConnection *) conn;
}

static void
tp_tests_simple_connection_manager_class_init (
    TpTestsSimpleConnectionManagerClass *klass)
{
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  base_class->new_connection = new_connection;
  base_class->cm_dbus_name = "simple";
  base_class->protocol_params = simple_protocols;
}
