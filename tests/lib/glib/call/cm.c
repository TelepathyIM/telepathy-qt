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

#include "cm.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>

#include "conn.h"
#include "protocol.h"

G_DEFINE_TYPE (ExampleCallConnectionManager,
    example_call_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

struct _ExampleCallConnectionManagerPrivate
{
  int dummy;
};

static void
example_call_connection_manager_init (ExampleCallConnectionManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALL_CONNECTION_MANAGER,
      ExampleCallConnectionManagerPrivate);
}

static void
example_call_connection_manager_constructed (GObject *object)
{
  ExampleCallConnectionManager *self =
    EXAMPLE_CALL_CONNECTION_MANAGER (object);
  TpBaseConnectionManager *base = (TpBaseConnectionManager *) self;
  void (*constructed) (GObject *) =
    ((GObjectClass *) example_call_connection_manager_parent_class)->constructed;
  TpBaseProtocol *protocol;

  if (constructed != NULL)
    constructed (object);

  protocol = g_object_new (EXAMPLE_TYPE_CALL_PROTOCOL,
      "name", "example",
      NULL);
  tp_base_connection_manager_add_protocol (base, protocol);
  g_object_unref (protocol);
}

static void
example_call_connection_manager_class_init (
    ExampleCallConnectionManagerClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  TpBaseConnectionManagerClass *base_class =
      (TpBaseConnectionManagerClass *) klass;

  g_type_class_add_private (klass,
      sizeof (ExampleCallConnectionManagerPrivate));

  object_class->constructed = example_call_connection_manager_constructed;
  base_class->cm_dbus_name = "example_call";
}
