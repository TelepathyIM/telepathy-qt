/*
 * bug16307-conn.c - connection that reproduces the #15307 bug
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */
#include "bug16307-conn.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/util.h>

static void service_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (TpTestsBug16307Connection,
    tp_tests_bug16307_connection,
    TP_TESTS_TYPE_SIMPLE_CONNECTION,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION,
      service_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_ALIASING,
      NULL);
    );

/* type definition stuff */

enum
{
  SIGNAL_GET_STATUS_RECEIVED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = {0};

struct _TpTestsBug16307ConnectionPrivate
{
  /* In a real connection manager, the underlying implementation start
   * connecting, then go to state CONNECTED when finished. Here there isn't
   * actually a connection, so the connection process is fake and the time
   * when it connects is, for this test purpose, when the D-Bus method GetStatus
   * is called.
   *
   * Also, the GetStatus D-Bus reply is delayed until
   * tp_tests_bug16307_connection_inject_get_status_return() is called
   */
  DBusGMethodInvocation *get_status_invocation;
};

static void
tp_tests_bug16307_connection_init (TpTestsBug16307Connection *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TP_TESTS_TYPE_BUG16307_CONNECTION, TpTestsBug16307ConnectionPrivate);
}

static void
finalize (GObject *object)
{
  G_OBJECT_CLASS (tp_tests_bug16307_connection_parent_class)->finalize (object);
}

static gboolean
pretend_connected (gpointer data)
{
  TpTestsBug16307Connection *self = TP_TESTS_BUG16307_CONNECTION (data);
  TpBaseConnection *conn = (TpBaseConnection *) self;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (conn,
      TP_HANDLE_TYPE_CONTACT);
  gchar *account;

  g_object_get (self, "account", &account, NULL);

  conn->self_handle = tp_handle_ensure (contact_repo, account,
      NULL, NULL);

  g_free (account);

  tp_base_connection_change_status (conn, TP_CONNECTION_STATUS_CONNECTED,
      TP_CONNECTION_STATUS_REASON_REQUESTED);

  return FALSE;
}

void
tp_tests_bug16307_connection_inject_get_status_return (TpTestsBug16307Connection *self)
{
  TpBaseConnection *self_base = TP_BASE_CONNECTION (self);
  DBusGMethodInvocation *context;
  gulong get_signal_id;

  /* if we don't have a pending get_status yet, wait for it */
  if (self->priv->get_status_invocation == NULL)
    {
      GMainLoop *loop = g_main_loop_new (NULL, FALSE);

      get_signal_id = g_signal_connect_swapped (self, "get-status-received",
          G_CALLBACK (g_main_loop_quit), loop);

      g_main_loop_run (loop);

      g_signal_handler_disconnect (self, get_signal_id);

      g_main_loop_unref (loop);
    }

  context = self->priv->get_status_invocation;
  g_assert (context != NULL);

  if (self_base->status == TP_INTERNAL_CONNECTION_STATUS_NEW)
    {
      tp_svc_connection_return_from_get_status (
          context, TP_CONNECTION_STATUS_DISCONNECTED);
    }
  else
    {
      tp_svc_connection_return_from_get_status (
          context, self_base->status);
    }

  self->priv->get_status_invocation = NULL;
}

static gboolean
start_connecting (TpBaseConnection *conn,
                  GError **error)
{
  tp_base_connection_change_status (conn, TP_CONNECTION_STATUS_CONNECTING,
      TP_CONNECTION_STATUS_REASON_REQUESTED);

  return TRUE;
}

static void
tp_tests_bug16307_connection_class_init (TpTestsBug16307ConnectionClass *klass)
{
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;
  static const gchar *interfaces_always_present[] = {
      TP_IFACE_CONNECTION_INTERFACE_ALIASING,
      TP_IFACE_CONNECTION_INTERFACE_CAPABILITIES,
      TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_AVATARS,
      NULL };
  static TpDBusPropertiesMixinPropImpl connection_properties[] = {
      { "Status", "dbus-status-except-i-broke-it", NULL },
      { NULL }
  };

  object_class->finalize = finalize;
  g_type_class_add_private (klass, sizeof (TpTestsBug16307ConnectionPrivate));

  base_class->start_connecting = start_connecting;

  base_class->interfaces_always_present = interfaces_always_present;

  signals[SIGNAL_GET_STATUS_RECEIVED] = g_signal_new ("get-status-received",
      G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
      0,
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  /* break the Connection D-Bus properties implementation, so that we always
   * cause the slower introspection codepath (the one that actually calls
   * GetStatus) in TpConnection to be invoked */
  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_IFACE_QUARK_CONNECTION,
      NULL, NULL, connection_properties);
}

/**
 * tp_tests_bug16307_connection_get_status
 *
 * Implements D-Bus method GetStatus
 * on interface org.freedesktop.Telepathy.Connection
 */
static void
tp_tests_bug16307_connection_get_status (TpSvcConnection *iface,
                              DBusGMethodInvocation *context)
{
  TpBaseConnection *self_base = TP_BASE_CONNECTION (iface);
  TpTestsBug16307Connection *self = TP_TESTS_BUG16307_CONNECTION (iface);

  /* auto-connect on get_status */
  if ((self_base->status == TP_INTERNAL_CONNECTION_STATUS_NEW ||
       self_base->status == TP_CONNECTION_STATUS_DISCONNECTED))
    {
      pretend_connected (self);
    }

  /* D-Bus return call later */
  g_assert (self->priv->get_status_invocation == NULL);
  g_assert (context != NULL);
  self->priv->get_status_invocation = context;

  g_signal_emit (self, signals[SIGNAL_GET_STATUS_RECEIVED], 0);
}


static void
service_iface_init (gpointer g_iface, gpointer iface_data)
{
  TpSvcConnectionClass *klass = g_iface;

#define IMPLEMENT(prefix,x) tp_svc_connection_implement_##x (klass, \
    tp_tests_bug16307_connection_##prefix##x)
  IMPLEMENT(,get_status);
#undef IMPLEMENT
}

