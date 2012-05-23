/*
 * stream-tube-chan.c - Simple stream tube channel
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "stream-tube-chan.h"

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/svc-channel.h>
#include <telepathy-glib/gnio-util.h>

#include <gio/gunixsocketaddress.h>
#include <gio/gunixconnection.h>

#include <glib/gstdio.h>

enum
{
  PROP_SERVICE = 1,
  PROP_SUPPORTED_SOCKET_TYPES,
  PROP_PARAMETERS,
  PROP_STATE,
};

enum
{
  SIG_INCOMING_CONNECTION,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0, };


struct _TpTestsStreamTubeChannelPrivate {
    TpTubeChannelState state;
    GHashTable *supported_socket_types;

    /* Accepting side */
    GSocketService *service;
    GValue *access_control_param;

    /* Offering side */
    TpSocketAddressType address_type;
    GValue *address;
    gchar *unix_address;
    guint connection_id;

    TpSocketAccessControl access_control;

    GHashTable *parameters;

    gboolean close_on_accept;
};

static void
destroy_socket_control_list (gpointer data)
{
  GArray *tab = data;
  g_array_free (tab, TRUE);
}

static void
create_supported_socket_types (TpTestsStreamTubeChannel *self)
{
  TpSocketAccessControl access_control;
  GArray *unix_tab;

  g_assert (self->priv->supported_socket_types == NULL);
  self->priv->supported_socket_types = g_hash_table_new_full (NULL, NULL,
      NULL, destroy_socket_control_list);

  /* Socket_Address_Type_Unix */
  unix_tab = g_array_sized_new (FALSE, FALSE, sizeof (TpSocketAccessControl),
      1);
  access_control = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
  g_array_append_val (unix_tab, access_control);

  g_hash_table_insert (self->priv->supported_socket_types,
      GUINT_TO_POINTER (TP_SOCKET_ADDRESS_TYPE_UNIX), unix_tab);
}

static void
tp_tests_stream_tube_channel_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpTestsStreamTubeChannel *self = (TpTestsStreamTubeChannel *) object;

  switch (property_id)
    {
      case PROP_SERVICE:
        g_value_set_string (value, "test-service");
        break;

      case PROP_SUPPORTED_SOCKET_TYPES:
        g_value_set_boxed (value,
            self->priv->supported_socket_types);
        break;

      case PROP_PARAMETERS:
        g_value_set_boxed (value, self->priv->parameters);
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
tp_tests_stream_tube_channel_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  TpTestsStreamTubeChannel *self = (TpTestsStreamTubeChannel *) object;

  switch (property_id)
    {
      case PROP_SUPPORTED_SOCKET_TYPES:
        self->priv->supported_socket_types = g_value_dup_boxed (value);
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

static void stream_tube_iface_init (gpointer iface, gpointer data);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (TpTestsStreamTubeChannel,
    tp_tests_stream_tube_channel,
    TP_TYPE_BASE_CHANNEL,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_STREAM_TUBE,
      stream_tube_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_TUBE,
      NULL);
    )

/* type definition stuff */

static const char * tp_tests_stream_tube_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_TUBE,
    NULL
};

static void
tp_tests_stream_tube_channel_init (TpTestsStreamTubeChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
      TP_TESTS_TYPE_STREAM_TUBE_CHANNEL, TpTestsStreamTubeChannelPrivate);
}

static GObject *
constructor (GType type,
             guint n_props,
             GObjectConstructParam *props)
{
  GObject *object =
      G_OBJECT_CLASS (tp_tests_stream_tube_channel_parent_class)->constructor (
          type, n_props, props);
  TpTestsStreamTubeChannel *self = TP_TESTS_STREAM_TUBE_CHANNEL (object);

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

  if (self->priv->supported_socket_types == NULL)
    create_supported_socket_types (self);

  tp_base_channel_register (TP_BASE_CHANNEL (self));

  return object;
}

static void
dispose (GObject *object)
{
  TpTestsStreamTubeChannel *self = (TpTestsStreamTubeChannel *) object;

  if (self->priv->service != NULL)
    {
      g_socket_service_stop (self->priv->service);
      tp_clear_object (&self->priv->service);
    }

  tp_clear_pointer (&self->priv->address, tp_g_value_slice_free);
  tp_clear_pointer (&self->priv->supported_socket_types, g_hash_table_unref);
  tp_clear_pointer (&self->priv->access_control_param, tp_g_value_slice_free);

  if (self->priv->unix_address != NULL)
    g_unlink (self->priv->unix_address);

  tp_clear_pointer (&self->priv->unix_address, g_free);

  ((GObjectClass *) tp_tests_stream_tube_channel_parent_class)->dispose (
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
      tp_tests_stream_tube_channel_parent_class);

  klass->fill_immutable_properties (chan, properties);

  tp_dbus_properties_mixin_fill_properties_hash (
      G_OBJECT (chan), properties,
      TP_IFACE_CHANNEL_TYPE_STREAM_TUBE, "Service",
      TP_IFACE_CHANNEL_TYPE_STREAM_TUBE, "SupportedSocketTypes",
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
tp_tests_stream_tube_channel_class_init (TpTestsStreamTubeChannelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);
  GParamSpec *param_spec;
  static TpDBusPropertiesMixinPropImpl stream_tube_props[] = {
      { "Service", "service", NULL, },
      { "SupportedSocketTypes", "supported-socket-types", NULL },
      { NULL }
  };
  static TpDBusPropertiesMixinPropImpl tube_props[] = {
      { "Parameters", "parameters", NULL, },
      { "State", "state", NULL, },
      { NULL }
  };

  object_class->constructor = constructor;
  object_class->get_property = tp_tests_stream_tube_channel_get_property;
  object_class->set_property = tp_tests_stream_tube_channel_set_property;
  object_class->dispose = dispose;

  base_class->channel_type = TP_IFACE_CHANNEL_TYPE_STREAM_TUBE;
  base_class->interfaces = tp_tests_stream_tube_channel_interfaces;
  base_class->close = channel_close;
  base_class->fill_immutable_properties = fill_immutable_properties;

  /* base_class->target_handle_type is defined in subclasses */

  param_spec = g_param_spec_string ("service", "service name",
      "the service associated with this tube object.",
       "",
       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SERVICE, param_spec);

  param_spec = g_param_spec_boxed (
      "supported-socket-types", "Supported socket types",
      "GHashTable containing supported socket types.",
      TP_HASH_TYPE_SUPPORTED_SOCKET_MAP,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SUPPORTED_SOCKET_TYPES,
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

  signals[SIG_INCOMING_CONNECTION] = g_signal_new ("incoming-connection",
      G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_LAST,
      0, NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE,
      1, G_TYPE_IO_STREAM);

  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_IFACE_QUARK_CHANNEL_TYPE_STREAM_TUBE,
      tp_dbus_properties_mixin_getter_gobject_properties, NULL,
      stream_tube_props);

  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_IFACE_QUARK_CHANNEL_INTERFACE_TUBE,
      tp_dbus_properties_mixin_getter_gobject_properties, NULL,
      tube_props);

  g_type_class_add_private (object_class,
      sizeof (TpTestsStreamTubeChannelPrivate));
}

static void
change_state (TpTestsStreamTubeChannel *self,
  TpTubeChannelState state)
{
  self->priv->state = state;

  tp_svc_channel_interface_tube_emit_tube_channel_state_changed (self, state);
}

/* Return the address of the socket which has been shared over the tube */
GSocketAddress *
tp_tests_stream_tube_channel_get_server_address (TpTestsStreamTubeChannel *self)
{
  return tp_g_socket_address_from_variant (self->priv->address_type,
      self->priv->address, NULL);
}

static gboolean
check_address_type (TpTestsStreamTubeChannel *self,
    TpSocketAddressType address_type,
    TpSocketAccessControl access_control)
{
  GArray *arr;
  guint i;

  arr = g_hash_table_lookup (self->priv->supported_socket_types,
      GUINT_TO_POINTER (address_type));
  if (arr == NULL)
    return FALSE;

  for (i = 0; i < arr->len; i++)
    {
      if (g_array_index (arr, TpSocketAccessControl, i) == access_control)
        return TRUE;
    }

  return FALSE;
}

static void
stream_tube_offer (TpSvcChannelTypeStreamTube *iface,
    guint address_type,
    const GValue *address,
    guint access_control,
    GHashTable *parameters,
    DBusGMethodInvocation *context)
{
  TpTestsStreamTubeChannel *self = (TpTestsStreamTubeChannel *) iface;
  GError *error = NULL;

  if (self->priv->state != TP_TUBE_CHANNEL_STATE_NOT_OFFERED)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Tube is not in the not offered state");
      goto fail;
    }

  if (!check_address_type (self, address_type, access_control))
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Address type not supported with this access control");
      goto fail;
    }

  self->priv->address_type = address_type;
  self->priv->address = tp_g_value_slice_dup (address);
  self->priv->access_control = access_control;

  g_object_set (self, "parameters", parameters, NULL);

  change_state (self, TP_TUBE_CHANNEL_STATE_REMOTE_PENDING);

  tp_svc_channel_type_stream_tube_return_from_offer (context);
  return;

fail:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
service_incoming_cb (GSocketService *service,
    GSocketConnection *connection,
    GObject *source_object,
    gpointer user_data)
{
  TpTestsStreamTubeChannel *self = user_data;
  GError *error = NULL;

  if (self->priv->access_control == TP_SOCKET_ACCESS_CONTROL_CREDENTIALS)
    {
      GCredentials *creds;
      guchar byte;

      /* FIXME: we should an async version of this API (bgo #629503) */
      creds = tp_unix_connection_receive_credentials_with_byte (
              connection, &byte, NULL, &error);
      g_assert_no_error (error);

      g_assert_cmpuint (byte, ==,
          g_value_get_uchar (self->priv->access_control_param));
      g_object_unref (creds);
    }
  else if (self->priv->access_control == TP_SOCKET_ACCESS_CONTROL_PORT)
    {
      GSocketAddress *remote_addr;
      gchar *host, *remote_host;
      guint port, remote_port;

      dbus_g_type_struct_get (self->priv->access_control_param,
              0, &host,
              1, &port,
              G_MAXUINT);

      remote_addr = g_socket_connection_get_remote_address (connection, &error);
      g_assert_no_error (error);

      remote_host = g_inet_address_to_string (
              g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (remote_addr)));
      remote_port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (remote_addr));

      g_assert_cmpuint (remote_port, ==, port);
      g_assert_cmpstr (remote_host, ==, host);

      g_free (host);
      g_free (remote_host);
      g_object_unref (remote_addr);
    }

  tp_svc_channel_type_stream_tube_emit_new_local_connection (self,
      self->priv->connection_id);

  self->priv->connection_id++;

  g_signal_emit (self, signals[SIG_INCOMING_CONNECTION], 0, connection);
}

static GValue *
create_local_socket (TpTestsStreamTubeChannel *self,
    TpSocketAddressType address_type,
    TpSocketAccessControl access_control,
    GError **error)
{
  gboolean success;
  GSocketAddress *address, *effective_address;
  GValue *address_gvalue;

  switch (access_control)
    {
      case TP_SOCKET_ACCESS_CONTROL_LOCALHOST:
      case TP_SOCKET_ACCESS_CONTROL_CREDENTIALS:
      case TP_SOCKET_ACCESS_CONTROL_PORT:
        break;

      default:
        g_assert_not_reached ();
    }

  switch (address_type)
    {
      case TP_SOCKET_ADDRESS_TYPE_UNIX:
        {
          address = g_unix_socket_address_new (tmpnam (NULL));
          break;
        }

      case TP_SOCKET_ADDRESS_TYPE_IPV4:
      case TP_SOCKET_ADDRESS_TYPE_IPV6:
        {
          GInetAddress *localhost;

          localhost = g_inet_address_new_loopback (
              address_type == TP_SOCKET_ADDRESS_TYPE_IPV4 ?
              G_SOCKET_FAMILY_IPV4 : G_SOCKET_FAMILY_IPV6);
          address = g_inet_socket_address_new (localhost, 0);

          g_object_unref (localhost);
          break;
        }

      default:
        g_assert_not_reached ();
    }

  self->priv->service = g_socket_service_new ();

  success = g_socket_listener_add_address (
      G_SOCKET_LISTENER (self->priv->service),
      address, G_SOCKET_TYPE_STREAM,
      G_SOCKET_PROTOCOL_DEFAULT,
      NULL, &effective_address, NULL);
  g_assert (success);

  tp_g_signal_connect_object (self->priv->service, "incoming",
      G_CALLBACK (service_incoming_cb), self, 0);

  switch (address_type)
    {
      case TP_SOCKET_ADDRESS_TYPE_UNIX:
        self->priv->unix_address = g_strdup (g_unix_socket_address_get_path (
              G_UNIX_SOCKET_ADDRESS (effective_address)));
        address_gvalue =  tp_g_value_slice_new_bytes (
            g_unix_socket_address_get_path_len (
              G_UNIX_SOCKET_ADDRESS (effective_address)),
            g_unix_socket_address_get_path (
              G_UNIX_SOCKET_ADDRESS (effective_address)));
        break;

      case TP_SOCKET_ADDRESS_TYPE_IPV4:
      case TP_SOCKET_ADDRESS_TYPE_IPV6:
        address_gvalue = tp_g_value_slice_new_take_boxed (
            TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4,
            dbus_g_type_specialized_construct (
              TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4));

        dbus_g_type_struct_set (address_gvalue,
            0, address_type == TP_SOCKET_ADDRESS_TYPE_IPV4 ?
              "127.0.0.1" : "::1",
            1, g_inet_socket_address_get_port (
              G_INET_SOCKET_ADDRESS (effective_address)),
            G_MAXUINT);
        break;

      default:
        g_assert_not_reached ();
    }


  g_object_unref (address);
  g_object_unref (effective_address);
  return address_gvalue;
}

static void
stream_tube_accept (TpSvcChannelTypeStreamTube *iface,
    TpSocketAddressType address_type,
    TpSocketAccessControl access_control,
    const GValue *access_control_param,
    DBusGMethodInvocation *context)
{
  TpTestsStreamTubeChannel *self = (TpTestsStreamTubeChannel *) iface;
  GError *error = NULL;
  GValue *address;

  if (self->priv->state != TP_TUBE_CHANNEL_STATE_LOCAL_PENDING)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Tube is not in the local pending state");
      goto fail;
    }

  if (!check_address_type (self, address_type, access_control))
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

  address = create_local_socket (self, address_type, access_control, &error);

  self->priv->access_control = access_control;
  self->priv->access_control_param = tp_g_value_slice_dup (
      access_control_param);

  change_state (self, TP_TUBE_CHANNEL_STATE_OPEN);

  tp_svc_channel_type_stream_tube_return_from_accept (context, address);

  tp_g_value_slice_free (address);
  return;

fail:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
stream_tube_iface_init (gpointer iface,
    gpointer data)
{
  TpSvcChannelTypeStreamTubeClass *klass = iface;

#define IMPLEMENT(x) tp_svc_channel_type_stream_tube_implement_##x (klass, stream_tube_##x)
  IMPLEMENT(offer);
  IMPLEMENT(accept);
#undef IMPLEMENT
}

/* Called to emulate a peer connecting to an offered tube */
void
tp_tests_stream_tube_channel_peer_connected (TpTestsStreamTubeChannel *self,
    GIOStream *stream,
    TpHandle handle)
{
  GValue *connection_param;

  if (self->priv->state == TP_TUBE_CHANNEL_STATE_REMOTE_PENDING)
    change_state (self, TP_TUBE_CHANNEL_STATE_OPEN);

  g_assert (self->priv->state == TP_TUBE_CHANNEL_STATE_OPEN);

  switch (self->priv->access_control)
    {
      case TP_SOCKET_ACCESS_CONTROL_LOCALHOST:
        connection_param = tp_g_value_slice_new_static_string ("dummy");
        break;

      case TP_SOCKET_ACCESS_CONTROL_CREDENTIALS:
        {
          GError *error = NULL;
          guchar byte = g_random_int_range (0, G_MAXUINT8);

          /* FIXME: we should an async version of this API (bgo #629503) */
          tp_unix_connection_send_credentials_with_byte (
              G_SOCKET_CONNECTION (stream), byte, NULL, &error);
          g_assert_no_error (error);

          connection_param = tp_g_value_slice_new_byte (byte);
        }
        break;

      case TP_SOCKET_ACCESS_CONTROL_PORT:
        {
          GSocketAddress *addr;
          GError *error = NULL;

          addr = g_socket_connection_get_local_address (
              G_SOCKET_CONNECTION (stream), &error);
          g_assert_no_error (error);

          connection_param = tp_g_value_slice_new_take_boxed (
              TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4,
              dbus_g_type_specialized_construct (
                TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4));

          dbus_g_type_struct_set (connection_param,
              0, "badger",
              1, g_inet_socket_address_get_port (
                G_INET_SOCKET_ADDRESS (addr)),
              G_MAXUINT);

          g_object_unref (addr);
        }
        break;

      default:
        g_assert_not_reached ();
    }

  tp_svc_channel_type_stream_tube_emit_new_remote_connection (self, handle,
      connection_param, self->priv->connection_id);

  self->priv->connection_id++;

  tp_g_value_slice_free (connection_param);
}

/* Called to emulate a peer connecting to an offered tube */
void
tp_tests_stream_tube_channel_peer_connected_no_stream (TpTestsStreamTubeChannel *self,
    const GValue *connection_param,
    TpHandle handle)
{
  if (self->priv->state == TP_TUBE_CHANNEL_STATE_REMOTE_PENDING)
    change_state (self, TP_TUBE_CHANNEL_STATE_OPEN);

  g_assert (self->priv->state == TP_TUBE_CHANNEL_STATE_OPEN);

  tp_svc_channel_type_stream_tube_emit_new_remote_connection (self, handle,
      connection_param, self->priv->connection_id);

  self->priv->connection_id++;
}

void
tp_tests_stream_tube_channel_last_connection_disconnected (
    TpTestsStreamTubeChannel *self,
    const gchar *error)
{
  tp_svc_channel_type_stream_tube_emit_connection_closed (self,
      self->priv->connection_id - 1, error, "kaboum");
}

void
tp_tests_stream_tube_channel_set_close_on_accept (
    TpTestsStreamTubeChannel *self,
    gboolean close_on_accept)
{
    self->priv->close_on_accept = close_on_accept;
}

/* Contact Stream Tube */

G_DEFINE_TYPE (TpTestsContactStreamTubeChannel,
    tp_tests_contact_stream_tube_channel,
    TP_TESTS_TYPE_STREAM_TUBE_CHANNEL)

static void
tp_tests_contact_stream_tube_channel_init (
    TpTestsContactStreamTubeChannel *self)
{
}

static void
tp_tests_contact_stream_tube_channel_class_init (
    TpTestsContactStreamTubeChannelClass *klass)
{
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);

  base_class->target_handle_type = TP_HANDLE_TYPE_CONTACT;
}

/* Room Stream Tube */

G_DEFINE_TYPE (TpTestsRoomStreamTubeChannel,
    tp_tests_room_stream_tube_channel,
    TP_TESTS_TYPE_STREAM_TUBE_CHANNEL)

static void
tp_tests_room_stream_tube_channel_init (
    TpTestsRoomStreamTubeChannel *self)
{
}

static void
tp_tests_room_stream_tube_channel_class_init (
    TpTestsRoomStreamTubeChannelClass *klass)
{
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);

  base_class->target_handle_type = TP_HANDLE_TYPE_ROOM;
}
