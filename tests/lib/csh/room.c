/*
 * room.c - a chatroom channel
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "room.h"

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/svc-channel.h>
#include <telepathy-glib/svc-generic.h>

static void text_iface_init (gpointer iface, gpointer data);
static void channel_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (ExampleCSHRoomChannel,
    example_csh_room_channel,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_TEXT, text_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_GROUP,
      tp_group_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_EXPORTABLE_CHANNEL, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_IFACE, NULL))

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

struct _ExampleCSHRoomChannelPrivate
{
  TpBaseConnection *conn;
  gchar *object_path;
  TpHandle handle;
  TpHandle initiator;
  TpIntSet *remote;
  guint accept_invitations_timeout;

  /* These are really booleans, but gboolean is signed. Thanks, GLib */
  unsigned closed:1;
  unsigned disposed:1;
};


static const char * example_csh_room_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_GROUP,
    NULL
};


static void
example_csh_room_channel_init (ExampleCSHRoomChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, EXAMPLE_TYPE_CSH_ROOM_CHANNEL,
      ExampleCSHRoomChannelPrivate);
}

static TpHandle
suggest_room_identity (ExampleCSHRoomChannel *self)
{
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  TpHandleRepoIface *room_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_ROOM);
  gchar *nick, *id;
  TpHandle ret;

  nick = g_strdup (tp_handle_inspect (contact_repo,
        self->priv->conn->self_handle));
  g_strdelimit (nick, "@", '\0');
  id = g_strdup_printf ("%s@%s", nick, tp_handle_inspect (room_repo,
        self->priv->handle));
  g_free (nick);

  ret = tp_handle_ensure (contact_repo, id, NULL, NULL);
  g_free (id);

  g_assert (ret != 0);
  return ret;
}


/* This timeout callback represents a successful join. In a real CM it'd
 * happen in response to network events, rather than just a timer */
static void
complete_join (ExampleCSHRoomChannel *self)
{
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  TpHandleRepoIface *room_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_ROOM);
  const gchar *room_name = tp_handle_inspect (room_repo, self->priv->handle);
  gchar *str;
  TpHandle alice_local, bob_local, chris_local, anon_local;
  TpHandle alice_global, bob_global, chris_global;
  TpGroupMixin *mixin = TP_GROUP_MIXIN (self);
  TpIntSet *added;

  /* For this example, we assume that all chatrooms initially contain
   * Alice, Bob and Chris (and that their global IDs are also known),
   * and they also contain one anonymous user. */

  str = g_strdup_printf ("alice@%s", room_name);
  alice_local = tp_handle_ensure (contact_repo, str, NULL, NULL);
  g_free (str);
  alice_global = tp_handle_ensure (contact_repo, "alice@alpha", NULL, NULL);

  str = g_strdup_printf ("bob@%s", room_name);
  bob_local = tp_handle_ensure (contact_repo, str, NULL, NULL);
  g_free (str);
  bob_global = tp_handle_ensure (contact_repo, "bob@beta", NULL, NULL);

  str = g_strdup_printf ("chris@%s", room_name);
  chris_local = tp_handle_ensure (contact_repo, str, NULL, NULL);
  g_free (str);
  chris_global = tp_handle_ensure (contact_repo, "chris@chi", NULL, NULL);

  str = g_strdup_printf ("anonymous coward@%s", room_name);
  anon_local = tp_handle_ensure (contact_repo, str, NULL, NULL);
  g_free (str);

  /* If our chosen nick is not available, pretend the server would
   * automatically rename us on entry. */
  if (mixin->self_handle == alice_local ||
      mixin->self_handle == bob_local ||
      mixin->self_handle == chris_local ||
      mixin->self_handle == anon_local)
    {
      TpHandle new_self;
      TpIntSet *rp = tp_intset_new ();
      TpIntSet *removed = tp_intset_new ();

      str = g_strdup_printf ("renamed by server@%s", room_name);
      new_self = tp_handle_ensure (contact_repo, str, NULL, NULL);
      g_free (str);

      tp_intset_add (rp, new_self);
      tp_intset_add (removed, mixin->self_handle);

      tp_group_mixin_add_handle_owner ((GObject *) self, new_self,
          self->priv->conn->self_handle);
      tp_group_mixin_change_self_handle ((GObject *) self, new_self);

      tp_group_mixin_change_members ((GObject *) self, "", NULL, removed, NULL,
          rp, 0, TP_CHANNEL_GROUP_CHANGE_REASON_RENAMED);

      tp_handle_unref (contact_repo, new_self);
      tp_intset_destroy (removed);
      tp_intset_destroy (rp);
    }

  tp_group_mixin_add_handle_owner ((GObject *) self, alice_local,
      alice_global);
  tp_group_mixin_add_handle_owner ((GObject *) self, bob_local,
      bob_global);
  tp_group_mixin_add_handle_owner ((GObject *) self, chris_local,
      chris_global);
  /* we know that anon_local is channel-specific, but not whose it is,
   * hence 0 */
  tp_group_mixin_add_handle_owner ((GObject *) self, anon_local, 0);

  /* everyone in! */
  added = tp_intset_new();
  tp_intset_add (added, alice_local);
  tp_intset_add (added, bob_local);
  tp_intset_add (added, chris_local);
  tp_intset_add (added, anon_local);
  tp_intset_add (added, mixin->self_handle);

  tp_group_mixin_change_members ((GObject *) self, "", added, NULL, NULL,
      NULL, 0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  tp_handle_unref (contact_repo, alice_local);
  tp_handle_unref (contact_repo, bob_local);
  tp_handle_unref (contact_repo, chris_local);
  tp_handle_unref (contact_repo, anon_local);

  tp_handle_unref (contact_repo, alice_global);
  tp_handle_unref (contact_repo, bob_global);
  tp_handle_unref (contact_repo, chris_global);

  /* now that the dust has settled, we can also invite people */
  tp_group_mixin_change_flags ((GObject *) self,
      TP_CHANNEL_GROUP_FLAG_CAN_ADD | TP_CHANNEL_GROUP_FLAG_MESSAGE_ADD,
      0);
}

static void
accept_invitations (ExampleCSHRoomChannel *self)
{
  tp_group_mixin_change_members ((GObject *) self, "", self->priv->remote, NULL, NULL,
      self->priv->remote, 0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_clear(self->priv->remote);
}

static void
join_room (ExampleCSHRoomChannel *self)
{
  TpGroupMixin *mixin = TP_GROUP_MIXIN (self);
  GObject *object = (GObject *) self;
  TpIntSet *add_remote_pending;

  g_assert (!tp_handle_set_is_member (mixin->members, mixin->self_handle));
  g_assert (!tp_handle_set_is_member (mixin->remote_pending,
        mixin->self_handle));

  /* Indicate in the Group interface that a join is in progress */

  add_remote_pending = tp_intset_new ();
  tp_intset_add (add_remote_pending, mixin->self_handle);

  tp_group_mixin_add_handle_owner (object, mixin->self_handle,
      self->priv->conn->self_handle);
  tp_group_mixin_change_members (object, "", NULL, NULL, NULL,
      add_remote_pending, self->priv->conn->self_handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  tp_intset_destroy (add_remote_pending);

  /* Actually join the room. In a real implementation this would be a network
   * round-trip - we don't have a network, so pretend that joining takes
   * 500ms */
  g_timeout_add (500, (GSourceFunc) complete_join, self);
}


static GObject *
constructor (GType type,
             guint n_props,
             GObjectConstructParam *props)
{
  GObject *object =
      G_OBJECT_CLASS (example_csh_room_channel_parent_class)->constructor (type,
          n_props, props);
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (object);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  TpHandleRepoIface *room_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_ROOM);
  DBusGConnection *bus;
  TpHandle self_handle;

  tp_handle_ref (room_repo, self->priv->handle);

  if (self->priv->initiator != 0)
    tp_handle_ref (contact_repo, self->priv->initiator);

  bus = tp_get_bus ();
  dbus_g_connection_register_g_object (bus, self->priv->object_path, object);

  tp_text_mixin_init (object, G_STRUCT_OFFSET (ExampleCSHRoomChannel, text),
      contact_repo);

  tp_text_mixin_set_message_types (object,
      TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL,
      TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION,
      G_MAXUINT);

  /* We start off remote-pending (if this CM supported other people inviting
   * us, we'd start off local-pending in that case instead - but it doesn't),
   * with this self-handle. */
  self_handle = suggest_room_identity (self);

  tp_group_mixin_init (object,
      G_STRUCT_OFFSET (ExampleCSHRoomChannel, group),
      contact_repo, self_handle);

  /* Initially, we can't do anything. */
  tp_group_mixin_change_flags (object,
      TP_CHANNEL_GROUP_FLAG_CHANNEL_SPECIFIC_HANDLES |
      TP_CHANNEL_GROUP_FLAG_PROPERTIES,
      0);

  self->priv->remote = tp_intset_new ();

  /* Immediately attempt to join the group */
  join_room (self);

  return object;
}


static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;
    case PROP_CHANNEL_TYPE:
      g_value_set_static_string (value, TP_IFACE_CHANNEL_TYPE_TEXT);
      break;
    case PROP_HANDLE_TYPE:
      g_value_set_uint (value, TP_HANDLE_TYPE_ROOM);
      break;
    case PROP_HANDLE:
      g_value_set_uint (value, self->priv->handle);
      break;
    case PROP_TARGET_ID:
        {
          TpHandleRepoIface *room_repo = tp_base_connection_get_handles (
              self->priv->conn, TP_HANDLE_TYPE_ROOM);

          g_value_set_string (value,
              tp_handle_inspect (room_repo, self->priv->handle));
        }
      break;
    case PROP_REQUESTED:
      /* this example CM doesn't yet support being invited into a chatroom,
       * so the only way a channel can exist is if the user asked for it */
      g_value_set_boolean (value, TRUE);
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
      g_value_set_boxed (value, example_csh_room_channel_interfaces);
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
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_free (self->priv->object_path);
      self->priv->object_path = g_value_dup_string (value);
      break;
    case PROP_HANDLE:
      /* we don't ref it here because we don't necessarily have access to the
       * room repo yet - instead we ref it in the constructor.
       */
      self->priv->handle = g_value_get_uint (value);
      break;
    case PROP_INITIATOR_HANDLE:
      /* similarly, we don't yet have the contact repo */
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
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (object);

  if (self->priv->disposed)
    return;

  self->priv->disposed = TRUE;

  if (self->priv->accept_invitations_timeout)
    g_source_remove (self->priv->accept_invitations_timeout);

  tp_intset_destroy (self->priv->remote);

  if (!self->priv->closed)
    {
      self->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }

  ((GObjectClass *) example_csh_room_channel_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (object);
  TpHandleRepoIface *contact_handles = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  TpHandleRepoIface *room_handles = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_ROOM);

  if (self->priv->initiator != 0)
    tp_handle_unref (contact_handles, self->priv->initiator);

  tp_handle_unref (room_handles, self->priv->handle);
  g_free (self->priv->object_path);

  tp_text_mixin_finalize (object);

  ((GObjectClass *) example_csh_room_channel_parent_class)->finalize (object);
}

static gboolean
add_member (GObject *object,
            TpHandle handle,
            const gchar *message,
            GError **error)
{
  /* In a real implementation, if handle was mixin->self_handle we'd accept
   * an invitation here; otherwise we'd invite the given contact. */
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (object);

  /* we know that anon_local is channel-specific, but not whose it is,
   * hence 0 */
  tp_group_mixin_add_handle_owner (object, handle, 0);

  /* everyone in! */
  tp_intset_add (self->priv->remote, handle);

  tp_group_mixin_change_members (object, message, NULL, NULL, NULL,
      self->priv->remote, 0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  // accept invitation after 500ms
  self->priv->accept_invitations_timeout =
      g_timeout_add (500, (GSourceFunc) accept_invitations, self);

  return TRUE;
}

static void
example_csh_room_channel_class_init (ExampleCSHRoomChannelClass *klass)
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

  g_type_class_add_private (klass, sizeof (ExampleCSHRoomChannelPrivate));

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

  param_spec = g_param_spec_string ("target-id", "Chatroom's ID",
      "The string obtained by inspecting the MUC's handle",
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
      G_STRUCT_OFFSET (ExampleCSHRoomChannelClass, text_class));

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCSHRoomChannelClass, dbus_properties_class));

  tp_group_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCSHRoomChannelClass, group_class),
      add_member,
      NULL);
  tp_group_mixin_init_dbus_properties (object_class);
}


static void
channel_close (TpSvcChannel *iface,
               DBusGMethodInvocation *context)
{
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (iface);

  if (!self->priv->closed)
    {
      self->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }

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
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (iface);

  tp_svc_channel_return_from_get_handle (context, TP_HANDLE_TYPE_ROOM,
      self->priv->handle);
}


static void
channel_get_interfaces (TpSvcChannel *iface,
                        DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_interfaces (context,
      example_csh_room_channel_interfaces);
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
  ExampleCSHRoomChannel *self = EXAMPLE_CSH_ROOM_CHANNEL (iface);
  time_t timestamp = time (NULL);

  /* The /dev/null of text channels - we claim to have sent the message,
   * but nothing more happens */
  tp_svc_channel_type_text_emit_sent ((GObject *) self, timestamp, type, text);
  tp_svc_channel_type_text_return_from_send (context);
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
