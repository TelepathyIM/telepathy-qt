/*
 * chan.c - an example text channel talking to a particular
 * contact. Similar code is used for 1-1 IM channels in many protocols
 * (IRC private messages ("/query"), XMPP IM etc.)
 *
 * Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

// We need to use the deprecated TpTextMixin here to test compatibility functionality
#define _TP_IGNORE_DEPRECATIONS

#include "chan.h"

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/svc-channel.h>

static void text_iface_init (gpointer iface, gpointer data);
static void channel_iface_init (gpointer iface, gpointer data);
static void destroyable_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (ExampleEchoChannel,
    example_echo_channel,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_TEXT, text_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_DESTROYABLE,
      destroyable_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_IFACE, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_EXPORTABLE_CHANNEL, NULL))

/* type definition stuff */

enum
{
  PROP_OBJECT_PATH = 1,
  PROP_CHANNEL_TYPE,
  PROP_HANDLE_TYPE,
  PROP_HANDLE,
  PROP_TARGET_ID,
  PROP_REQUESTED,
  PROP_INITIATOR_HANDLE,
  PROP_INITIATOR_ID,
  PROP_CONNECTION,
  PROP_INTERFACES,
  PROP_CHANNEL_DESTROYED,
  PROP_CHANNEL_PROPERTIES,
  N_PROPS
};

struct _ExampleEchoChannelPrivate
{
  TpBaseConnection *conn;
  gchar *object_path;
  TpHandle handle;
  TpHandle initiator;

  /* These are really booleans, but gboolean is signed. Thanks, GLib */
  unsigned closed:1;
  unsigned disposed:1;
};

static const char * example_echo_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_DESTROYABLE,
    NULL
};

static void
example_echo_channel_init (ExampleEchoChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, EXAMPLE_TYPE_ECHO_CHANNEL,
      ExampleEchoChannelPrivate);
}

static GObject *
constructor (GType type,
             guint n_props,
             GObjectConstructParam *props)
{
  GObject *object =
      G_OBJECT_CLASS (example_echo_channel_parent_class)->constructor (type,
          n_props, props);
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (object);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);

  tp_dbus_daemon_register_object (
      tp_base_connection_get_dbus_daemon (self->priv->conn),
      self->priv->object_path, self);

  tp_text_mixin_init (object, G_STRUCT_OFFSET (ExampleEchoChannel, text),
      contact_repo);

  tp_text_mixin_set_message_types (object,
      TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL,
      TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION,
      TP_CHANNEL_TEXT_MESSAGE_TYPE_NOTICE,
      G_MAXUINT);

  return object;
}

static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;
    case PROP_CHANNEL_TYPE:
      g_value_set_static_string (value, TP_IFACE_CHANNEL_TYPE_TEXT);
      break;
    case PROP_HANDLE_TYPE:
      g_value_set_uint (value, TP_HANDLE_TYPE_CONTACT);
      break;
    case PROP_HANDLE:
      g_value_set_uint (value, self->priv->handle);
      break;
    case PROP_TARGET_ID:
        {
          TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
              self->priv->conn, TP_HANDLE_TYPE_CONTACT);

          g_value_set_string (value,
              tp_handle_inspect (contact_repo, self->priv->handle));
        }
      break;
    case PROP_REQUESTED:
      g_value_set_boolean (value,
          (self->priv->initiator == self->priv->conn->self_handle));
      break;
    case PROP_INITIATOR_HANDLE:
      g_value_set_uint (value, self->priv->initiator);
      break;
    case PROP_INITIATOR_ID:
        {
          TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
              self->priv->conn, TP_HANDLE_TYPE_CONTACT);

          g_value_set_string (value,
              self->priv->initiator == 0
                  ? ""
                  : tp_handle_inspect (contact_repo, self->priv->initiator));
        }
      break;
    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->conn);
      break;
    case PROP_INTERFACES:
      g_value_set_boxed (value, example_echo_channel_interfaces);
      break;
    case PROP_CHANNEL_DESTROYED:
      g_value_set_boolean (value, self->priv->closed);
      break;
    case PROP_CHANNEL_PROPERTIES:
      g_value_take_boxed (value,
          tp_dbus_properties_mixin_make_properties_hash (object,
              TP_IFACE_CHANNEL, "ChannelType",
              TP_IFACE_CHANNEL, "TargetHandleType",
              TP_IFACE_CHANNEL, "TargetHandle",
              TP_IFACE_CHANNEL, "TargetID",
              TP_IFACE_CHANNEL, "InitiatorHandle",
              TP_IFACE_CHANNEL, "InitiatorID",
              TP_IFACE_CHANNEL, "Requested",
              TP_IFACE_CHANNEL, "Interfaces",
              NULL));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
set_property (GObject *object,
              guint property_id,
              const GValue *value,
              GParamSpec *pspec)
{
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_free (self->priv->object_path);
      self->priv->object_path = g_value_dup_string (value);
      break;
    case PROP_HANDLE:
      /* we don't ref it here because we don't necessarily have access to the
       * contact repo yet - instead we ref it in the constructor.
       */
      self->priv->handle = g_value_get_uint (value);
      break;
    case PROP_INITIATOR_HANDLE:
      /* likewise */
      self->priv->initiator = g_value_get_uint (value);
      break;
    case PROP_HANDLE_TYPE:
    case PROP_CHANNEL_TYPE:
      /* these properties are writable in the interface, but not actually
       * meaningfully changable on this channel, so we do nothing */
      break;
    case PROP_CONNECTION:
      self->priv->conn = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
dispose (GObject *object)
{
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (object);

  if (self->priv->disposed)
    return;

  self->priv->disposed = TRUE;

  if (!self->priv->closed)
    {
      self->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }

  ((GObjectClass *) example_echo_channel_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (object);

  g_free (self->priv->object_path);

  tp_text_mixin_finalize (object);

  ((GObjectClass *) example_echo_channel_parent_class)->finalize (object);
}

static void
example_echo_channel_class_init (ExampleEchoChannelClass *klass)
{
  static TpDBusPropertiesMixinPropImpl channel_props[] = {
      { "TargetHandleType", "handle-type", NULL },
      { "TargetHandle", "handle", NULL },
      { "ChannelType", "channel-type", NULL },
      { "Interfaces", "interfaces", NULL },
      { "TargetID", "target-id", NULL },
      { "Requested", "requested", NULL },
      { "InitiatorHandle", "initiator-handle", NULL },
      { "InitiatorID", "initiator-id", NULL },
      { NULL }
  };
  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
      { TP_IFACE_CHANNEL,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        channel_props,
      },
      { NULL }
  };
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  g_type_class_add_private (klass, sizeof (ExampleEchoChannelPrivate));

  object_class->constructor = constructor;
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->dispose = dispose;
  object_class->finalize = finalize;

  g_object_class_override_property (object_class, PROP_OBJECT_PATH,
      "object-path");
  g_object_class_override_property (object_class, PROP_CHANNEL_TYPE,
      "channel-type");
  g_object_class_override_property (object_class, PROP_HANDLE_TYPE,
      "handle-type");
  g_object_class_override_property (object_class, PROP_HANDLE, "handle");

  g_object_class_override_property (object_class, PROP_CHANNEL_DESTROYED,
      "channel-destroyed");
  g_object_class_override_property (object_class, PROP_CHANNEL_PROPERTIES,
      "channel-properties");

  param_spec = g_param_spec_object ("connection", "TpBaseConnection object",
      "Connection object that owns this channel",
      TP_TYPE_BASE_CONNECTION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_boxed ("interfaces", "Extra D-Bus interfaces",
      "Additional Channel.Interface.* interfaces",
      G_TYPE_STRV,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INTERFACES, param_spec);

  param_spec = g_param_spec_string ("target-id", "Peer's ID",
      "The string obtained by inspecting the target handle",
      NULL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TARGET_ID, param_spec);

  param_spec = g_param_spec_uint ("initiator-handle", "Initiator's handle",
      "The contact who initiated the channel",
      0, G_MAXUINT32, 0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INITIATOR_HANDLE,
      param_spec);

  param_spec = g_param_spec_string ("initiator-id", "Initiator's ID",
      "The string obtained by inspecting the initiator-handle",
      NULL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INITIATOR_ID,
      param_spec);

  param_spec = g_param_spec_boolean ("requested", "Requested?",
      "True if this channel was requested by the local user",
      FALSE,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_REQUESTED, param_spec);

  tp_text_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleEchoChannelClass, text_class));

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleEchoChannelClass, dbus_properties_class));
}

static void
example_echo_channel_close (ExampleEchoChannel *self)
{
  GObject *object = (GObject *) self;

  if (!self->priv->closed)
    {
      TpHandle first_sender;

      /* The manager wants to be able to respawn the channel if it has pending
       * messages. When respawned, the channel must have the initiator set
       * to the contact who sent us those messages (if it isn't already),
       * and the messages must be marked as having been rescued so they
       * don't get logged twice. */
      if (tp_text_mixin_has_pending_messages (object, &first_sender))
        {
          if (self->priv->initiator != first_sender)
            {
              self->priv->initiator = first_sender;
            }

          tp_text_mixin_set_rescued (object);
        }
      else
        {
          /* No pending messages, so it's OK to really close */
          self->priv->closed = TRUE;
        }

      tp_svc_channel_emit_closed (self);
    }
}

static void
channel_close (TpSvcChannel *iface,
               DBusGMethodInvocation *context)
{
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (iface);

  example_echo_channel_close (self);
  tp_svc_channel_return_from_close (context);
}

static void
channel_get_channel_type (TpSvcChannel *iface,
                          DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_channel_type (context,
      TP_IFACE_CHANNEL_TYPE_TEXT);
}

static void
channel_get_handle (TpSvcChannel *iface,
                    DBusGMethodInvocation *context)
{
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (iface);

  tp_svc_channel_return_from_get_handle (context, TP_HANDLE_TYPE_CONTACT,
      self->priv->handle);
}

static void
channel_get_interfaces (TpSvcChannel *iface,
                        DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_interfaces (context,
      example_echo_channel_interfaces);
}

static void
channel_iface_init (gpointer iface,
                    gpointer data)
{
  TpSvcChannelClass *klass = iface;

#define IMPLEMENT(x) tp_svc_channel_implement_##x (klass, channel_##x)
  IMPLEMENT (close);
  IMPLEMENT (get_channel_type);
  IMPLEMENT (get_handle);
  IMPLEMENT (get_interfaces);
#undef IMPLEMENT
}

static void
text_send (TpSvcChannelTypeText *iface,
           guint type,
           const gchar *text,
           DBusGMethodInvocation *context)
{
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (iface);
  time_t timestamp = time (NULL);
  gchar *echo;
  guint echo_type = type;

  /* Send should return just before Sent is emitted. */
  tp_svc_channel_type_text_return_from_send (context);

  /* Tell the client that the message was submitted for sending */
  tp_svc_channel_type_text_emit_sent ((GObject *) self, timestamp, type, text);

  /* Pretend that the remote contact has replied. Normally, you'd
   * call tp_text_mixin_receive or tp_text_mixin_receive_with_flags
   * in response to network events */

  switch (type)
    {
    case TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL:
      echo = g_strdup_printf ("You said: %s", text);
      break;
    case TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION:
      echo = g_strdup_printf ("notices that the user %s", text);
      break;
    case TP_CHANNEL_TEXT_MESSAGE_TYPE_NOTICE:
      echo = g_strdup_printf ("You sent a notice: %s", text);
      break;
    default:
      echo = g_strdup_printf ("You sent some weird message type, %u: \"%s\"",
          type, text);
      echo_type = TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL;
    }

  tp_text_mixin_receive ((GObject *) self, echo_type, self->priv->handle,
      timestamp, echo);

  g_free (echo);
}

static void
text_iface_init (gpointer iface,
                 gpointer data)
{
  TpSvcChannelTypeTextClass *klass = iface;

  tp_text_mixin_iface_init (iface, data);
#define IMPLEMENT(x) tp_svc_channel_type_text_implement_##x (klass, text_##x)
  IMPLEMENT (send);
#undef IMPLEMENT
}

static void
destroyable_destroy (TpSvcChannelInterfaceDestroyable *iface,
                     DBusGMethodInvocation *context)
{
  ExampleEchoChannel *self = EXAMPLE_ECHO_CHANNEL (iface);

  tp_text_mixin_clear ((GObject *) self);
  example_echo_channel_close (self);
  g_assert (self->priv->closed);
  tp_svc_channel_interface_destroyable_return_from_destroy (context);
}

static void
destroyable_iface_init (gpointer iface,
                        gpointer data)
{
  TpSvcChannelInterfaceDestroyableClass *klass = iface;

#define IMPLEMENT(x) \
  tp_svc_channel_interface_destroyable_implement_##x (klass, destroyable_##x)
  IMPLEMENT (destroy);
#undef IMPLEMENT
}
