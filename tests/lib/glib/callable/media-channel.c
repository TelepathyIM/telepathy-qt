/*
 * media-channel.c - an example 1-1 streamed media call.
 *
 * For simplicity, this channel emulates a device with its own
 * audio/video user interface, like a video-equipped form of the phones
 * manipulated by telepathy-snom or gnome-phone-manager.
 *
 * As a result, this channel does not have the MediaSignalling interface, and
 * clients should not attempt to do their own streaming using
 * telepathy-farsight, telepathy-stream-engine or maemo-stream-engine.
 *
 * In practice, nearly all connection managers also have the MediaSignalling
 * interface on their streamed media channels. Usage for those CMs is the
 * same, except that whichever client is the primary handler for the channel
 * should also hand the channel over to telepathy-farsight or
 * telepathy-stream-engine to implement the actual streaming.
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
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

#include "media-channel.h"

#include "media-stream.h"

#include <string.h>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/svc-channel.h>
#include <telepathy-glib/svc-generic.h>

static void media_iface_init (gpointer iface, gpointer data);
static void channel_iface_init (gpointer iface, gpointer data);
static void hold_iface_init (gpointer iface, gpointer data);
static void dtmf_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (ExampleCallableMediaChannel,
    example_callable_media_channel,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_STREAMED_MEDIA,
      media_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_GROUP,
      tp_group_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_HOLD,
      hold_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_DTMF,
      dtmf_iface_init);
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
  PROP_SIMULATION_DELAY,
  PROP_INITIAL_AUDIO,
  PROP_INITIAL_VIDEO,
  N_PROPS
};

enum
{
  SIGNAL_CALL_TERMINATED,
  N_SIGNALS
};

typedef enum {
    PROGRESS_NONE,
    PROGRESS_CALLING,
    PROGRESS_ACTIVE,
    PROGRESS_ENDED
} ExampleCallableCallProgress;

static guint signals[N_SIGNALS] = { 0 };

struct _ExampleCallableMediaChannelPrivate
{
  TpBaseConnection *conn;
  gchar *object_path;
  TpHandle handle;
  TpHandle initiator;
  ExampleCallableCallProgress progress;

  guint simulation_delay;

  guint next_stream_id;

  GHashTable *streams;

  guint hold_state;
  guint hold_state_reason;

  gboolean locally_requested;
  gboolean initial_audio;
  gboolean initial_video;
  gboolean disposed;
};

static const char * example_callable_media_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_GROUP,
    TP_IFACE_CHANNEL_INTERFACE_HOLD,
    TP_IFACE_CHANNEL_INTERFACE_DTMF,
    NULL
};

static void
example_callable_media_channel_init (ExampleCallableMediaChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL,
      ExampleCallableMediaChannelPrivate);

  self->priv->next_stream_id = 1;
  self->priv->streams = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, g_object_unref);

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_UNHELD;
  self->priv->hold_state_reason = TP_LOCAL_HOLD_STATE_REASON_NONE;
}

static ExampleCallableMediaStream *example_callable_media_channel_add_stream (
    ExampleCallableMediaChannel *self, TpMediaStreamType media_type,
    gboolean locally_requested);

static void
constructed (GObject *object)
{
  void (*chain_up) (GObject *) =
      ((GObjectClass *) example_callable_media_channel_parent_class)->constructed;
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (object);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  TpIntSet *members;
  TpIntSet *local_pending;

  if (chain_up != NULL)
    chain_up (object);

  tp_dbus_daemon_register_object (
      tp_base_connection_get_dbus_daemon (self->priv->conn),
      self->priv->object_path, self);

  tp_group_mixin_init (object,
      G_STRUCT_OFFSET (ExampleCallableMediaChannel, group),
      contact_repo, self->priv->conn->self_handle);

  /* Initially, the channel contains the initiator as a member; they are also
   * the actor for the change that adds any initial members. */

  members = tp_intset_new_containing (self->priv->initiator);

  if (self->priv->locally_requested)
    {
      /* Nobody is locally pending. The remote peer will turn up in
       * remote-pending state when we actually contact them, which is done
       * in RequestStreams */
      self->priv->progress = PROGRESS_NONE;
      local_pending = NULL;
    }
  else
    {
      /* This is an incoming call, so the self-handle is locally
       * pending, to indicate that we need to answer. */
      self->priv->progress = PROGRESS_CALLING;
      local_pending = tp_intset_new_containing (self->priv->conn->self_handle);
    }

  tp_group_mixin_change_members (object, "",
      members /* added */,
      NULL /* nobody removed */,
      local_pending, /* added to local-pending */
      NULL /* nobody added to remote-pending */,
      self->priv->initiator /* actor */, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (members);

  if (local_pending != NULL)
    tp_intset_destroy (local_pending);

  /* We don't need to allow adding or removing members to this Group in ways
   * that need flags set, so the only flag we set is to say we support the
   * Properties interface to the Group.
   *
   * It doesn't make sense to add anyone to the Group, since we already know
   * who we're going to call (or were called by). The only call to AddMembers
   * we need to support is to move ourselves from local-pending to member in
   * the incoming call case, and that's always allowed anyway.
   *
   * (Connection managers that support the various backwards-compatible
   * ways to make an outgoing StreamedMedia channel have to support adding the
   * peer to remote-pending, but that has no actual effect other than to
   * obscure what's going on; in this one, there's no need to support that
   * usage.)
   *
   * Similarly, it doesn't make sense to remove anyone from this Group apart
   * from ourselves (to hang up), and removing the SelfHandle is always
   * allowed anyway.
   */
  tp_group_mixin_change_flags (object, TP_CHANNEL_GROUP_FLAG_PROPERTIES, 0);

  /* Future versions of telepathy-spec will allow a channel request to
   * say "initially include an audio stream" and/or "initially include a video
   * stream", which would be represented like this; we don't support this
   * usage yet, though, so ExampleCallableMediaManager will never invoke
   * our constructor in this way. */
  g_assert (!(self->priv->locally_requested && self->priv->initial_audio));
  g_assert (!(self->priv->locally_requested && self->priv->initial_video));

  if (!self->priv->locally_requested)
    {
      /* the caller has almost certainly asked us for some streams - there's
       * not much point in having a call otherwise */

      if (self->priv->initial_audio)
        {
          g_message ("Channel initially has an audio stream");
          example_callable_media_channel_add_stream (self,
              TP_MEDIA_STREAM_TYPE_AUDIO, FALSE);
        }

      if (self->priv->initial_video)
        {
          g_message ("Channel initially has a video stream");
          example_callable_media_channel_add_stream (self,
              TP_MEDIA_STREAM_TYPE_VIDEO, FALSE);
        }
    }
}

static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;

    case PROP_CHANNEL_TYPE:
      g_value_set_static_string (value, TP_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
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
      g_value_set_boolean (value, self->priv->locally_requested);
      break;

    case PROP_INITIATOR_HANDLE:
      g_value_set_uint (value, self->priv->initiator);
      break;

    case PROP_INITIATOR_ID:
        {
          TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
              self->priv->conn, TP_HANDLE_TYPE_CONTACT);

          g_value_set_string (value,
              tp_handle_inspect (contact_repo, self->priv->initiator));
        }
      break;

    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->conn);
      break;

    case PROP_INTERFACES:
      g_value_set_boxed (value, example_callable_media_channel_interfaces);
      break;

    case PROP_CHANNEL_DESTROYED:
      g_value_set_boolean (value, (self->priv->progress == PROGRESS_ENDED));
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

    case PROP_SIMULATION_DELAY:
      g_value_set_uint (value, self->priv->simulation_delay);
      break;

    case PROP_INITIAL_AUDIO:
      g_value_set_boolean (value, self->priv->initial_audio);
      break;

    case PROP_INITIAL_VIDEO:
      g_value_set_boolean (value, self->priv->initial_video);
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
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_assert (self->priv->object_path == NULL);
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

    case PROP_REQUESTED:
      self->priv->locally_requested = g_value_get_boolean (value);
      break;

    case PROP_HANDLE_TYPE:
    case PROP_CHANNEL_TYPE:
      /* these properties are writable in the interface, but not actually
       * meaningfully changable on this channel, so we do nothing */
      break;

    case PROP_CONNECTION:
      self->priv->conn = g_value_get_object (value);
      break;

    case PROP_SIMULATION_DELAY:
      self->priv->simulation_delay = g_value_get_uint (value);
      break;

    case PROP_INITIAL_AUDIO:
      self->priv->initial_audio = g_value_get_boolean (value);
      break;

    case PROP_INITIAL_VIDEO:
      self->priv->initial_video = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
example_callable_media_channel_close (ExampleCallableMediaChannel *self,
                                      TpHandle actor,
                                      TpChannelGroupChangeReason reason)
{
  if (self->priv->progress != PROGRESS_ENDED)
    {
      TpIntSet *everyone;

      self->priv->progress = PROGRESS_ENDED;

      if (actor == self->group.self_handle)
        {
          const gchar *send_reason;

          /* In a real protocol these would be some sort of real protocol
           * construct, like an XMPP error stanza or a SIP error code */
          switch (reason)
            {
            case TP_CHANNEL_GROUP_CHANGE_REASON_BUSY:
              send_reason = "<user-is-busy/>";
              break;

            case TP_CHANNEL_GROUP_CHANGE_REASON_NO_ANSWER:
              send_reason = "<no-answer/>";
              break;

            default:
              send_reason = "<call-terminated/>";
            }

          g_message ("SIGNALLING: send: Terminating call: %s", send_reason);
        }

      everyone = tp_intset_new_containing (self->priv->handle);
      tp_intset_add (everyone, self->group.self_handle);
      tp_group_mixin_change_members ((GObject *) self, "",
          NULL /* nobody added */,
          everyone /* removed */,
          NULL /* nobody locally pending */,
          NULL /* nobody remotely pending */,
          actor,
          reason);
      tp_intset_destroy (everyone);

      g_signal_emit (self, signals[SIGNAL_CALL_TERMINATED], 0);
      tp_svc_channel_emit_closed (self);
    }
}

static void
dispose (GObject *object)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (object);

  if (self->priv->disposed)
    return;

  self->priv->disposed = TRUE;

  g_hash_table_destroy (self->priv->streams);
  self->priv->streams = NULL;

  example_callable_media_channel_close (self, self->group.self_handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  ((GObjectClass *) example_callable_media_channel_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (object);

  g_free (self->priv->object_path);

  tp_group_mixin_finalize (object);

  ((GObjectClass *) example_callable_media_channel_parent_class)->finalize (object);
}

static gboolean
add_member (GObject *object,
            TpHandle member,
            const gchar *message,
            GError **error)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (object);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);

  /* In connection managers that supported the RequestChannel method for
   * streamed media channels, it would be necessary to support adding the
   * called contact to the members of an outgoing call. However, in this
   * legacy-free example, we don't support that usage, so the only use for
   * AddMembers is to accept an incoming call.
   */

  if (member == self->group.self_handle &&
      tp_handle_set_is_member (self->group.local_pending, member))
    {
      /* We're in local-pending, move to members to accept. */
      TpIntSet *set = tp_intset_new_containing (member);
      GHashTableIter iter;
      gpointer v;

      g_assert (self->priv->progress == PROGRESS_CALLING);

      g_message ("SIGNALLING: send: Accepting incoming call from %s",
          tp_handle_inspect (contact_repo, self->priv->handle));

      self->priv->progress = PROGRESS_ACTIVE;

      tp_group_mixin_change_members (object, "",
          set /* added */,
          NULL /* nobody removed */,
          NULL /* nobody added to local pending */,
          NULL /* nobody added to remote pending */,
          member /* actor */, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

      tp_intset_destroy (set);

      g_hash_table_iter_init (&iter, self->priv->streams);

      while (g_hash_table_iter_next (&iter, NULL, &v))
        {
          /* we accept the proposed stream direction... */
          example_callable_media_stream_accept_proposed_direction (v);
          /* ... and the stream tries to connect */
          example_callable_media_stream_connect (v);
        }

      return TRUE;
    }

  /* Otherwise it's a meaningless request, so reject it. */
  g_set_error (error, TP_ERROR, TP_ERROR_NOT_AVAILABLE,
      "Cannot add handle %u to channel", member);
  return FALSE;
}

static gboolean
remove_member_with_reason (GObject *object,
                           TpHandle member,
                           const gchar *message,
                           guint reason,
                           GError **error)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (object);

  /* The TpGroupMixin won't call this unless removing the member is allowed
   * by the group flags, which in this case means it must be our own handle
   * (because the other user never appears in local-pending).
   */

  g_assert (member == self->group.self_handle);

  example_callable_media_channel_close (self, self->group.self_handle, reason);
  return TRUE;
}

static void
example_callable_media_channel_class_init (ExampleCallableMediaChannelClass *klass)
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

  g_type_class_add_private (klass,
      sizeof (ExampleCallableMediaChannelPrivate));

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

  param_spec = g_param_spec_uint ("simulation-delay", "Simulation delay",
      "Delay between simulated network events",
      0, G_MAXUINT32, 1000,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SIMULATION_DELAY,
      param_spec);

  param_spec = g_param_spec_boolean ("initial-audio", "Initial audio?",
      "True if this channel had an audio stream when first announced",
      FALSE,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INITIAL_AUDIO,
      param_spec);

  param_spec = g_param_spec_boolean ("initial-video", "Initial video?",
      "True if this channel had a video stream when first announced",
      FALSE,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INITIAL_VIDEO,
      param_spec);

  signals[SIGNAL_CALL_TERMINATED] = g_signal_new ("call-terminated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCallableMediaChannelClass,
        dbus_properties_class));

  tp_group_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCallableMediaChannelClass, group_class),
      add_member,
      NULL);
  tp_group_mixin_class_allow_self_removal (object_class);
  tp_group_mixin_class_set_remove_with_reason_func (object_class,
      remove_member_with_reason);
  tp_group_mixin_init_dbus_properties (object_class);
}

static void
channel_close (TpSvcChannel *iface,
               DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);

  example_callable_media_channel_close (self, self->group.self_handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_svc_channel_return_from_close (context);
}

static void
channel_get_channel_type (TpSvcChannel *iface G_GNUC_UNUSED,
                          DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_channel_type (context,
      TP_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
}

static void
channel_get_handle (TpSvcChannel *iface,
                    DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);

  tp_svc_channel_return_from_get_handle (context, TP_HANDLE_TYPE_CONTACT,
      self->priv->handle);
}

static void
channel_get_interfaces (TpSvcChannel *iface G_GNUC_UNUSED,
                        DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_interfaces (context,
      example_callable_media_channel_interfaces);
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
media_list_streams (TpSvcChannelTypeStreamedMedia *iface,
                    DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);
  GPtrArray *array = g_ptr_array_sized_new (g_hash_table_size (
        self->priv->streams));
  GHashTableIter iter;
  gpointer v;

  g_hash_table_iter_init (&iter, self->priv->streams);

  while (g_hash_table_iter_next (&iter, NULL, &v))
    {
      ExampleCallableMediaStream *stream = v;
      GValueArray *va;

      g_object_get (stream,
          "stream-info", &va,
          NULL);

      g_ptr_array_add (array, va);
    }

  tp_svc_channel_type_streamed_media_return_from_list_streams (context,
      array);
  g_ptr_array_foreach (array, (GFunc) g_value_array_free, NULL);
  g_ptr_array_free (array, TRUE);
}

static void
media_remove_streams (TpSvcChannelTypeStreamedMedia *iface,
                      const GArray *stream_ids,
                      DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);
  guint i;

  for (i = 0; i < stream_ids->len; i++)
    {
      guint id = g_array_index (stream_ids, guint, i);

      if (g_hash_table_lookup (self->priv->streams,
            GUINT_TO_POINTER (id)) == NULL)
        {
          GError *error = g_error_new (TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
              "No stream with ID %u in this channel", id);

          dbus_g_method_return_error (context, error);
          g_error_free (error);
          return;
        }
    }

  for (i = 0; i < stream_ids->len; i++)
    {
      guint id = g_array_index (stream_ids, guint, i);

      example_callable_media_stream_close (
          g_hash_table_lookup (self->priv->streams, GUINT_TO_POINTER (id)));
    }

  tp_svc_channel_type_streamed_media_return_from_remove_streams (context);
}

static void
media_request_stream_direction (TpSvcChannelTypeStreamedMedia *iface,
                                guint stream_id,
                                guint stream_direction,
                                DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);
  ExampleCallableMediaStream *stream = g_hash_table_lookup (
      self->priv->streams, GUINT_TO_POINTER (stream_id));
  GError *error = NULL;

  if (stream == NULL)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "No stream with ID %u in this channel", stream_id);
      goto error;
    }

  if (stream_direction > TP_MEDIA_STREAM_DIRECTION_BIDIRECTIONAL)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Stream direction %u is not valid", stream_direction);
      goto error;
    }

  /* In some protocols, streams cannot be neither sending nor receiving, so
   * if a stream is set to TP_MEDIA_STREAM_DIRECTION_NONE, this is equivalent
   * to removing it with RemoveStreams. (This is true in XMPP, for instance.)
   *
   * If this was the case, there would be code like this here:
   *
   * if (stream_direction == TP_MEDIA_STREAM_DIRECTION_NONE)
   *   {
   *     example_callable_media_stream_close (stream);
   *     tp_svc_channel_type_streamed_media_return_from_request_stream_direction (
   *        context);
   *     return;
   *   }
   *
   * However, for this example we'll emulate a protocol where streams can be
   * directionless.
   */

  if (!example_callable_media_stream_change_direction (stream,
        stream_direction, &error))
    goto error;

  tp_svc_channel_type_streamed_media_return_from_request_stream_direction (
      context);
  return;

error:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
stream_removed_cb (ExampleCallableMediaStream *stream,
                   ExampleCallableMediaChannel *self)
{
  guint id;

  g_object_get (stream,
      "id", &id,
      NULL);

  g_signal_handlers_disconnect_matched (stream, G_SIGNAL_MATCH_DATA,
      0, 0, NULL, NULL, self);
  g_hash_table_remove (self->priv->streams, GUINT_TO_POINTER (id));
  tp_svc_channel_type_streamed_media_emit_stream_removed (self, id);

  if (g_hash_table_size (self->priv->streams) == 0)
    {
      /* no streams left, so the call terminates */
      example_callable_media_channel_close (self, 0,
          TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
    }
}

static void
stream_direction_changed_cb (ExampleCallableMediaStream *stream,
                             ExampleCallableMediaChannel *self)
{
  guint id, direction, pending;

  g_object_get (stream,
      "id", &id,
      "direction", &direction,
      "pending-send", &pending,
      NULL);

  tp_svc_channel_type_streamed_media_emit_stream_direction_changed (self, id,
      direction, pending);
}

static void
stream_state_changed_cb (ExampleCallableMediaStream *stream,
                         GParamSpec *spec G_GNUC_UNUSED,
                         ExampleCallableMediaChannel *self)
{
  guint id, state;

  g_object_get (stream,
      "id", &id,
      "state", &state,
      NULL);

  tp_svc_channel_type_streamed_media_emit_stream_state_changed (self, id,
      state);
}

static gboolean
simulate_contact_ended_cb (gpointer p)
{
  ExampleCallableMediaChannel *self = p;

  /* if the call has been cancelled while we were waiting for the
   * contact to do so, do nothing! */
  if (self->priv->progress == PROGRESS_ENDED)
    return FALSE;

  g_message ("SIGNALLING: receive: call terminated: <call-terminated/>");

  example_callable_media_channel_close (self, self->priv->handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  return FALSE;
}

static gboolean
simulate_contact_answered_cb (gpointer p)
{
  ExampleCallableMediaChannel *self = p;
  TpIntSet *peer_set;
  GHashTableIter iter;
  gpointer v;
  TpHandleRepoIface *contact_repo;
  const gchar *peer;

  /* if the call has been cancelled while we were waiting for the
   * contact to answer, do nothing */
  if (self->priv->progress == PROGRESS_ENDED)
    return FALSE;

  /* otherwise, we're waiting for a response from the contact, which now
   * arrives */
  g_assert (self->priv->progress == PROGRESS_CALLING);

  g_message ("SIGNALLING: receive: contact answered our call");

  self->priv->progress = PROGRESS_ACTIVE;

  peer_set = tp_intset_new_containing (self->priv->handle);
  tp_group_mixin_change_members ((GObject *) self, "",
      peer_set /* added */,
      NULL /* nobody removed */,
      NULL /* nobody added to local-pending */,
      NULL /* nobody added to remote-pending */,
      self->priv->handle /* actor */,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (peer_set);

  g_hash_table_iter_init (&iter, self->priv->streams);

  while (g_hash_table_iter_next (&iter, NULL, &v))
    {
      /* remote contact accepts our proposed stream direction... */
      example_callable_media_stream_simulate_contact_agreed_to_send (v);
      /* ... and the stream tries to connect */
      example_callable_media_stream_connect (v);
    }

  contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  peer = tp_handle_inspect (contact_repo, self->priv->handle);

  /* If the contact's ID contains the magic string "(terminate)", simulate
   * them hanging up after a moment. */
  if (strstr (peer, "(terminate)") != NULL)
    {
      g_timeout_add_full (G_PRIORITY_DEFAULT,
          self->priv->simulation_delay,
          simulate_contact_ended_cb, g_object_ref (self),
          g_object_unref);
    }

  return FALSE;
}

static gboolean
simulate_contact_busy_cb (gpointer p)
{
  ExampleCallableMediaChannel *self = p;

  /* if the call has been cancelled while we were waiting for the
   * contact to answer, do nothing */
  if (self->priv->progress == PROGRESS_ENDED)
    return FALSE;

  /* otherwise, we're waiting for a response from the contact, which now
   * arrives */
  g_assert (self->priv->progress == PROGRESS_CALLING);

  g_message ("SIGNALLING: receive: call terminated: <user-is-busy/>");

  example_callable_media_channel_close (self, self->priv->handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_BUSY);

  return FALSE;
}

static ExampleCallableMediaStream *
example_callable_media_channel_add_stream (ExampleCallableMediaChannel *self,
                                           TpMediaStreamType media_type,
                                           gboolean locally_requested)
{
  ExampleCallableMediaStream *stream;
  guint id = self->priv->next_stream_id++;
  guint state, direction, pending_send;

  if (locally_requested)
    {
      g_message ("SIGNALLING: send: new %s stream",
          media_type == TP_MEDIA_STREAM_TYPE_AUDIO ? "audio" : "video");
    }

  stream = g_object_new (EXAMPLE_TYPE_CALLABLE_MEDIA_STREAM,
      "channel", self,
      "id", id,
      "handle", self->priv->handle,
      "type", media_type,
      "locally-requested", locally_requested,
      "simulation-delay", self->priv->simulation_delay,
      NULL);

  g_hash_table_insert (self->priv->streams, GUINT_TO_POINTER (id), stream);

  tp_svc_channel_type_streamed_media_emit_stream_added (self, id,
      self->priv->handle, media_type);

  g_object_get (stream,
      "state", &state,
      "direction", &direction,
      "pending-send", &pending_send,
      NULL);

  /* this is the "implicit" initial state mandated by telepathy-spec */
  if (state != TP_MEDIA_STREAM_STATE_DISCONNECTED)
    {
      tp_svc_channel_type_streamed_media_emit_stream_state_changed (self, id,
          state);
    }

  /* this is the "implicit" initial direction mandated by telepathy-spec */
  if (direction != TP_MEDIA_STREAM_DIRECTION_RECEIVE ||
      pending_send != TP_MEDIA_STREAM_PENDING_LOCAL_SEND)
    {
      tp_svc_channel_type_streamed_media_emit_stream_direction_changed (self,
          id, direction, pending_send);
    }

  g_signal_connect (stream, "removed", G_CALLBACK (stream_removed_cb),
      self);
  g_signal_connect (stream, "notify::state",
      G_CALLBACK (stream_state_changed_cb), self);
  g_signal_connect (stream, "direction-changed",
      G_CALLBACK (stream_direction_changed_cb), self);

  if (self->priv->progress == PROGRESS_ACTIVE)
    {
      example_callable_media_stream_connect (stream);
    }

  return stream;
}

static void
media_request_streams (TpSvcChannelTypeStreamedMedia *iface,
                       guint contact_handle,
                       const GArray *media_types,
                       DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  GPtrArray *array;
  guint i;
  GError *error = NULL;

  if (!tp_handle_is_valid (contact_repo, contact_handle, &error))
    goto error;

  if (contact_handle != self->priv->handle)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "This channel is for handle #%u, we can't make a stream to #%u",
          self->priv->handle, contact_handle);
      goto error;
    }

  if (self->priv->progress == PROGRESS_ENDED)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_NOT_AVAILABLE,
          "Call has terminated");
      goto error;
    }

  for (i = 0; i < media_types->len; i++)
    {
      guint media_type = g_array_index (media_types, guint, i);

      switch (media_type)
        {
        case TP_MEDIA_STREAM_TYPE_AUDIO:
        case TP_MEDIA_STREAM_TYPE_VIDEO:
          break;
        default:
          g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
              "%u is not a valid Media_Stream_Type", media_type);
          goto error;
        }
    }

  array = g_ptr_array_sized_new (media_types->len);

  for (i = 0; i < media_types->len; i++)
    {
      guint media_type = g_array_index (media_types, guint, i);
      ExampleCallableMediaStream *stream;
      GValueArray *info;

      if (self->priv->progress < PROGRESS_CALLING)
        {
          TpIntSet *peer_set = tp_intset_new_containing (self->priv->handle);
          const gchar *peer;

          g_message ("SIGNALLING: send: new streamed media call");
          self->priv->progress = PROGRESS_CALLING;

          tp_group_mixin_change_members ((GObject *) self, "",
              NULL /* nobody added */,
              NULL /* nobody removed */,
              NULL /* nobody added to local-pending */,
              peer_set /* added to remote-pending */,
              self->group.self_handle /* actor */,
              TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

          tp_intset_destroy (peer_set);

          /* In this example there is no real contact, so just simulate them
           * answering after a short time - unless the contact's name
           * contains "(no answer)" or "(busy)" */

          peer = tp_handle_inspect (contact_repo, self->priv->handle);

          if (strstr (peer, "(busy)") != NULL)
            {
              g_timeout_add_full (G_PRIORITY_DEFAULT,
                  self->priv->simulation_delay,
                  simulate_contact_busy_cb, g_object_ref (self),
                  g_object_unref);
            }
          else if (strstr (peer, "(no answer)") != NULL)
            {
              /* do nothing - the call just rings forever */
            }
          else
            {
              g_timeout_add_full (G_PRIORITY_DEFAULT,
                  self->priv->simulation_delay,
                  simulate_contact_answered_cb, g_object_ref (self),
                  g_object_unref);
            }
        }

      stream = example_callable_media_channel_add_stream (self, media_type,
          TRUE);

      g_object_get (stream,
          "stream-info", &info,
          NULL);

      g_ptr_array_add (array, info);
    }

  tp_svc_channel_type_streamed_media_return_from_request_streams (context,
      array);
  g_boxed_free (TP_ARRAY_TYPE_MEDIA_STREAM_INFO_LIST, array);

  return;

error:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
media_iface_init (gpointer iface,
                  gpointer data)
{
  TpSvcChannelTypeStreamedMediaClass *klass = iface;

#define IMPLEMENT(x) \
  tp_svc_channel_type_streamed_media_implement_##x (klass, media_##x)
  IMPLEMENT (list_streams);
  IMPLEMENT (remove_streams);
  IMPLEMENT (request_stream_direction);
  IMPLEMENT (request_streams);
#undef IMPLEMENT
}

static gboolean
simulate_hold (gpointer p)
{
  ExampleCallableMediaChannel *self = p;

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_HELD;
  g_message ("SIGNALLING: hold state changed to held");
  tp_svc_channel_interface_hold_emit_hold_state_changed (self,
      self->priv->hold_state, self->priv->hold_state_reason);
  return FALSE;
}

static gboolean
simulate_unhold (gpointer p)
{
  ExampleCallableMediaChannel *self = p;

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_UNHELD;
  g_message ("SIGNALLING: hold state changed to unheld");
  tp_svc_channel_interface_hold_emit_hold_state_changed (self,
      self->priv->hold_state, self->priv->hold_state_reason);
  return FALSE;
}

static gboolean
simulate_inability_to_unhold (gpointer p)
{
  ExampleCallableMediaChannel *self = p;

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_PENDING_HOLD;
  g_message ("SIGNALLING: unable to unhold - hold state changed to "
      "pending hold");
  tp_svc_channel_interface_hold_emit_hold_state_changed (self,
      self->priv->hold_state, self->priv->hold_state_reason);
  /* hold again */
  g_timeout_add_full (G_PRIORITY_DEFAULT,
      self->priv->simulation_delay,
      simulate_hold, g_object_ref (self),
      g_object_unref);
  return FALSE;
}

static void
hold_get_hold_state (TpSvcChannelInterfaceHold *iface,
                     DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);

  tp_svc_channel_interface_hold_return_from_get_hold_state (context,
      self->priv->hold_state, self->priv->hold_state_reason);
}

static void
hold_request_hold (TpSvcChannelInterfaceHold *iface,
                   gboolean hold,
                   DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;
  const gchar *peer;
  GSourceFunc callback;

  if ((hold && self->priv->hold_state == TP_LOCAL_HOLD_STATE_HELD) ||
      (!hold && self->priv->hold_state == TP_LOCAL_HOLD_STATE_UNHELD))
    {
      tp_svc_channel_interface_hold_return_from_request_hold (context);
      return;
    }

  peer = tp_handle_inspect (contact_repo, self->priv->handle);

  if (!hold && strstr (peer, "(no unhold)") != NULL)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "unable to unhold");
      goto error;
    }

  self->priv->hold_state_reason = TP_LOCAL_HOLD_STATE_REASON_REQUESTED;

  if (hold)
    {
      self->priv->hold_state = TP_LOCAL_HOLD_STATE_PENDING_HOLD;
      callback = simulate_hold;
    }
  else
    {
      self->priv->hold_state = TP_LOCAL_HOLD_STATE_PENDING_UNHOLD;

      peer = tp_handle_inspect (contact_repo, self->priv->handle);

      if (strstr (peer, "(inability to unhold)") != NULL)
        {
          callback = simulate_inability_to_unhold;
        }
      else
        {
          callback = simulate_unhold;
        }
    }

  g_message ("SIGNALLING: hold state changed to pending %s",
             (hold ? "hold" : "unhold"));
  tp_svc_channel_interface_hold_emit_hold_state_changed (iface,
    self->priv->hold_state, self->priv->hold_state_reason);

  g_timeout_add_full (G_PRIORITY_DEFAULT,
      self->priv->simulation_delay,
      callback, g_object_ref (self),
      g_object_unref);

  tp_svc_channel_interface_hold_return_from_request_hold (context);
  return;

error:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}


void
hold_iface_init (gpointer iface,
                 gpointer data)
{
  TpSvcChannelInterfaceHoldClass *klass = iface;

#define IMPLEMENT(x) \
  tp_svc_channel_interface_hold_implement_##x (klass, hold_##x)
  IMPLEMENT (get_hold_state);
  IMPLEMENT (request_hold);
#undef IMPLEMENT
}

static void
dtmf_start_tone (TpSvcChannelInterfaceDTMF *iface,
                 guint stream_id,
                 guchar event,
                 DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);
  ExampleCallableMediaStream *stream = g_hash_table_lookup (self->priv->streams,
      GUINT_TO_POINTER (stream_id));
  GError *error = NULL;
  guint media_type;

  if (stream == NULL)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "No stream with ID %u in this channel", stream_id);
      goto error;
    }

  g_object_get (G_OBJECT (stream), "type", &media_type, NULL);
  if (media_type != TP_MEDIA_STREAM_TYPE_AUDIO)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "DTMF is only supported by audio streams");
      goto error;
    }

  tp_svc_channel_interface_dtmf_return_from_start_tone (context);

  return;

error:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
dtmf_stop_tone (TpSvcChannelInterfaceDTMF *iface,
                guint stream_id,
                DBusGMethodInvocation *context)
{
  ExampleCallableMediaChannel *self = EXAMPLE_CALLABLE_MEDIA_CHANNEL (iface);
  ExampleCallableMediaStream *stream = g_hash_table_lookup (self->priv->streams,
      GUINT_TO_POINTER (stream_id));
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;
  const gchar *peer;
  guint media_type;

  if (stream == NULL)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "No stream with ID %u in this channel", stream_id);
      goto error;
    }

  g_object_get (G_OBJECT (stream), "type", &media_type, NULL);
  if (media_type != TP_MEDIA_STREAM_TYPE_AUDIO)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "DTMF is only supported by audio streams");
      goto error;
    }

  peer = tp_handle_inspect (contact_repo, self->priv->handle);
  if (strstr (peer, "(no continuous tone)") != NULL)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_NOT_AVAILABLE,
          "Continuous tones are not supported by this stream");
      goto error;
    }

  tp_svc_channel_interface_dtmf_return_from_stop_tone (context);

  return;

error:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
dtmf_iface_init (gpointer iface,
                 gpointer data)
{
  TpSvcChannelInterfaceDTMFClass *klass = iface;

#define IMPLEMENT(x) \
  tp_svc_channel_interface_dtmf_implement_##x (klass, dtmf_##x)
  IMPLEMENT (start_tone);
  IMPLEMENT (stop_tone);
#undef IMPLEMENT
}
