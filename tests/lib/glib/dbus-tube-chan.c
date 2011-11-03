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

#include <gio/gunixsocketaddress.h>
#include <gio/gunixconnection.h>

#include <glib/gstdio.h>

enum
{
  PROP_SERVICE_NAME = 1,
  PROP_DBUS_NAMES,
  PROP_SUPPORTED_ACCESS_CONTROLS,
  PROP_PARAMETERS,
  PROP_STATE,
};

struct _TpTestsDBusTubeChannelPrivate {
    TpTubeChannelState state;

    /* TpHandle -> gchar * */
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

  self->priv->dbus_names = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, NULL, g_free);
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

  tp_text_mixin_class_init (object_class,
      G_STRUCT_OFFSET (TpTestsDBusTubeChannelClass, text_class));

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

  param_spec = g_param_spec_boxed (
      "parameters", "Parameters",
      "parameters of the tube",
      TP_HASH_TYPE_STRING_VARIANT_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
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
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "Tube is not in the not offered state");
      goto fail;
    }

  if (!check_access_control (self, access_control))
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "Address type not supported with this access control");
      goto fail;
    }

//   self->priv->address_type = address_type;
//   self->priv->address = tp_g_value_slice_dup (address);
//   self->priv->access_control = access_control;

  g_object_set (self, "parameters", parameters, NULL);

  change_state (self, TP_TUBE_CHANNEL_STATE_REMOTE_PENDING);

  tp_svc_channel_type_stream_tube_return_from_offer (context);
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
  gchar *address = NULL;

  if (self->priv->state != TP_TUBE_CHANNEL_STATE_LOCAL_PENDING)
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "Tube is not in the local pending state");
      goto fail;
    }

  if (!check_access_control (self, access_control))
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "Address type not supported with this access control");
      goto fail;
    }

  if (self->priv->close_on_accept)
    {
      tp_base_channel_close (TP_BASE_CHANNEL (self));
      return;
    }

//   address = create_local_socket (self, address_type, access_control, &error);
//
//   self->priv->access_control = access_control;
//   self->priv->access_control_param = tp_g_value_slice_dup (
//       access_control_param);

  change_state (self, TP_TUBE_CHANNEL_STATE_OPEN);

  tp_svc_channel_type_dbus_tube_return_from_accept (context, address);

  g_free (address);
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
