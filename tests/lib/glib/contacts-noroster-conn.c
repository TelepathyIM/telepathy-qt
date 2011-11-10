/*
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2011 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "contacts-noroster-conn.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>
#include <telepathy-glib/gtypes.h>

G_DEFINE_TYPE_WITH_CODE (TpTestsContactsNorosterConnection,
    tp_tests_contacts_noroster_connection,
    TP_TESTS_TYPE_SIMPLE_CONNECTION,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACTS,
      NULL);
    );

static void
tp_tests_contacts_noroster_connection_init (TpTestsContactsNorosterConnection *self)
{
}

static void
finalize (GObject *object)
{
  G_OBJECT_CLASS (tp_tests_contacts_noroster_connection_parent_class)->finalize (object);
}

static void
tp_tests_contacts_noroster_connection_class_init (TpTestsContactsNorosterConnectionClass *klass)
{
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;
  static const gchar *interfaces_always_present[] = {
      TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
      NULL };

  object_class->finalize = finalize;

  base_class->interfaces_always_present = interfaces_always_present;
}
