/*
 * manager.c - an example connection manager
 *
 * Copyright © 2007-2010 Collabora Ltd.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "connection-manager.h"

#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>

#include <telepathy-glib/telepathy-glib.h>

#include "protocol.h"

G_DEFINE_TYPE (ExampleEcho2ConnectionManager,
    example_echo_2_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

/* type definition stuff */

static void
example_echo_2_connection_manager_init (
    ExampleEcho2ConnectionManager *self)
{
}

static void
example_echo_2_connection_manager_constructed (GObject *object)
{
  ExampleEcho2ConnectionManager *self =
    EXAMPLE_ECHO_2_CONNECTION_MANAGER (object);
  TpBaseConnectionManager *base = (TpBaseConnectionManager *) self;
  void (*constructed) (GObject *) =
    ((GObjectClass *) example_echo_2_connection_manager_parent_class)->constructed;
  TpBaseProtocol *protocol;

  if (constructed != NULL)
    constructed (object);

  protocol = g_object_new (EXAMPLE_TYPE_ECHO_2_PROTOCOL,
      "name", "example",
      NULL);
  tp_base_connection_manager_add_protocol (base, protocol);
  g_object_unref (protocol);
}

static const TpCMProtocolSpec no_protocols[] = {
  { NULL, NULL }
};

static TpBaseConnection *
new_connection (TpBaseConnectionManager *self,
                const gchar *proto,
                TpIntSet *params_present,
                gpointer parsed_params,
                GError **error)
{
  g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
      "Protocol's new_connection() should be called instead");
  return NULL;
}

static void
example_echo_2_connection_manager_class_init (
    ExampleEcho2ConnectionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  object_class->constructed = example_echo_2_connection_manager_constructed;

  base_class->new_connection = new_connection;
  base_class->cm_dbus_name = "example_echo_2";
  base_class->protocol_params = no_protocols;
}
