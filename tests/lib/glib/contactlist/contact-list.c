/*
 * An example ContactList channel with handle type LIST or GROUP
 *
 * Copyright © 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "contact-list.h"

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/exportable-channel.h>
#include <telepathy-glib/svc-channel.h>

#include "contact-list-manager.h"

static void channel_iface_init (gpointer iface, gpointer data);
static void list_channel_iface_init (gpointer iface, gpointer data);
static void group_channel_iface_init (gpointer iface, gpointer data);

/* Abstract base class */
G_DEFINE_TYPE_WITH_CODE (ExampleContactListBase, example_contact_list_base,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_CONTACT_LIST, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_GROUP,
      tp_group_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_EXPORTABLE_CHANNEL, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_IFACE, NULL))

/* Subclass for handle type LIST */
G_DEFINE_TYPE_WITH_CODE (ExampleContactList, example_contact_list,
    EXAMPLE_TYPE_CONTACT_LIST_BASE,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, list_channel_iface_init))

/* Subclass for handle type GROUP */
G_DEFINE_TYPE_WITH_CODE (ExampleContactGroup, example_contact_group,
    EXAMPLE_TYPE_CONTACT_LIST_BASE,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, group_channel_iface_init))

static const gchar *contact_list_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_GROUP,
    NULL
};

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
  PROP_MANAGER,
  PROP_INTERFACES,
  PROP_CHANNEL_DESTROYED,
  PROP_CHANNEL_PROPERTIES,
  N_PROPS
};

struct _ExampleContactListBasePrivate
{
  TpBaseConnection *conn;
  ExampleContactListManager *manager;
  gchar *object_path;
  TpHandleType handle_type;
  TpHandle handle;

  /* These are really booleans, but gboolean is signed. Thanks, GLib */
  unsigned closed:1;
  unsigned disposed:1;
};

struct _ExampleContactListPrivate
{
  int dummy:1;
};

struct _ExampleContactGroupPrivate
{
  int dummy:1;
};

static void
example_contact_list_base_init (ExampleContactListBase *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CONTACT_LIST_BASE, ExampleContactListBasePrivate);
}

static void
example_contact_list_init (ExampleContactList *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, EXAMPLE_TYPE_CONTACT_LIST,
      ExampleContactListPrivate);
}

static void
example_contact_group_init (ExampleContactGroup *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, EXAMPLE_TYPE_CONTACT_GROUP,
      ExampleContactGroupPrivate);
}

static void
constructed (GObject *object)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);
  void (*chain_up) (GObject *) =
    ((GObjectClass *) example_contact_list_base_parent_class)->constructed;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  TpHandle self_handle = self->priv->conn->self_handle;

  if (chain_up != NULL)
    chain_up (object);

  g_assert (TP_IS_BASE_CONNECTION (self->priv->conn));
  g_assert (EXAMPLE_IS_CONTACT_LIST_MANAGER (self->priv->manager));

  tp_dbus_daemon_register_object (
      tp_base_connection_get_dbus_daemon (self->priv->conn),
      self->priv->object_path, self);

  tp_group_mixin_init (object, G_STRUCT_OFFSET (ExampleContactListBase, group),
      contact_repo, self_handle);
  /* Both the subclasses have full support for telepathy-spec 0.17.6. */
  tp_group_mixin_change_flags (object,
      TP_CHANNEL_GROUP_FLAG_PROPERTIES, 0);
}

static void
list_constructed (GObject *object)
{
  ExampleContactList *self = EXAMPLE_CONTACT_LIST (object);
  void (*chain_up) (GObject *) =
    ((GObjectClass *) example_contact_list_parent_class)->constructed;

  if (chain_up != NULL)
    chain_up (object);

  g_assert (self->parent.priv->handle_type == TP_HANDLE_TYPE_LIST);

  switch (self->parent.priv->handle)
    {
    case EXAMPLE_CONTACT_LIST_PUBLISH:
      /* We can stop publishing presence to people, but we can't
       * start sending people our presence unless they ask for it.
       *
       * (We can accept people's requests to see our presence - but that's
       * always allowed, so there's no flag.)
       */
      tp_group_mixin_change_flags (object,
          TP_CHANNEL_GROUP_FLAG_CAN_REMOVE, 0);
      break;
    case EXAMPLE_CONTACT_LIST_STORED:
    case EXAMPLE_CONTACT_LIST_DENY:
      /* We can add people to our roster (not that that's very useful without
       * also adding them to subscribe), and we can remove them altogether
       * (which implicitly removes them from subscribe, publish, and all
       * user-defined groups).
       *
       * Similarly, we can block and unblock people (i.e. add/remove them
       * to/from the deny list)
       */
      tp_group_mixin_change_flags (object,
          TP_CHANNEL_GROUP_FLAG_CAN_ADD | TP_CHANNEL_GROUP_FLAG_CAN_REMOVE, 0);
      break;
    case EXAMPLE_CONTACT_LIST_SUBSCRIBE:
      /* We can ask people to show us their presence, attaching a message.
       * We can also cancel (rescind) requests that they haven't replied to,
       * and stop receiving their presence after they allow it.
       */
      tp_group_mixin_change_flags (object,
          TP_CHANNEL_GROUP_FLAG_CAN_ADD | TP_CHANNEL_GROUP_FLAG_MESSAGE_ADD |
          TP_CHANNEL_GROUP_FLAG_CAN_REMOVE |
          TP_CHANNEL_GROUP_FLAG_CAN_RESCIND,
          0);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
group_constructed (GObject *object)
{
  ExampleContactGroup *self = EXAMPLE_CONTACT_GROUP (object);
  void (*chain_up) (GObject *) =
    ((GObjectClass *) example_contact_group_parent_class)->constructed;

  if (chain_up != NULL)
    chain_up (object);

  g_assert (self->parent.priv->handle_type == TP_HANDLE_TYPE_GROUP);

  /* We can add people to user-defined groups, and also remove them. */
  tp_group_mixin_change_flags (object,
      TP_CHANNEL_GROUP_FLAG_CAN_ADD | TP_CHANNEL_GROUP_FLAG_CAN_REMOVE, 0);
}


static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;
    case PROP_CHANNEL_TYPE:
      g_value_set_static_string (value, TP_IFACE_CHANNEL_TYPE_CONTACT_LIST);
      break;
    case PROP_HANDLE_TYPE:
      g_value_set_uint (value, self->priv->handle_type);
      break;
    case PROP_HANDLE:
      g_value_set_uint (value, self->priv->handle);
      break;
    case PROP_TARGET_ID:
        {
          TpHandleRepoIface *handle_repo = tp_base_connection_get_handles (
              self->priv->conn, self->priv->handle_type);

          g_value_set_string (value,
              tp_handle_inspect (handle_repo, self->priv->handle));
        }
      break;
    case PROP_REQUESTED:
      g_value_set_boolean (value, FALSE);
      break;
    case PROP_INITIATOR_HANDLE:
      g_value_set_uint (value, 0);
      break;
    case PROP_INITIATOR_ID:
      g_value_set_static_string (value, "");
      break;
    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->conn);
      break;
    case PROP_MANAGER:
      g_value_set_object (value, self->priv->manager);
      break;
    case PROP_INTERFACES:
      g_value_set_boxed (value, contact_list_interfaces);
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
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_free (self->priv->object_path);
      self->priv->object_path = g_value_dup_string (value);
      break;
    case PROP_HANDLE:
      /* we don't ref it here because we don't necessarily have access to the
       * repository (or even type) yet - instead we ref it in the constructor.
       */
      self->priv->handle = g_value_get_uint (value);
      break;
    case PROP_HANDLE_TYPE:
      self->priv->handle_type = g_value_get_uint (value);
      break;
    case PROP_CHANNEL_TYPE:
      /* this property is writable in the interface, but not actually
       * meaningfully changable on this channel, so we do nothing */
      break;
    case PROP_CONNECTION:
      self->priv->conn = g_value_get_object (value);
      break;
    case PROP_MANAGER:
      self->priv->manager = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
dispose (GObject *object)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);

  if (self->priv->disposed)
    return;

  self->priv->disposed = TRUE;

  if (!self->priv->closed)
    {
      self->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }

  ((GObjectClass *) example_contact_list_base_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);

  g_free (self->priv->object_path);
  tp_group_mixin_finalize (object);

  ((GObjectClass *) example_contact_list_base_parent_class)->finalize (object);
}

static gboolean
group_add_member (GObject *object,
                  TpHandle handle,
                  const gchar *message,
                  GError **error)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);

  return example_contact_list_manager_add_to_group (self->priv->manager,
      object, self->priv->handle, handle, message, error);
}

static gboolean
group_remove_member (GObject *object,
                     TpHandle handle,
                     const gchar *message,
                     GError **error)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);

  return example_contact_list_manager_remove_from_group (self->priv->manager,
      object, self->priv->handle, handle, message, error);
}

static gboolean
list_add_member (GObject *object,
                 TpHandle handle,
                 const gchar *message,
                 GError **error)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);

  return example_contact_list_manager_add_to_list (self->priv->manager,
      object, self->priv->handle, handle, message, error);
}

static gboolean
list_remove_member (GObject *object,
                    TpHandle handle,
                    const gchar *message,
                    GError **error)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (object);

  return example_contact_list_manager_remove_from_list (self->priv->manager,
      object, self->priv->handle, handle, message, error);
}

static void
example_contact_list_base_class_init (ExampleContactListBaseClass *klass)
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

  g_type_class_add_private (klass, sizeof (ExampleContactListBasePrivate));

  object_class->constructed = constructed;
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

  param_spec = g_param_spec_object ("manager", "ExampleContactListManager",
      "ExampleContactListManager object that owns this channel",
      EXAMPLE_TYPE_CONTACT_LIST_MANAGER,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MANAGER, param_spec);

  param_spec = g_param_spec_boxed ("interfaces", "Extra D-Bus interfaces",
      "Additional Channel.Interface.* interfaces",
      G_TYPE_STRV,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INTERFACES, param_spec);

  param_spec = g_param_spec_string ("target-id", "Chatroom's ID",
      "The string obtained by inspecting the MUC's handle",
      NULL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TARGET_ID, param_spec);

  param_spec = g_param_spec_uint ("initiator-handle", "Initiator's handle",
      "The contact who initiated the channel",
      0, G_MAXUINT32, 0,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
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

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleContactListBaseClass, dbus_properties_class));

  /* Group mixin is initialized separately for each subclass - they have
   *  different callbacks */
}

static void
example_contact_list_class_init (ExampleContactListClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  g_type_class_add_private (klass, sizeof (ExampleContactListPrivate));

  object_class->constructed = list_constructed;

  tp_group_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleContactListBaseClass, group_class),
      list_add_member,
      list_remove_member);
  tp_group_mixin_init_dbus_properties (object_class);
}

static void
example_contact_group_class_init (ExampleContactGroupClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  g_type_class_add_private (klass, sizeof (ExampleContactGroupPrivate));

  object_class->constructed = group_constructed;

  tp_group_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleContactListBaseClass, group_class),
      group_add_member,
      group_remove_member);
  tp_group_mixin_init_dbus_properties (object_class);
}

static void
list_channel_close (TpSvcChannel *iface G_GNUC_UNUSED,
                    DBusGMethodInvocation *context)
{
  GError e = { TP_ERROR, TP_ERROR_NOT_IMPLEMENTED,
      "ContactList channels with handle type LIST may not be closed" };

  dbus_g_method_return_error (context, &e);
}

static void
group_channel_close (TpSvcChannel *iface,
                     DBusGMethodInvocation *context)
{
  ExampleContactGroup *self = EXAMPLE_CONTACT_GROUP (iface);
  ExampleContactListBase *base = EXAMPLE_CONTACT_LIST_BASE (iface);

  if (tp_handle_set_size (base->group.members) > 0)
    {
      GError e = { TP_ERROR, TP_ERROR_NOT_AVAILABLE,
          "Non-empty groups may not be deleted (closed)" };

      dbus_g_method_return_error (context, &e);
      return;
    }

  if (!base->priv->closed)
    {
      /* If this was a real connection manager we'd delete the group here,
       * if such a concept existed in the protocol (in XMPP, it doesn't).
       *
       * Afterwards, close the channel:
       */
      base->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }

  tp_svc_channel_return_from_close (context);
}

static void
channel_get_channel_type (TpSvcChannel *iface G_GNUC_UNUSED,
                          DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_channel_type (context,
      TP_IFACE_CHANNEL_TYPE_CONTACT_LIST);
}

static void
channel_get_handle (TpSvcChannel *iface,
                    DBusGMethodInvocation *context)
{
  ExampleContactListBase *self = EXAMPLE_CONTACT_LIST_BASE (iface);

  tp_svc_channel_return_from_get_handle (context, self->priv->handle_type,
      self->priv->handle);
}

static void
channel_get_interfaces (TpSvcChannel *iface G_GNUC_UNUSED,
                        DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_interfaces (context,
      contact_list_interfaces);
}

static void
channel_iface_init (gpointer iface,
                    gpointer data)
{
  TpSvcChannelClass *klass = iface;

#define IMPLEMENT(x) tp_svc_channel_implement_##x (klass, channel_##x)
  /* close is implemented in subclasses, so don't IMPLEMENT (close); */
  IMPLEMENT (get_channel_type);
  IMPLEMENT (get_handle);
  IMPLEMENT (get_interfaces);
#undef IMPLEMENT
}

static void
list_channel_iface_init (gpointer iface,
                         gpointer data G_GNUC_UNUSED)
{
  TpSvcChannelClass *klass = iface;

#define IMPLEMENT(x) tp_svc_channel_implement_##x (klass, list_channel_##x)
  IMPLEMENT (close);
#undef IMPLEMENT
}

static void
group_channel_iface_init (gpointer iface,
                          gpointer data G_GNUC_UNUSED)
{
  TpSvcChannelClass *klass = iface;

#define IMPLEMENT(x) tp_svc_channel_implement_##x (klass, group_channel_##x)
  IMPLEMENT (close);
#undef IMPLEMENT
}
