/*
 * conference-channel.c - an tp_tests conference channel
 *
 * Copyright © 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2010 Nokia Corporation
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

#include "chan.h"

#include <string.h>

#include <gobject/gvaluecollector.h>

#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/svc-channel.h>
#include <telepathy-glib/svc-properties-interface.h>

#include "extensions/extensions.h"

/* TODO:
 * Simulate Conference.ChannelRemoved
 */

static void mergeable_conference_iface_init (gpointer iface, gpointer data);
static void channel_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (TpTestsConferenceChannel,
    tp_tests_conference_channel,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_GROUP,
      tp_group_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_CONFERENCE,
      NULL);
    G_IMPLEMENT_INTERFACE (FUTURE_TYPE_SVC_CHANNEL_INTERFACE_MERGEABLE_CONFERENCE,
      mergeable_conference_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_IFACE, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_EXPORTABLE_CHANNEL, NULL))

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
  PROP_CONFERENCE_CHANNELS,
  PROP_CONFERENCE_INITIAL_CHANNELS,
  PROP_CONFERENCE_INITIAL_INVITEE_HANDLES,
  PROP_CONFERENCE_INITIAL_INVITEE_IDS,
  PROP_CONFERENCE_INVITATION_MESSAGE,
  PROP_CONFERENCE_ORIGINAL_CHANNELS,
  N_PROPS
};

struct _TpTestsConferenceChannelPrivate
{
  TpBaseConnection *conn;
  gchar *object_path;
  guint handle_type;

  GPtrArray *conference_initial_channels;
  GPtrArray *conference_channels;
  GArray *conference_initial_invitee_handles;
  gchar **conference_initial_invitee_ids;
  gchar *conference_invitation_message;
  GHashTable *conference_original_channels;

  gboolean disposed;
  gboolean closed;
};

static const gchar * tp_tests_conference_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_GROUP,
    TP_IFACE_CHANNEL_INTERFACE_CONFERENCE,
    FUTURE_IFACE_CHANNEL_INTERFACE_MERGEABLE_CONFERENCE,
    NULL
};

static void
tp_tests_conference_channel_init (TpTestsConferenceChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TP_TESTS_TYPE_CONFERENCE_CHANNEL,
      TpTestsConferenceChannelPrivate);

  self->priv->handle_type = (guint) -1;
}

static void
constructed (GObject *object)
{
  void (*chain_up) (GObject *) =
      ((GObjectClass *) tp_tests_conference_channel_parent_class)->constructed;
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (object);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  TpDBusDaemon *bus;

  if (chain_up != NULL)
    {
      chain_up (object);
    }

  bus = tp_dbus_daemon_dup (NULL);
  tp_dbus_daemon_register_object (bus, self->priv->object_path, object);

  tp_group_mixin_init (object,
      G_STRUCT_OFFSET (TpTestsConferenceChannel, group),
      contact_repo, self->priv->conn->self_handle);

  if (self->priv->handle_type == (guint) -1)
    {
      self->priv->handle_type = TP_HANDLE_TYPE_NONE;
    }
  if (!self->priv->conference_channels)
    {
      self->priv->conference_channels = g_ptr_array_new ();
    }
  if (!self->priv->conference_initial_channels)
    {
      self->priv->conference_initial_channels = g_ptr_array_new ();
    }
  if (!self->priv->conference_initial_invitee_handles)
    {
      self->priv->conference_initial_invitee_handles =
          g_array_new (FALSE, TRUE, sizeof (guint));
    }
  if (!self->priv->conference_initial_invitee_ids)
    {
      self->priv->conference_initial_invitee_ids = g_new0 (gchar *, 1);
    }
  if (!self->priv->conference_invitation_message)
    {
      self->priv->conference_invitation_message = g_strdup ("");
    }
  if (!self->priv->conference_original_channels)
    {
      self->priv->conference_original_channels =  dbus_g_type_specialized_construct (
          TP_HASH_TYPE_CHANNEL_ORIGINATOR_MAP);
    }
}

static void
get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;

    case PROP_CHANNEL_TYPE:
      g_value_set_static_string (value, TP_IFACE_CHANNEL);
      break;

    case PROP_HANDLE_TYPE:
      g_value_set_uint (value, self->priv->handle_type);
      break;

    case PROP_HANDLE:
      g_value_set_uint (value, 0);
      break;

    case PROP_TARGET_ID:
      g_value_set_string (value, "");
      break;

    case PROP_REQUESTED:
      g_value_set_boolean (value, TRUE);
      break;

    case PROP_INITIATOR_HANDLE:
      g_value_set_uint (value, 0);
      break;

    case PROP_INITIATOR_ID:
      g_value_set_string (value, "");
      break;

    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->conn);
      break;

    case PROP_INTERFACES:
      g_value_set_boxed (value, tp_tests_conference_channel_interfaces);
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
              TP_IFACE_CHANNEL_INTERFACE_CONFERENCE, "Channels",
              TP_IFACE_CHANNEL_INTERFACE_CONFERENCE, "InitialChannels",
              TP_IFACE_CHANNEL_INTERFACE_CONFERENCE, "InitialInviteeHandles",
              TP_IFACE_CHANNEL_INTERFACE_CONFERENCE, "InitialInviteeIDs",
              TP_IFACE_CHANNEL_INTERFACE_CONFERENCE, "InvitationMessage",
              TP_IFACE_CHANNEL_INTERFACE_CONFERENCE, "OriginalChannels",
              NULL));
      break;

    case PROP_CONFERENCE_CHANNELS:
      g_value_set_boxed (value, self->priv->conference_channels);
      g_assert (G_VALUE_HOLDS (value, TP_ARRAY_TYPE_OBJECT_PATH_LIST));
      break;

    case PROP_CONFERENCE_INITIAL_CHANNELS:
      g_value_set_boxed (value, self->priv->conference_initial_channels);
      g_assert (G_VALUE_HOLDS (value, TP_ARRAY_TYPE_OBJECT_PATH_LIST));
      break;

    case PROP_CONFERENCE_INITIAL_INVITEE_HANDLES:
      g_value_set_boxed (value, self->priv->conference_initial_invitee_handles);
      g_assert (G_VALUE_HOLDS (value, DBUS_TYPE_G_UINT_ARRAY));
      break;

    case PROP_CONFERENCE_INITIAL_INVITEE_IDS:
      g_value_set_boxed (value, self->priv->conference_initial_invitee_ids);
      g_assert (G_VALUE_HOLDS (value, G_TYPE_STRV));
      break;

    case PROP_CONFERENCE_INVITATION_MESSAGE:
      g_value_set_string (value, self->priv->conference_invitation_message);
      g_assert (G_VALUE_HOLDS (value, G_TYPE_STRING));
      break;

    case PROP_CONFERENCE_ORIGINAL_CHANNELS:
      g_value_set_boxed (value, self->priv->conference_original_channels);
      g_assert (G_VALUE_HOLDS (value, TP_HASH_TYPE_CHANNEL_ORIGINATOR_MAP));
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
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      self->priv->object_path = g_value_dup_string (value);
      break;

    case PROP_HANDLE_TYPE:
      self->priv->handle_type = g_value_get_uint (value);
      break;

    case PROP_CONNECTION:
      self->priv->conn = g_value_get_object (value);
      break;

    case PROP_CONFERENCE_INITIAL_CHANNELS:
      g_ptr_array_free(self->priv->conference_initial_channels, TRUE);
      self->priv->conference_initial_channels = g_value_dup_boxed (value);

      g_ptr_array_free(self->priv->conference_channels, TRUE);
      self->priv->conference_channels = g_value_dup_boxed (value);
      break;

    case PROP_CONFERENCE_INITIAL_INVITEE_HANDLES:
      self->priv->conference_initial_invitee_handles = g_value_dup_boxed (value);
      break;

    case PROP_CONFERENCE_INITIAL_INVITEE_IDS:
      self->priv->conference_initial_invitee_ids = g_value_dup_boxed (value);
      break;

    case PROP_CONFERENCE_INVITATION_MESSAGE:
      self->priv->conference_invitation_message = g_value_dup_string (value);
      break;

    case PROP_CHANNEL_TYPE:
    case PROP_HANDLE:
    case PROP_TARGET_ID:
    case PROP_REQUESTED:
    case PROP_INITIATOR_HANDLE:
    case PROP_INITIATOR_ID:
    case PROP_CONFERENCE_CHANNELS:
      /* these properties are not actually meaningfully changeable on this
       * channel, so we do nothing */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
dispose (GObject *object)
{
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (object);

  if (self->priv->disposed)
    {
      return;
    }

  self->priv->disposed = TRUE;

  g_ptr_array_free (self->priv->conference_channels, TRUE);
  self->priv->conference_channels = NULL;
  g_ptr_array_free (self->priv->conference_initial_channels, TRUE);
  self->priv->conference_initial_channels = NULL;
  if (self->priv->conference_initial_invitee_handles)
    {
      g_array_free (self->priv->conference_initial_invitee_handles, FALSE);
    }
  self->priv->conference_initial_invitee_handles = NULL;
  g_strfreev (self->priv->conference_initial_invitee_ids);
  self->priv->conference_initial_invitee_ids = NULL;
  g_free (self->priv->conference_invitation_message);
  self->priv->conference_invitation_message = NULL;
  g_boxed_free (TP_HASH_TYPE_CHANNEL_ORIGINATOR_MAP, self->priv->conference_original_channels);
  self->priv->conference_original_channels = NULL;

  if (!self->priv->closed)
    {
      self->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }

  ((GObjectClass *) tp_tests_conference_channel_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (object);

  g_free (self->priv->object_path);

  tp_group_mixin_finalize (object);

  ((GObjectClass *) tp_tests_conference_channel_parent_class)->finalize (object);
}

static gboolean
add_member (GObject *obj,
            TpHandle handle,
            const gchar *message,
            GError **error)
{
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (obj);
  TpIntSet *add = tp_intset_new ();

  tp_intset_add (add, handle);
  tp_group_mixin_change_members (obj, message, add, NULL, NULL, NULL,
      self->priv->conn->self_handle, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (add);

  return TRUE;
}

static void
tp_tests_conference_channel_class_init (TpTestsConferenceChannelClass *klass)
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
  static TpDBusPropertiesMixinPropImpl conference_props[] = {
      { "Channels", "channels", NULL },
      { "InitialChannels", "initial-channels", NULL },
      { "InitialInviteeHandles", "initial-invitee-handles", NULL },
      { "InitialInviteeIDs", "initial-invitee-ids", NULL },
      { "InvitationMessage", "invitation-message", NULL },
      { "OriginalChannels", "original-channels", NULL },
      { NULL }
  };
  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
      { TP_IFACE_CHANNEL,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        channel_props,
      },
      { TP_IFACE_CHANNEL_INTERFACE_CONFERENCE,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        conference_props,
      },
      { NULL }
  };
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  g_type_class_add_private (klass,
      sizeof (TpTestsConferenceChannelPrivate));

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
  g_object_class_override_property (object_class, PROP_HANDLE,
      "handle");
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
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_REQUESTED, param_spec);

  param_spec = g_param_spec_boxed ("channels", "Channel paths",
      "A list of the object paths of channels",
      TP_ARRAY_TYPE_OBJECT_PATH_LIST,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONFERENCE_CHANNELS,
      param_spec);

  param_spec = g_param_spec_boxed ("initial-channels", "Initial Channel paths",
      "A list of the object paths of initial channels",
      TP_ARRAY_TYPE_OBJECT_PATH_LIST,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONFERENCE_INITIAL_CHANNELS,
      param_spec);

  param_spec = g_param_spec_boxed ("initial-invitee-handles", "Initial Invitee Handles",
      "A list of additional contacts invited to this conference when it was created",
      DBUS_TYPE_G_UINT_ARRAY,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONFERENCE_INITIAL_INVITEE_HANDLES, param_spec);

  param_spec = g_param_spec_boxed ("initial-invitee-ids", "Initial Invitee IDs",
      "A list of additional contacts invited to this conference when it was created",
      G_TYPE_STRV,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONFERENCE_INITIAL_INVITEE_IDS, param_spec);

  param_spec = g_param_spec_string ("invitation-message", "Invitation message",
      "The message that was sent to the InitialInviteeHandles when they were invited",
      NULL,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONFERENCE_INVITATION_MESSAGE,
      param_spec);

  param_spec = g_param_spec_boxed ("original-channels",
      "Original Channels",
      "A map of channel specific handles to channels",
      TP_HASH_TYPE_CHANNEL_ORIGINATOR_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONFERENCE_ORIGINAL_CHANNELS,
      param_spec);

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (TpTestsConferenceChannelClass,
        dbus_properties_class));

  tp_group_mixin_class_init (object_class,
      G_STRUCT_OFFSET (TpTestsConferenceChannelClass, group_class),
      add_member,
      NULL);
  tp_group_mixin_init_dbus_properties (object_class);
}

static void
channel_close (TpSvcChannel *iface,
    DBusGMethodInvocation *context)
{
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (iface);

  if (!self->priv->closed)
    {
      self->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }

  tp_svc_channel_return_from_close (context);
}

static void
channel_get_channel_type (TpSvcChannel *iface G_GNUC_UNUSED,
    DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_channel_type (context,
      TP_IFACE_CHANNEL);
}

static void
channel_get_handle (TpSvcChannel *iface,
    DBusGMethodInvocation *context)
{
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (iface);

  tp_svc_channel_return_from_get_handle (context, self->priv->handle_type, 0);
}

static void
channel_get_interfaces (TpSvcChannel *iface G_GNUC_UNUSED,
    DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_interfaces (context,
      tp_tests_conference_channel_interfaces);
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
mergeable_conference_merge (FutureSvcChannelInterfaceMergeableConference *iface G_GNUC_UNUSED,
                            const gchar *channel,
                            DBusGMethodInvocation *context)
{
  TpTestsConferenceChannel *self = TP_TESTS_CONFERENCE_CHANNEL (iface);
  GHashTable *immutable_props = g_hash_table_new (NULL, NULL);

  g_ptr_array_add (self->priv->conference_channels, g_strdup (channel));

  tp_svc_channel_interface_conference_emit_channel_merged (self, channel, 0, immutable_props);

  g_hash_table_destroy (immutable_props);

  future_svc_channel_interface_mergeable_conference_return_from_merge (context);
}

static void
mergeable_conference_iface_init (gpointer iface,
                                 gpointer data)
{
  FutureSvcChannelInterfaceMergeableConferenceClass *klass = iface;

#define IMPLEMENT(x) future_svc_channel_interface_mergeable_conference_implement_##x (klass, mergeable_conference_##x)
  IMPLEMENT (merge);
#undef IMPLEMENT
}

void tp_tests_conference_channel_remove_channel (TpTestsConferenceChannel *self,
        const gchar *channel)
{
  guint i;

  for (i = 0; i < self->priv->conference_channels->len; i++)
    {
      gchar *path = g_ptr_array_index (self->priv->conference_channels, i);

      if (strcmp (path, channel) == 0)
        {
          GHashTable *details = g_hash_table_new_full (g_str_hash, g_str_equal,
                  NULL, (GDestroyNotify) tp_g_value_slice_free);

          g_ptr_array_remove (self->priv->conference_channels, (gpointer) path);
          g_free (path);

          g_hash_table_insert (details, "actor",
              tp_g_value_slice_new_uint (self->priv->conn->self_handle));
          g_hash_table_insert (details, "domain-specific-detail-uint", tp_g_value_slice_new_uint (3));

          tp_svc_channel_interface_conference_emit_channel_removed (self, channel, details);

          g_hash_table_destroy (details);
        }
    }
}
