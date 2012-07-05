/*
 * dbus-tube-chan.c - Simple dbus tube channel
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "dbus-tube-chan.h"

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/svc-channel.h>
#include <telepathy-glib/gnio-util.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <glib/gstdio.h>

enum
{
  PROP_SERVICE_NAME = 1,
  PROP_DBUS_NAMES,
  PROP_SUPPORTED_ACCESS_CONTROLS,
  PROP_PARAMETERS,
  PROP_STATE,
  PROP_DBUS_ADDRESS
};

struct _TpTestsDBusTubeChannelPrivate {
    TpTubeChannelState state;

    TpSocketAccessControl access_control;

    /* our unique D-Bus name on the virtual tube bus (NULL for 1-1 D-Bus tubes)*/
    gchar *dbus_local_name;
    /* the address that we are listening for D-Bus connections on */
    gchar *dbus_srv_addr;
    /* the path of the UNIX socket used by the D-Bus server */
    gchar *socket_path;
    /* the server that's listening on dbus_srv_addr */
    DBusServer *dbus_srv;
    /* the connection to dbus_srv from a local client, or NULL */
    DBusConnection *dbus_conn;
    /* the queue of D-Bus messages to be delivered to a local client when it
    * will connect */
    GSList *dbus_msg_queue;
    /* current size of the queue in bytes. The maximum is MAX_QUEUE_SIZE */
    unsigned long dbus_msg_queue_size;
    /* mapping of contact handle -> D-Bus name (empty for 1-1 D-Bus tubes) */
    GHashTable *dbus_names;

    GArray *supported_access_controls;

    GHashTable *parameters;

    gboolean close_on_accept;
};

static void
tp_tests_dbus_tube_channel_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpTestsDBusTubeChannel *self = (TpTestsDBusTubeChannel *) object;

  switch (property_id)
    {
      case PROP_SERVICE_NAME:
        g_value_set_string (value, "com.test.Test");
        break;

      case PROP_DBUS_NAMES:
        g_value_set_boxed (value, self->priv->dbus_names);
        break;

      case PROP_SUPPORTED_ACCESS_CONTROLS:
        g_value_set_boxed (value, self->priv->supported_access_controls);
        break;

      case PROP_PARAMETERS:
        g_value_set_boxed (value, self->priv->parameters);
        break;

      case PROP_DBUS_ADDRESS:
        g_value_set_string (value, self->priv->dbus_srv_addr);
        break;

      case PROP_STATE:
        g_value_set_uint (value, self->priv->state);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
tp_tests_dbus_tube_channel_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  TpTestsDBusTubeChannel *self = (TpTestsDBusTubeChannel *) object;

  switch (property_id)
    {
      case PROP_SUPPORTED_ACCESS_CONTROLS:
        if (self->priv->supported_access_controls != NULL)
          g_array_free (self->priv->supported_access_controls, FALSE);
        self->priv->supported_access_controls = g_value_dup_boxed (value);
        break;

      case PROP_PARAMETERS:
        if (self->priv->parameters != NULL)
          g_hash_table_destroy (self->priv->parameters);
        self->priv->parameters = g_value_dup_boxed (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void dbus_tube_iface_init (gpointer iface, gpointer data);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (TpTestsDBusTubeChannel,
    tp_tests_dbus_tube_channel,
    TP_TYPE_BASE_CHANNEL,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_DBUS_TUBE,
      dbus_tube_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_TUBE,
      NULL);
    )

/* type definition stuff */

static const char * tp_tests_dbus_tube_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_TUBE,
    NULL
};

static void
tp_tests_dbus_tube_channel_init (TpTestsDBusTubeChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
      TP_TESTS_TYPE_DBUS_TUBE_CHANNEL, TpTestsDBusTubeChannelPrivate);

  self->priv->dbus_names = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static GObject *
constructor (GType type,
             guint n_props,
             GObjectConstructParam *props)
{
  GObject *object =
      G_OBJECT_CLASS (tp_tests_dbus_tube_channel_parent_class)->constructor (
          type, n_props, props);
  TpTestsDBusTubeChannel *self = TP_TESTS_DBUS_TUBE_CHANNEL (object);

  if (tp_base_channel_is_requested (TP_BASE_CHANNEL (self)))
    {
      self->priv->state = TP_TUBE_CHANNEL_STATE_NOT_OFFERED;
      self->priv->parameters = tp_asv_new (NULL, NULL);
    }
  else
    {
      self->priv->state = TP_TUBE_CHANNEL_STATE_LOCAL_PENDING;
      self->priv->parameters = tp_asv_new ("badger", G_TYPE_UINT, 42,
            NULL);
    }

  if (self->priv->supported_access_controls == NULL)
    {
      GArray *acontrols;
      TpSocketAccessControl a;

      acontrols = g_array_sized_new (FALSE, FALSE, sizeof (guint), 1);

      a = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
      acontrols = g_array_append_val (acontrols, a);

      self->priv->supported_access_controls = acontrols;
    }

  tp_base_channel_register (TP_BASE_CHANNEL (self));

  return object;
}

static void
dispose (GObject *object)
{
  TpTestsDBusTubeChannel *self = (TpTestsDBusTubeChannel *) object;

  tp_clear_pointer (&self->priv->dbus_names, g_hash_table_unref);

  if (self->priv->supported_access_controls != NULL)
    g_array_free (self->priv->supported_access_controls, TRUE);

  if (self->priv->parameters != NULL)
    g_hash_table_destroy (self->priv->parameters);

  if (self->priv->dbus_srv != NULL)
    dbus_server_unref (self->priv->dbus_srv);

  if (self->priv->dbus_conn != NULL)
    dbus_connection_unref (self->priv->dbus_conn);

  if (self->priv->dbus_srv_addr != NULL)
    g_free (self->priv->dbus_srv_addr);

  if (self->priv->socket_path != NULL)
    g_free (self->priv->socket_path);

  ((GObjectClass *) tp_tests_dbus_tube_channel_parent_class)->dispose (
    object);
}

static void
channel_close (TpBaseChannel *channel)
{
  tp_base_channel_destroyed (channel);
}

static void
fill_immutable_properties (TpBaseChannel *chan,
    GHashTable *properties)
{
  TpBaseChannelClass *klass = TP_BASE_CHANNEL_CLASS (
      tp_tests_dbus_tube_channel_parent_class);

  klass->fill_immutable_properties (chan, properties);

  tp_dbus_properties_mixin_fill_properties_hash (
      G_OBJECT (chan), properties,
      TP_IFACE_CHANNEL_TYPE_DBUS_TUBE, "ServiceName",
      TP_IFACE_CHANNEL_TYPE_DBUS_TUBE, "SupportedAccessControls",
      NULL);

  if (!tp_base_channel_is_requested (chan))
    {
      /* Parameters is immutable only for incoming tubes */
      tp_dbus_properties_mixin_fill_properties_hash (
          G_OBJECT (chan), properties,
          TP_IFACE_CHANNEL_INTERFACE_TUBE, "Parameters",
          NULL);
    }
}

static void
tp_tests_dbus_tube_channel_class_init (TpTestsDBusTubeChannelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);
  GParamSpec *param_spec;
  static TpDBusPropertiesMixinPropImpl dbus_tube_props[] = {
      { "ServiceName", "service-name", NULL, },
      { "DBusNames", "dbus-names", NULL, },
      { "SupportedAccessControls", "supported-access-controls", NULL, },
      { NULL }
  };
  static TpDBusPropertiesMixinPropImpl tube_props[] = {
      { "Parameters", "parameters", NULL, },
      { "State", "state", NULL, },
      { NULL }
  };

  object_class->constructor = constructor;
  object_class->get_property = tp_tests_dbus_tube_channel_get_property;
  object_class->set_property = tp_tests_dbus_tube_channel_set_property;
  object_class->dispose = dispose;

  base_class->channel_type = TP_IFACE_CHANNEL_TYPE_DBUS_TUBE;
  base_class->interfaces = tp_tests_dbus_tube_channel_interfaces;
  base_class->close = channel_close;
  base_class->fill_immutable_properties = fill_immutable_properties;

  /* base_class->target_handle_type is defined in subclasses */

  param_spec = g_param_spec_string ("service-name", "Service Name",
      "the service name associated with this tube object.",
       "",
       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SERVICE_NAME, param_spec);

  param_spec = g_param_spec_boxed ("dbus-names", "DBus Names",
      "DBusTube.DBusNames",
      TP_HASH_TYPE_DBUS_TUBE_PARTICIPANTS,
       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DBUS_NAMES, param_spec);

  param_spec = g_param_spec_boxed ("supported-access-controls",
      "Supported access-controls",
      "GArray containing supported access controls.",
      DBUS_TYPE_G_UINT_ARRAY,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
      PROP_SUPPORTED_ACCESS_CONTROLS, param_spec);

  param_spec = g_param_spec_string (
      "dbus-address",
      "D-Bus address",
      "The D-Bus address on which this tube will listen for connections",
      "",
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DBUS_ADDRESS,
      param_spec);

  param_spec = g_param_spec_boxed (
      "parameters", "Parameters",
      "parameters of the tube",
      TP_HASH_TYPE_STRING_VARIANT_MAP,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PARAMETERS,
      param_spec);

  param_spec = g_param_spec_uint (
      "state", "TpTubeState",
      "state of the tube",
      0, NUM_TP_TUBE_CHANNEL_STATES - 1, 0,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_STATE,
      param_spec);

  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_IFACE_QUARK_CHANNEL_TYPE_DBUS_TUBE,
      tp_dbus_properties_mixin_getter_gobject_properties, NULL,
      dbus_tube_props);

  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_IFACE_QUARK_CHANNEL_INTERFACE_TUBE,
      tp_dbus_properties_mixin_getter_gobject_properties, NULL,
      tube_props);

  g_type_class_add_private (object_class,
      sizeof (TpTestsDBusTubeChannelPrivate));
}

static gboolean
check_access_control (TpTestsDBusTubeChannel *self,
    TpSocketAccessControl access_control)
{
  guint i;

  for (i = 0; i < self->priv->supported_access_controls->len; i++)
    {
      if (g_array_index (self->priv->supported_access_controls, TpSocketAccessControl, i) == access_control)
        return TRUE;
    }

  return FALSE;
}

static void
change_state (TpTestsDBusTubeChannel *self,
  TpTubeChannelState state)
{
  self->priv->state = state;

  tp_svc_channel_interface_tube_emit_tube_channel_state_changed (self, state);
}

/*
 * Characters used are permissible both in filenames and in D-Bus names. (See
 * D-Bus specification for restrictions.)
 */
static void
generate_ascii_string (guint len,
                       gchar *buf)
{
  const gchar *chars =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "_-";
  guint i;

  for (i = 0; i < len; i++)
    buf[i] = chars[g_random_int_range (0, 64)];
}

static DBusHandlerResult
filter_cb (DBusConnection *conn,
           DBusMessage *msg,
           void *user_data)
{
  TpTestsDBusTubeChannel *self = TP_TESTS_DBUS_TUBE_CHANNEL (user_data);
  TpTestsDBusTubeChannelPrivate *priv = self->priv;
  gchar *marshalled = NULL;
  gint len;

  if (dbus_message_get_type (msg) == DBUS_MESSAGE_TYPE_SIGNAL &&
      !tp_strdiff (dbus_message_get_interface (msg),
        "org.freedesktop.DBus.Local") &&
      !tp_strdiff (dbus_message_get_member (msg), "Disconnected"))
    {
      /* connection was disconnected */
      g_debug ("connection was disconnected");
      dbus_connection_close (priv->dbus_conn);
      tp_clear_pointer (&priv->dbus_conn, dbus_connection_unref);
      goto out;
    }

  if (priv->dbus_local_name != NULL)
    {
      if (!dbus_message_set_sender (msg, priv->dbus_local_name))
        g_debug ("dbus_message_set_sender failed");
    }

  if (!dbus_message_marshal (msg, &marshalled, &len))
    goto out;

out:
  if (marshalled != NULL)
    g_free (marshalled);

  return DBUS_HANDLER_RESULT_HANDLED;
}

static dbus_bool_t
allow_all_connections (DBusConnection *conn,
                       unsigned long uid,
                       void *data)
{
  return TRUE;
}

static void
new_connection_cb (DBusServer *server,
                   DBusConnection *conn,
                   void *data)
{
  TpTestsDBusTubeChannel *self = TP_TESTS_DBUS_TUBE_CHANNEL (data);
  TpTestsDBusTubeChannelPrivate *priv = self->priv;
  guint32 serial;
  GSList *i;

  if (priv->dbus_conn != NULL)
    /* we already have a connection; drop this new one */
    /* return without reffing conn means it will be dropped */
    return;

  dbus_connection_ref (conn);
  dbus_connection_setup_with_g_main (conn, NULL);
  dbus_connection_add_filter (conn, filter_cb, self, NULL);
  priv->dbus_conn = conn;

  if (priv->access_control == TP_SOCKET_ACCESS_CONTROL_LOCALHOST)
    {
      /* By default libdbus use Credentials access control. If user wants
       * to use the Localhost access control, we need to bypass this check. */
      dbus_connection_set_unix_user_function (conn, allow_all_connections,
          NULL, NULL);
    }

  /* We may have received messages to deliver before the local connection is
   * established. Theses messages are kept in the dbus_msg_queue list and are
   * delivered as soon as we get the connection. */
  g_debug ("%u messages in the queue (%lu bytes)",
         g_slist_length (priv->dbus_msg_queue), priv->dbus_msg_queue_size);
  priv->dbus_msg_queue = g_slist_reverse (priv->dbus_msg_queue);
  for (i = priv->dbus_msg_queue; i != NULL; i = g_slist_delete_link (i, i))
    {
      DBusMessage *msg = i->data;
      g_debug ("delivering queued message from '%s' to '%s' on the "
             "new connection",
             dbus_message_get_sender (msg),
             dbus_message_get_destination (msg));
      dbus_connection_send (priv->dbus_conn, msg, &serial);
      dbus_message_unref (msg);
    }
  priv->dbus_msg_queue = NULL;
  priv->dbus_msg_queue_size = 0;
}

/* There is two step to enable receiving a D-Bus connection from the local
 * application:
 * - listen on the socket
 * - add the socket in the mainloop
 *
 * We need to know the socket path to return from the AcceptDBusTube D-Bus
 * call but the socket in the mainloop must be added only when we are ready
 * to receive connections, that is when the bytestream is fully open with the
 * remote contact.
 *
 * See also Bug 13891:
 * https://bugs.freedesktop.org/show_bug.cgi?id=13891
 * */
static gboolean
create_dbus_server (TpTestsDBusTubeChannel *self,
                    GError **err)
{
#define SERVER_LISTEN_MAX_TRIES 5
  TpTestsDBusTubeChannelPrivate *priv = self->priv;
  guint i;

  if (priv->dbus_srv != NULL)
    return TRUE;

  for (i = 0; i < SERVER_LISTEN_MAX_TRIES; i++)
    {
      gchar suffix[8];
      DBusError error;

      g_free (priv->dbus_srv_addr);
      g_free (priv->socket_path);

      generate_ascii_string (8, suffix);
      priv->socket_path = g_strdup_printf ("%s/dbus-tpqt4-test-%.8s",
          g_get_tmp_dir (), suffix);
      priv->dbus_srv_addr = g_strdup_printf ("unix:path=%s",
          priv->socket_path);

      dbus_error_init (&error);
      priv->dbus_srv = dbus_server_listen (priv->dbus_srv_addr, &error);

      if (priv->dbus_srv != NULL)
        break;

      g_debug ("dbus_server_listen failed (try %u): %s: %s", i, error.name,
          error.message);
      dbus_error_free (&error);
    }

  if (priv->dbus_srv == NULL)
    {
      g_debug ("all attempts failed. Close the tube");

      g_free (priv->dbus_srv_addr);
      priv->dbus_srv_addr = NULL;

      g_free (priv->socket_path);
      priv->socket_path = NULL;

      g_set_error (err, TP_ERROR, TP_ERROR_NOT_AVAILABLE,
          "Can't create D-Bus server");
      return FALSE;
    }

  g_debug ("listening on %s", priv->dbus_srv_addr);

  dbus_server_set_new_connection_function (priv->dbus_srv, new_connection_cb,
      self, NULL);

  return TRUE;
}

static void
dbus_tube_offer (TpSvcChannelTypeDBusTube *iface,
    GHashTable *parameters,
    guint access_control,
    DBusGMethodInvocation *context)
{
  TpTestsDBusTubeChannel *self = (TpTestsDBusTubeChannel *) iface;
  GError *error = NULL;

  if (self->priv->state != TP_TUBE_CHANNEL_STATE_NOT_OFFERED)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Tube is not in the not offered state");
      goto fail;
    }

  if (!check_access_control (self, access_control))
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Address type not supported with this access control");
      goto fail;
    }

  self->priv->access_control = access_control;

  if (!create_dbus_server (self, &error))
    goto fail;

  g_object_set (self, "parameters", parameters, NULL);

  change_state (self, TP_TUBE_CHANNEL_STATE_REMOTE_PENDING);

  tp_svc_channel_type_dbus_tube_return_from_offer (context, self->priv->dbus_srv_addr);
  return;

fail:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
dbus_tube_accept (TpSvcChannelTypeDBusTube *iface,
    guint access_control,
    DBusGMethodInvocation *context)
{
  TpTestsDBusTubeChannel *self = (TpTestsDBusTubeChannel *) iface;
  GError *error = NULL;

  if (self->priv->state != TP_TUBE_CHANNEL_STATE_LOCAL_PENDING)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Tube is not in the local pending state");
      goto fail;
    }

  if (!check_access_control (self, access_control))
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Address type not supported with this access control");
      goto fail;
    }

  if (self->priv->close_on_accept)
    {
      tp_base_channel_close (TP_BASE_CHANNEL (self));
      return;
    }

  if (!create_dbus_server (self, &error))
    goto fail;

  change_state (self, TP_TUBE_CHANNEL_STATE_OPEN);

  tp_svc_channel_type_dbus_tube_return_from_accept (context, self->priv->dbus_srv_addr);
  return;

fail:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
dbus_tube_iface_init (gpointer iface,
    gpointer data)
{
  TpSvcChannelTypeDBusTubeClass *klass = iface;

#define IMPLEMENT(x) tp_svc_channel_type_dbus_tube_implement_##x (klass, dbus_tube_##x)
  IMPLEMENT(offer);
  IMPLEMENT(accept);
#undef IMPLEMENT
}

/* Called to emulate a peer connecting to an offered tube */
void
tp_tests_dbus_tube_channel_peer_connected_no_stream (TpTestsDBusTubeChannel *self,
    gchar *bus_name,
    TpHandle handle)
{
  GHashTable *added;
  GArray *removed;

  if (self->priv->state == TP_TUBE_CHANNEL_STATE_REMOTE_PENDING)
    change_state (self, TP_TUBE_CHANNEL_STATE_OPEN);

  g_assert (self->priv->state == TP_TUBE_CHANNEL_STATE_OPEN);

  added = g_hash_table_new (g_direct_hash, g_direct_equal);
  removed = g_array_new (FALSE, FALSE, sizeof (TpHandle));

  g_hash_table_insert (added, GUINT_TO_POINTER (handle), bus_name);

  // Add to the global hash table as well
  g_hash_table_insert (self->priv->dbus_names, GUINT_TO_POINTER (handle), bus_name);

  tp_svc_channel_type_dbus_tube_emit_dbus_names_changed (self, added,
      removed);

  g_hash_table_destroy (added);
  g_array_free (removed, TRUE);
}

/* Called to emulate a peer connecting to an offered tube */
void
tp_tests_dbus_tube_channel_peer_disconnected (TpTestsDBusTubeChannel *self,
    TpHandle handle)
{
  GHashTable *added;
  GArray *removed;

  g_assert (self->priv->state == TP_TUBE_CHANNEL_STATE_OPEN);

  added = g_hash_table_new (g_direct_hash, g_direct_equal);
  removed = g_array_new (FALSE, FALSE, sizeof (TpHandle));

  g_array_append_val (removed, handle);

  // Remove from the global hash table as well
  g_hash_table_remove (self->priv->dbus_names, GUINT_TO_POINTER (handle));

  tp_svc_channel_type_dbus_tube_emit_dbus_names_changed (self, added,
      removed);

  g_hash_table_destroy (added);
  g_array_free (removed, TRUE);
}

void
tp_tests_dbus_tube_channel_set_close_on_accept (
    TpTestsDBusTubeChannel *self,
    gboolean close_on_accept)
{
    self->priv->close_on_accept = close_on_accept;
}

/* Contact DBus Tube */

G_DEFINE_TYPE (TpTestsContactDBusTubeChannel,
    tp_tests_contact_dbus_tube_channel,
    TP_TESTS_TYPE_DBUS_TUBE_CHANNEL)

static void
tp_tests_contact_dbus_tube_channel_init (
    TpTestsContactDBusTubeChannel *self)
{
}

static void
tp_tests_contact_dbus_tube_channel_class_init (
    TpTestsContactDBusTubeChannelClass *klass)
{
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);

  base_class->target_handle_type = TP_HANDLE_TYPE_CONTACT;
}

/* Room DBus Tube */

G_DEFINE_TYPE (TpTestsRoomDBusTubeChannel,
    tp_tests_room_dbus_tube_channel,
    TP_TESTS_TYPE_DBUS_TUBE_CHANNEL)

static void
tp_tests_room_dbus_tube_channel_init (
    TpTestsRoomDBusTubeChannel *self)
{
}

static void
tp_tests_room_dbus_tube_channel_class_init (
    TpTestsRoomDBusTubeChannelClass *klass)
{
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);

  base_class->target_handle_type = TP_HANDLE_TYPE_ROOM;
}
