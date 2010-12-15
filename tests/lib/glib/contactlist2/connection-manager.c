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
#include "protocol.h"

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

static void
example_contact_list_connection_manager_constructed (GObject *object)
{
  ExampleContactListConnectionManager *self =
    EXAMPLE_CONTACT_LIST_CONNECTION_MANAGER (object);
  TpBaseConnectionManager *base = (TpBaseConnectionManager *) self;
  void (*constructed) (GObject *) =
    ((GObjectClass *) example_contact_list_connection_manager_parent_class)->constructed;
  TpBaseProtocol *protocol;

  if (constructed != NULL)
    constructed (object);

  protocol = g_object_new (EXAMPLE_TYPE_CONTACT_LIST_PROTOCOL,
      "name", "example",
      NULL);
  tp_base_connection_manager_add_protocol (base, protocol);
  g_object_unref (protocol);
}

static void
example_contact_list_connection_manager_class_init (
    ExampleContactListConnectionManagerClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  g_type_class_add_private (klass,
      sizeof (ExampleContactListConnectionManagerPrivate));

  object_class->constructed = example_contact_list_connection_manager_constructed;
  base_class->cm_dbus_name = "example_contact_list";
}
