/*
 * simple-conn.c - a simple connection
 *
 * Copyright (C) 2007-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "simple-conn.h"

#include <string.h>

#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/util.h>

#include "textchan-null.h"
#include "util.h"

static void conn_iface_init (TpSvcConnectionClass *);

G_DEFINE_TYPE_WITH_CODE (TpTestsSimpleConnection, tp_tests_simple_connection,
    TP_TYPE_BASE_CONNECTION,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION, conn_iface_init))

/* type definition stuff */

enum
{
  PROP_ACCOUNT = 1,
  PROP_BREAK_PROPS = 2,
  PROP_DBUS_STATUS = 3,
  N_PROPS
};

enum
{
  SIGNAL_GOT_SELF_HANDLE,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = {0};

struct _TpTestsSimpleConnectionPrivate
{
  gchar *account;
  guint connect_source;
  guint disconnect_source;
  gboolean break_fastpath_props;

  /* TpHandle => reffed TpTestsTextChannelNull */
  GHashTable *channels;

  GError *get_self_handle_error /* initially NULL */ ;
};

static void
tp_tests_simple_connection_init (TpTestsSimpleConnection *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TP_TESTS_TYPE_SIMPLE_CONNECTION, TpTestsSimpleConnectionPrivate);

  self->priv->channels = g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) g_object_unref);
}

static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *spec)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (object);

  switch (property_id) {
    case PROP_ACCOUNT:
      g_value_set_string (value, self->priv->account);
      break;
    case PROP_BREAK_PROPS:
      g_value_set_boolean (value, self->priv->break_fastpath_props);
      break;
    case PROP_DBUS_STATUS:
      if (self->priv->break_fastpath_props)
        {
          g_debug ("returning broken value for Connection.Status");
          g_value_set_uint (value, 0xdeadbeefU);
        }
      else
        {
          guint32 status = TP_BASE_CONNECTION (self)->status;

          if (status == TP_INTERNAL_CONNECTION_STATUS_NEW)
            g_value_set_uint (value, TP_CONNECTION_STATUS_DISCONNECTED);
          else
            g_value_set_uint (value, status);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
  }
}

static void
set_property (GObject *object,
              guint property_id,
              const GValue *value,
              GParamSpec *spec)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (object);

  switch (property_id) {
    case PROP_ACCOUNT:
      g_free (self->priv->account);
      self->priv->account = g_utf8_strdown (g_value_get_string (value), -1);
      break;
    case PROP_BREAK_PROPS:
      self->priv->break_fastpath_props = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
  }
}

static void
dispose (GObject *object)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (object);

  g_hash_table_unref (self->priv->channels);

  G_OBJECT_CLASS (tp_tests_simple_connection_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (object);

  if (self->priv->connect_source != 0)
    {
      g_source_remove (self->priv->connect_source);
    }

  if (self->priv->disconnect_source != 0)
    {
      g_source_remove (self->priv->disconnect_source);
    }

  g_clear_error (&self->priv->get_self_handle_error);
  g_free (self->priv->account);

  G_OBJECT_CLASS (tp_tests_simple_connection_parent_class)->finalize (object);
}

static gchar *
get_unique_connection_name (TpBaseConnection *conn)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (conn);

  return g_strdup (self->priv->account);
}

static gchar *
tp_tests_simple_normalize_contact (TpHandleRepoIface *repo,
                           const gchar *id,
                           gpointer context,
                           GError **error)
{
  if (id[0] == '\0')
    {
      g_set_error (error, TP_ERROR, TP_ERROR_INVALID_HANDLE,
          "ID must not be empty");
      return NULL;
    }

  if (strchr (id, ' ') != NULL)
    {
      g_set_error (error, TP_ERROR, TP_ERROR_INVALID_HANDLE,
          "ID must not contain spaces");
      return NULL;
    }

  return g_utf8_strdown (id, -1);
}

static void
create_handle_repos (TpBaseConnection *conn,
                     TpHandleRepoIface *repos[NUM_TP_HANDLE_TYPES])
{
  repos[TP_HANDLE_TYPE_CONTACT] = tp_dynamic_handle_repo_new
      (TP_HANDLE_TYPE_CONTACT, tp_tests_simple_normalize_contact, NULL);
  repos[TP_HANDLE_TYPE_ROOM] = tp_dynamic_handle_repo_new
      (TP_HANDLE_TYPE_ROOM, NULL, NULL);
}

static GPtrArray *
create_channel_factories (TpBaseConnection *conn)
{
  return g_ptr_array_sized_new (0);
}

void
tp_tests_simple_connection_inject_disconnect (TpTestsSimpleConnection *self)
{
  tp_base_connection_change_status ((TpBaseConnection *) self,
      TP_CONNECTION_STATUS_DISCONNECTED,
      TP_CONNECTION_STATUS_REASON_REQUESTED);
}

static gboolean
pretend_connected (gpointer data)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (data);
  TpBaseConnection *conn = (TpBaseConnection *) self;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (conn,
      TP_HANDLE_TYPE_CONTACT);

  conn->self_handle = tp_handle_ensure (contact_repo, self->priv->account,
      NULL, NULL);

  if (conn->status == TP_CONNECTION_STATUS_CONNECTING)
    {
      tp_base_connection_change_status (conn, TP_CONNECTION_STATUS_CONNECTED,
          TP_CONNECTION_STATUS_REASON_REQUESTED);
    }

  self->priv->connect_source = 0;
  return FALSE;
}

static gboolean
start_connecting (TpBaseConnection *conn,
                  GError **error)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (conn);

  tp_base_connection_change_status (conn, TP_CONNECTION_STATUS_CONNECTING,
      TP_CONNECTION_STATUS_REASON_REQUESTED);

  /* In a real connection manager we'd ask the underlying implementation to
   * start connecting, then go to state CONNECTED when finished. Here there
   * isn't actually a connection, so we'll fake a connection process that
   * takes time. */
  self->priv->connect_source = g_timeout_add (0, pretend_connected, self);

  return TRUE;
}

static gboolean
pretend_disconnected (gpointer data)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (data);

  /* We are disconnected, all our channels are invalidated */
  g_hash_table_remove_all (self->priv->channels);

  tp_base_connection_finish_shutdown (TP_BASE_CONNECTION (data));
  self->priv->disconnect_source = 0;
  return FALSE;
}

static void
shut_down (TpBaseConnection *conn)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (conn);

  /* In a real connection manager we'd ask the underlying implementation to
   * start shutting down, then call this function when finished. Here there
   * isn't actually a connection, so we'll fake a disconnection process that
   * takes time. */
  self->priv->disconnect_source = g_timeout_add (0, pretend_disconnected,
      conn);
}

static void
tp_tests_simple_connection_class_init (TpTestsSimpleConnectionClass *klass)
{
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;
  static const gchar *interfaces_always_present[] = {
      TP_IFACE_CONNECTION_INTERFACE_REQUESTS, NULL };

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->dispose = dispose;
  object_class->finalize = finalize;
  g_type_class_add_private (klass, sizeof (TpTestsSimpleConnectionPrivate));

  base_class->create_handle_repos = create_handle_repos;
  base_class->get_unique_connection_name = get_unique_connection_name;
  base_class->create_channel_factories = create_channel_factories;
  base_class->start_connecting = start_connecting;
  base_class->shut_down = shut_down;

  base_class->interfaces_always_present = interfaces_always_present;

  param_spec = g_param_spec_string ("account", "Account name",
      "The username of this user", NULL,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_ACCOUNT, param_spec);

  param_spec = g_param_spec_boolean ("break-0192-properties",
      "Break 0.19.2 properties",
      "Break Connection D-Bus properties introduced in spec 0.19.2", FALSE,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_BREAK_PROPS, param_spec);

  param_spec = g_param_spec_uint ("dbus-status",
      "Connection.Status",
      "The connection status as visible on D-Bus (overridden so can break it)",
      TP_CONNECTION_STATUS_CONNECTED, G_MAXUINT,
      TP_CONNECTION_STATUS_DISCONNECTED,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DBUS_STATUS, param_spec);

  signals[SIGNAL_GOT_SELF_HANDLE] = g_signal_new ("got-self-handle",
      G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
      0,
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
}

void
tp_tests_simple_connection_set_identifier (TpTestsSimpleConnection *self,
                                  const gchar *identifier)
{
  TpBaseConnection *conn = (TpBaseConnection *) self;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (conn,
      TP_HANDLE_TYPE_CONTACT);
  TpHandle handle = tp_handle_ensure (contact_repo, identifier, NULL, NULL);

  /* if this fails then the identifier was bad - caller error */
  g_return_if_fail (handle != 0);

  tp_base_connection_set_self_handle (conn, handle);
}

TpTestsSimpleConnection *
tp_tests_simple_connection_new (const gchar *account,
    const gchar *protocol)
{
  return TP_TESTS_SIMPLE_CONNECTION (g_object_new (
      TP_TESTS_TYPE_SIMPLE_CONNECTION,
      "account", account,
      "protocol", protocol,
      NULL));
}

gchar *
tp_tests_simple_connection_ensure_text_chan (TpTestsSimpleConnection *self,
    const gchar *target_id,
    GHashTable **props)
{
  TpTestsTextChannelNull *chan;
  gchar *chan_path;
  TpHandleRepoIface *contact_repo;
  TpHandle handle;
  static guint count = 0;
  TpBaseConnection *base_conn = (TpBaseConnection *) self;

  /* Get contact handle */
  contact_repo = tp_base_connection_get_handles (base_conn,
      TP_HANDLE_TYPE_CONTACT);
  g_assert (contact_repo != NULL);

  handle = tp_handle_ensure (contact_repo, target_id, NULL, NULL);

  chan = g_hash_table_lookup (self->priv->channels, GUINT_TO_POINTER (handle));
  if (chan != NULL)
    {
      /* Channel already exist, reuse it */
      g_object_get (chan, "object-path", &chan_path, NULL);
    }
  else
    {
      chan_path = g_strdup_printf ("%s/Channel%u", base_conn->object_path,
          count++);

       chan = TP_TESTS_TEXT_CHANNEL_NULL (
          tp_tests_object_new_static_class (
            TP_TESTS_TYPE_TEXT_CHANNEL_NULL,
            "connection", self,
            "object-path", chan_path,
            "handle", handle,
            NULL));

      g_hash_table_insert (self->priv->channels, GUINT_TO_POINTER (handle),
          chan);
    }

  if (props != NULL)
    *props = tp_tests_text_channel_get_props (chan);

  return chan_path;
}

void
tp_tests_simple_connection_set_get_self_handle_error (
    TpTestsSimpleConnection *self,
    GQuark domain,
    gint code,
    const gchar *message)
{
  self->priv->get_self_handle_error = g_error_new_literal (domain, code,
      message);
}

static void
get_self_handle (TpSvcConnection *iface,
    DBusGMethodInvocation *context)
{
  TpTestsSimpleConnection *self = TP_TESTS_SIMPLE_CONNECTION (iface);
  TpBaseConnection *base = TP_BASE_CONNECTION (iface);

  g_assert (TP_IS_BASE_CONNECTION (base));

  TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

  if (self->priv->get_self_handle_error != NULL)
    {
      dbus_g_method_return_error (context, self->priv->get_self_handle_error);
      return;
    }

  tp_svc_connection_return_from_get_self_handle (context, base->self_handle);
  g_signal_emit (self, signals[SIGNAL_GOT_SELF_HANDLE], 0);
}

static void
conn_iface_init (TpSvcConnectionClass *iface)
{
#define IMPLEMENT(prefix,x) \
  tp_svc_connection_implement_##x (iface, prefix##x)
  IMPLEMENT(,get_self_handle);
#undef IMPLEMENT
}
