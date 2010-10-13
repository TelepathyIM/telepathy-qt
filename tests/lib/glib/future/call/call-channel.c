/*
 * call-channel.c - an example 1-1 audio/video call
 *
 * For simplicity, this channel emulates a device with its own
 * audio/video user interface, like a video-equipped form of the phones
 * manipulated by telepathy-snom or gnome-phone-manager.
 *
 * As a result, this channel has the HardwareStreaming flag, its contents
 * and streams do not have the Media interface, and clients should not attempt
 * to do their own streaming using telepathy-farsight, telepathy-stream-engine
 * or maemo-stream-engine.
 *
 * In practice, nearly all connection managers do not have HardwareStreaming,
 * and do have the Media interface on their contents/streams. Usage for those
 * CMs is the same, except that whichever client is the primary handler
 * for the channel should also hand the channel over to telepathy-farsight or
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

#include "call-channel.h"

#include <string.h>

#include <gobject/gvaluecollector.h>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/svc-channel.h>
#include <telepathy-glib/telepathy-glib.h>

#include "extensions/extensions.h"

#include "call-content.h"
#include "call-stream.h"

static void call_iface_init (gpointer iface, gpointer data);
static void channel_iface_init (gpointer iface, gpointer data);
static void hold_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (ExampleCallChannel,
    example_call_channel,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
    G_IMPLEMENT_INTERFACE (FUTURE_TYPE_SVC_CHANNEL_TYPE_CALL,
      call_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_HOLD,
      hold_iface_init);
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
  PROP_CONTENT_PATHS,
  PROP_CALL_STATE,
  PROP_CALL_FLAGS,
  PROP_CALL_STATE_REASON,
  PROP_CALL_STATE_DETAILS,
  PROP_HARDWARE_STREAMING,
  PROP_CALL_MEMBERS,
  PROP_INITIAL_TRANSPORT,
  PROP_MUTABLE_CONTENTS,
  N_PROPS
};

struct _ExampleCallChannelPrivate
{
  TpBaseConnection *conn;
  gchar *object_path;
  TpHandle handle;
  TpHandle initiator;

  FutureCallState call_state;
  FutureCallFlags call_flags;
  GValueArray *call_state_reason;
  GHashTable *call_state_details;
  FutureCallMemberFlags peer_flags;

  guint simulation_delay;

  guint next_stream_id;

  /* strdup'd name => referenced ExampleCallContent */
  GHashTable *contents;

  guint hold_state;
  guint hold_state_reason;

  gboolean locally_requested;
  gboolean initial_audio;
  gboolean initial_video;
  gboolean disposed;
  gboolean closed;
};

static const char * example_call_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_HOLD,
    NULL
};

/* In practice you need one for audio, plus one per video (e.g. a
 * presentation might have separate video contents for the slides
 * and a camera pointed at the presenter), so having more than three
 * would be highly unusual */
#define MAX_CONTENTS_PER_CALL 100

G_GNUC_NULL_TERMINATED static void
example_call_channel_set_state (ExampleCallChannel *self,
    FutureCallState state,
    FutureCallFlags flags,
    TpHandle actor,
    FutureCallStateChangeReason reason,
    const gchar *error,
    ...)
{
  TpHandleRepoIface *contact_handles = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  TpHandle old_actor;
  const gchar *key;
  va_list va;

  self->priv->call_state = state;
  self->priv->call_flags = flags;

  old_actor = g_value_get_uint (self->priv->call_state_reason->values + 0);

  if (actor != 0)
    tp_handle_ref (contact_handles, actor);

  if (old_actor != 0)
    tp_handle_unref (contact_handles, old_actor);

  g_value_set_uint (self->priv->call_state_reason->values + 0, actor);
  g_value_set_uint (self->priv->call_state_reason->values + 1, reason);
  g_value_set_string (self->priv->call_state_reason->values + 2, error);

  g_hash_table_remove_all (self->priv->call_state_details);

  va_start (va, error);

  /* This is basically tp_asv_new_va(), but that doesn't exist yet
   * (and when it does, we still won't want to use it in this example
   * just yet, because examples shouldn't use unreleased API) */
  for (key = va_arg (va, const gchar *);
       key != NULL;
       key = va_arg (va, const gchar *))
    {
      GType type = va_arg (va, GType);
      GValue *value = tp_g_value_slice_new (type);
      gchar *collect_error = NULL;

      G_VALUE_COLLECT (value, va, 0, &collect_error);

      if (collect_error != NULL)
        {
          g_critical ("key %s: %s", key, collect_error);
          g_free (collect_error);
          collect_error = NULL;
          tp_g_value_slice_free (value);
          continue;
        }

      g_hash_table_insert (self->priv->call_state_details,
          (gchar *) key, value);
    }

  va_end (va);

  future_svc_channel_type_call_emit_call_state_changed (self,
      self->priv->call_state, self->priv->call_flags,
      self->priv->call_state_reason, self->priv->call_state_details);
}

static void
example_call_channel_init (ExampleCallChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALL_CHANNEL,
      ExampleCallChannelPrivate);

  self->priv->next_stream_id = 1;
  self->priv->contents = g_hash_table_new_full (g_str_hash,
      g_str_equal, g_free, g_object_unref);

  self->priv->call_state = FUTURE_CALL_STATE_UNKNOWN; /* set in constructed */
  self->priv->call_flags = 0;
  self->priv->call_state_reason = tp_value_array_build (3,
      G_TYPE_UINT, 0,   /* actor */
      G_TYPE_UINT, FUTURE_CALL_STATE_CHANGE_REASON_UNKNOWN,
      G_TYPE_STRING, "",
      G_TYPE_INVALID);
  self->priv->call_state_details = tp_asv_new (
      NULL, NULL);

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_UNHELD;
  self->priv->hold_state_reason = TP_LOCAL_HOLD_STATE_REASON_NONE;
}

static ExampleCallContent *example_call_channel_add_content (
    ExampleCallChannel *self, TpMediaStreamType media_type,
    gboolean locally_requested, gboolean initial,
    const gchar *requested_name, GError **error);

static void example_call_channel_initiate_outgoing (ExampleCallChannel *self);

static void
constructed (GObject *object)
{
  void (*chain_up) (GObject *) =
      ((GObjectClass *) example_call_channel_parent_class)->constructed;
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (object);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);

  if (chain_up != NULL)
    chain_up (object);

  tp_handle_ref (contact_repo, self->priv->handle);
  tp_handle_ref (contact_repo, self->priv->initiator);

  tp_dbus_daemon_register_object (
      tp_base_connection_get_dbus_daemon (self->priv->conn),
      self->priv->object_path, self);

  if (self->priv->locally_requested)
    {
      /* Nobody is locally pending. The remote peer will turn up in
       * remote-pending state when we actually contact them, which is done
       * in example_call_channel_initiate_outgoing. */
      example_call_channel_set_state (self,
          FUTURE_CALL_STATE_PENDING_INITIATOR, 0, 0,
          FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
          NULL);
    }
  else
    {
      /* This is an incoming call, so the self-handle is locally
       * pending, to indicate that we need to answer. */
      example_call_channel_set_state (self,
          FUTURE_CALL_STATE_PENDING_RECEIVER, 0, self->priv->handle,
          FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
          NULL);
    }

  if (self->priv->locally_requested)
    {
      if (self->priv->initial_audio)
        {
          g_message ("Channel initially has an audio stream");
          example_call_channel_add_content (self,
              TP_MEDIA_STREAM_TYPE_AUDIO, TRUE, TRUE, NULL, NULL);
        }

      if (self->priv->initial_video)
        {
          g_message ("Channel initially has a video stream");
          example_call_channel_add_content (self,
              TP_MEDIA_STREAM_TYPE_VIDEO, TRUE, TRUE, NULL, NULL);
        }
    }
  else
    {
      /* the caller has almost certainly asked us for some streams - there's
       * not much point in having a call otherwise */

      if (self->priv->initial_audio)
        {
          g_message ("Channel initially has an audio stream");
          example_call_channel_add_content (self,
              TP_MEDIA_STREAM_TYPE_AUDIO, FALSE, TRUE, NULL, NULL);
        }

      if (self->priv->initial_video)
        {
          g_message ("Channel initially has a video stream");
          example_call_channel_add_content (self,
              TP_MEDIA_STREAM_TYPE_VIDEO, FALSE, TRUE, NULL, NULL);
        }
    }
}

static void
get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;

    case PROP_CHANNEL_TYPE:
      g_value_set_static_string (value, FUTURE_IFACE_CHANNEL_TYPE_CALL);
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
      g_value_set_boxed (value, example_call_channel_interfaces);
      break;

    case PROP_CHANNEL_DESTROYED:
      g_value_set_boolean (value,
          (self->priv->call_state == FUTURE_CALL_STATE_ENDED));
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

    case PROP_CONTENT_PATHS:
        {
          GPtrArray *paths = g_ptr_array_sized_new (g_hash_table_size (
                self->priv->contents));
          GHashTableIter iter;
          gpointer v;

          g_hash_table_iter_init (&iter, self->priv->contents);

          while (g_hash_table_iter_next (&iter, NULL, &v))
            {
              gchar *path;

              g_object_get (v,
                  "object-path", &path,
                  NULL);

              g_ptr_array_add (paths, path);
            }

          g_value_take_boxed (value, paths);
        }
      break;

    case PROP_CALL_STATE:
      g_value_set_uint (value, self->priv->call_state);
      break;

    case PROP_CALL_FLAGS:
      g_value_set_uint (value, self->priv->call_flags);
      break;

    case PROP_CALL_STATE_REASON:
      g_value_set_boxed (value, self->priv->call_state_reason);
      break;

    case PROP_CALL_STATE_DETAILS:
      g_value_set_boxed (value, self->priv->call_state_details);
      break;

    case PROP_HARDWARE_STREAMING:
      /* yes, this implementation has hardware streaming */
      g_value_set_boolean (value, TRUE);
      break;

    case PROP_MUTABLE_CONTENTS:
      /* yes, this implementation can add contents */
      g_value_set_boolean (value, TRUE);
      break;

    case PROP_INITIAL_TRANSPORT:
      /* this implementation has hardware_streaming, so the initial
       * transport is rather meaningless */
      g_value_set_static_string (value, "");
      break;

    case PROP_CALL_MEMBERS:
        {
          GHashTable *uu_map = g_hash_table_new (NULL, NULL);

          /* There is one contact, other than the self-handle. */
          g_hash_table_insert (uu_map, GUINT_TO_POINTER (self->priv->handle),
              GUINT_TO_POINTER (self->priv->peer_flags));
          g_value_take_boxed (value, uu_map);
        }
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
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_assert (self->priv->object_path == NULL);   /* construct-only */
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
example_call_channel_terminate (ExampleCallChannel *self,
    TpHandle actor,
    TpChannelGroupChangeReason reason,
    FutureCallStateChangeReason call_reason,
    const gchar *error_name)
{
  if (self->priv->call_state != FUTURE_CALL_STATE_ENDED)
    {
      GList *values;
      GHashTable *empty_uu_map = g_hash_table_new (NULL, NULL);
      GArray *au = g_array_sized_new (FALSE, FALSE, sizeof (guint), 1);

      example_call_channel_set_state (self,
          FUTURE_CALL_STATE_ENDED, 0, actor,
          call_reason, error_name,
          NULL);

      /* FIXME: fd.o #24936 #c20: it's unclear in the spec whether we should
       * remove peers on call termination or not. For now this example does. */
      g_array_append_val (au, self->priv->handle);
      future_svc_channel_type_call_emit_call_members_changed (self,
          empty_uu_map, au);
      g_hash_table_unref (empty_uu_map);
      g_array_free (au, TRUE);

      if (actor == tp_base_connection_get_self_handle (self->priv->conn))
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

      /* terminate all streams: to avoid modifying the hash table (in the
       * stream-removed handler) while iterating over it, we have to copy the
       * keys and iterate over those */
      values = g_hash_table_get_values (self->priv->contents);
      g_list_foreach (values, (GFunc) g_object_ref, NULL);

      for (; values != NULL; values = g_list_delete_link (values, values))
        {
          ExampleCallStream *stream =
            example_call_content_get_stream (values->data);

          if (stream != NULL)
            example_call_stream_close (stream);

          g_object_unref (values->data);
        }
    }
}

void
example_call_channel_disconnected (ExampleCallChannel *self)
{
  example_call_channel_terminate (self, 0,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
      FUTURE_CALL_STATE_CHANGE_REASON_UNKNOWN,
      TP_ERROR_STR_DISCONNECTED);

  if (!self->priv->closed)
    {
      self->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }
}

static void
dispose (GObject *object)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (object);

  if (self->priv->disposed)
    return;

  self->priv->disposed = TRUE;

  g_hash_table_destroy (self->priv->contents);
  self->priv->contents = NULL;

  /* FIXME: right error code? arguably this should always be a no-op */
  example_call_channel_terminate (self,
      tp_base_connection_get_self_handle (self->priv->conn),
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
      FUTURE_CALL_STATE_CHANGE_REASON_UNKNOWN, "");

  /* the manager is meant to hold a ref to us until we've closed */
  g_assert (self->priv->closed);

  ((GObjectClass *) example_call_channel_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (object);
  TpHandleRepoIface *contact_handles = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);

  g_value_array_free (self->priv->call_state_reason);
  g_hash_table_destroy (self->priv->call_state_details);

  tp_handle_unref (contact_handles, self->priv->handle);
  tp_handle_unref (contact_handles, self->priv->initiator);

  g_free (self->priv->object_path);

  ((GObjectClass *) example_call_channel_parent_class)->finalize (object);
}

static void
example_call_channel_class_init (ExampleCallChannelClass *klass)
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
  static TpDBusPropertiesMixinPropImpl call_props[] = {
      { "Contents", "content-paths", NULL },
      { "CallState", "call-state", NULL },
      { "CallFlags", "call-flags", NULL },
      { "CallStateReason", "call-state-reason", NULL },
      { "CallStateDetails", "call-state-details", NULL },
      { "HardwareStreaming", "hardware-streaming", NULL },
      { "CallMembers", "call-members", NULL },
      { "InitialTransport", "initial-transport", NULL },
      { "InitialAudio", "initial-audio", NULL },
      { "InitialVideo", "initial-video", NULL },
      { "MutableContents", "mutable-contents", NULL },
      { NULL }
  };
  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
      { TP_IFACE_CHANNEL,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        channel_props,
      },
      { FUTURE_IFACE_CHANNEL_TYPE_CALL,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        call_props,
      },
      { NULL }
  };
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  g_type_class_add_private (klass,
      sizeof (ExampleCallChannelPrivate));

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

  param_spec = g_param_spec_boxed ("content-paths", "Content paths",
      "A list of the object paths of contents",
      TP_ARRAY_TYPE_OBJECT_PATH_LIST,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONTENT_PATHS,
      param_spec);

  param_spec = g_param_spec_uint ("call-state", "Call state",
      "High-level state of the call",
      0, NUM_FUTURE_CALL_STATES - 1, FUTURE_CALL_STATE_PENDING_INITIATOR,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CALL_STATE,
      param_spec);

  param_spec = g_param_spec_uint ("call-flags", "Call flags",
      "Flags for additional sub-states",
      0, G_MAXUINT32, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CALL_FLAGS,
      param_spec);

  param_spec = g_param_spec_boxed ("call-state-reason", "Call state reason",
      "Reason for call-state and call-flags",
      FUTURE_STRUCT_TYPE_CALL_STATE_REASON,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CALL_STATE_REASON,
      param_spec);

  param_spec = g_param_spec_boxed ("call-state-details", "Call state details",
      "Additional details of the call state/flags/reason",
      TP_HASH_TYPE_STRING_VARIANT_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CALL_STATE_DETAILS,
      param_spec);

  param_spec = g_param_spec_boolean ("hardware-streaming",
      "Hardware streaming?",
      "True if this channel does all of its own streaming (it does)",
      TRUE,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_HARDWARE_STREAMING,
      param_spec);

  param_spec = g_param_spec_boxed ("call-members", "Call members",
      "A map from call members (only one in this example) to their states",
      FUTURE_HASH_TYPE_CALL_MEMBER_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CALL_MEMBERS,
      param_spec);

  param_spec = g_param_spec_string ("initial-transport", "Initial transport",
      "The initial transport for this channel (there is none)",
      "",
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INITIAL_TRANSPORT,
      param_spec);

  param_spec = g_param_spec_boolean ("mutable-contents", "Mutable contents?",
      "True if contents can be added to this channel (they can)",
      TRUE,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MUTABLE_CONTENTS,
      param_spec);

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCallChannelClass,
        dbus_properties_class));
}

static void
channel_close (TpSvcChannel *iface,
    DBusGMethodInvocation *context)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);

  example_call_channel_terminate (self,
      tp_base_connection_get_self_handle (self->priv->conn),
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "");

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
      FUTURE_IFACE_CHANNEL_TYPE_CALL);
}

static void
channel_get_handle (TpSvcChannel *iface,
    DBusGMethodInvocation *context)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);

  tp_svc_channel_return_from_get_handle (context, TP_HANDLE_TYPE_CONTACT,
      self->priv->handle);
}

static void
channel_get_interfaces (TpSvcChannel *iface G_GNUC_UNUSED,
    DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_interfaces (context,
      example_call_channel_interfaces);
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

#if 0
/* FIXME: there's no equivalent of this in Call (yet?) */

/* This is expressed in terms of streams because it's the old API, but it
 * really means removing contents. */
static void
media_remove_streams (TpSvcChannelTypeStreamedMedia *iface,
    const GArray *stream_ids,
    DBusGMethodInvocation *context)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);
  guint i;

  for (i = 0; i < stream_ids->len; i++)
    {
      guint id = g_array_index (stream_ids, guint, i);

      if (g_hash_table_lookup (self->priv->contents,
            GUINT_TO_POINTER (id)) == NULL)
        {
          GError *error = g_error_new (TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
              "No stream with ID %u in this channel", id);

          dbus_g_method_return_error (context, error);
          g_error_free (error);
          return;
        }
    }

  for (i = 0; i < stream_ids->len; i++)
    {
      guint id = g_array_index (stream_ids, guint, i);
      ExampleCallContent *content =
        g_hash_table_lookup (self->priv->contents, GUINT_TO_POINTER (id));
      ExampleCallStream *stream = example_call_content_get_stream (content);

      if (stream != NULL)
        example_call_stream_close (stream);
    }

  tp_svc_channel_type_streamed_media_return_from_remove_streams (context);
}
#endif

static void
stream_removed_cb (ExampleCallContent *content,
    const gchar *stream_path G_GNUC_UNUSED,
    ExampleCallChannel *self)
{
  gchar *path, *name;

  /* Contents in this example CM can only have one stream, so if their
   * stream disappears, the content has to be removed too. */

  g_object_get (content,
      "object-path", &path,
      "name", &name,
      NULL);

  g_hash_table_remove (self->priv->contents, name);

  future_svc_channel_type_call_emit_content_removed (self, path);
  g_free (path);
  g_free (name);

  if (g_hash_table_size (self->priv->contents) == 0)
    {
      /* no contents left, so the call terminates */
      example_call_channel_terminate (self, 0,
          TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
          FUTURE_CALL_STATE_CHANGE_REASON_UNKNOWN, "");
      /* FIXME: is there an appropriate error? */
    }
}

static void
content_removed_cb (ExampleCallContent *content,
    ExampleCallChannel *self)
{
  gchar *path, *name;

  g_object_get (content,
      "object-path", &path,
      "name", &name,
      NULL);

  g_hash_table_remove (self->priv->contents, name);

  future_svc_channel_type_call_emit_content_removed (self, path);
  g_free (path);
  g_free (name);

  if (g_hash_table_size (self->priv->contents) == 0)
    {
      /* no contents left, so the call terminates */
      example_call_channel_terminate (self, 0,
          TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
          FUTURE_CALL_STATE_CHANGE_REASON_UNKNOWN, "");
      /* FIXME: is there an appropriate error? */
    }
}

static gboolean
simulate_contact_ended_cb (gpointer p)
{
  ExampleCallChannel *self = p;

  /* if the call has been cancelled while we were waiting for the
   * contact to do so, do nothing! */
  if (self->priv->call_state == FUTURE_CALL_STATE_ENDED)
    return FALSE;

  g_message ("SIGNALLING: receive: call terminated: <call-terminated/>");

  example_call_channel_terminate (self, self->priv->handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "");

  return FALSE;
}

static gboolean
simulate_contact_answered_cb (gpointer p)
{
  ExampleCallChannel *self = p;
  GHashTableIter iter;
  gpointer v;
  TpHandleRepoIface *contact_repo;
  const gchar *peer;

  /* if the call has been cancelled while we were waiting for the
   * contact to answer, do nothing! */
  if (self->priv->call_state == FUTURE_CALL_STATE_ENDED)
    return FALSE;

  /* otherwise, we're waiting for a response from the contact, which now
   * arrives */
  g_assert (self->priv->call_state == FUTURE_CALL_STATE_PENDING_RECEIVER);

  g_message ("SIGNALLING: receive: contact answered our call");

  example_call_channel_set_state (self,
      FUTURE_CALL_STATE_ACCEPTED, 0, self->priv->handle,
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
      NULL);

  g_hash_table_iter_init (&iter, self->priv->contents);

  while (g_hash_table_iter_next (&iter, NULL, &v))
    {
      ExampleCallStream *stream = example_call_content_get_stream (v);

      if (stream == NULL)
        continue;

      /* remote contact accepts our proposed stream direction */
      example_call_stream_simulate_contact_agreed_to_send (stream);
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
  ExampleCallChannel *self = p;

  /* if the call has been cancelled while we were waiting for the
   * contact to answer, do nothing */
  if (self->priv->call_state == FUTURE_CALL_STATE_ENDED)
    return FALSE;

  /* otherwise, we're waiting for a response from the contact, which now
   * arrives */
  g_assert (self->priv->call_state == FUTURE_CALL_STATE_PENDING_RECEIVER);

  g_message ("SIGNALLING: receive: call terminated: <user-is-busy/>");

  example_call_channel_terminate (self, self->priv->handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_BUSY,
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED,
      TP_ERROR_STR_BUSY);

  return FALSE;
}

static ExampleCallContent *
example_call_channel_add_content (ExampleCallChannel *self,
    TpMediaStreamType media_type,
    gboolean locally_requested,
    gboolean initial,
    const gchar *requested_name,
    GError **error)
{
  ExampleCallContent *content;
  ExampleCallStream *stream;
  guint id = self->priv->next_stream_id++;
  const gchar *type_str;
  TpHandle creator;
  gchar *name;
  gchar *path;
  FutureCallContentDisposition disposition =
    FUTURE_CALL_CONTENT_DISPOSITION_NONE;
  guint i;

  type_str = (media_type == TP_MEDIA_STREAM_TYPE_AUDIO ? "audio" : "video");
  creator = self->priv->handle;

  /* an arbitrary limit much less than 2**32 means we don't use ridiculous
   * amounts of memory, and also means @i can't wrap around when we use it to
   * uniquify content names. */
  if (g_hash_table_size (self->priv->contents) > MAX_CONTENTS_PER_CALL)
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_PERMISSION_DENIED,
          "What are you doing with all those contents anyway?!");
      return NULL;
    }

  if (tp_str_empty (requested_name))
    {
      requested_name = type_str;
    }

  for (i = 0; ; i++)
    {
      if (i == 0)
        name = g_strdup (requested_name);
      else
        name = g_strdup_printf ("%s (%u)", requested_name, i);

      if (!g_hash_table_lookup_extended (self->priv->contents, name,
            NULL, NULL))
        {
          /* this name hasn't been used - good enough */
          break;
        }

      g_free (name);
      name = NULL;
    }

  if (initial)
    disposition = FUTURE_CALL_CONTENT_DISPOSITION_INITIAL;

  if (locally_requested)
    {
      g_message ("SIGNALLING: send: new %s stream %s", type_str, name);
      creator = self->priv->conn->self_handle;
    }

  path = g_strdup_printf ("%s/Content%u", self->priv->object_path, id);
  content = g_object_new (EXAMPLE_TYPE_CALL_CONTENT,
      "connection", self->priv->conn,
      "creator", creator,
      "type", media_type,
      "name", name,
      "disposition", disposition,
      "object-path", path,
      NULL);

  g_hash_table_insert (self->priv->contents, name, content);
  future_svc_channel_type_call_emit_content_added (self, path, media_type);
  g_free (path);

  path = g_strdup_printf ("%s/Stream%u", self->priv->object_path, id);
  stream = g_object_new (EXAMPLE_TYPE_CALL_STREAM,
      "connection", self->priv->conn,
      "handle", self->priv->handle,
      "locally-requested", locally_requested,
      "object-path", path,
      "simulation-delay", self->priv->simulation_delay,
      NULL);

  example_call_content_add_stream (content, stream);
  g_free (path);

  tp_g_signal_connect_object (content, "stream-removed",
      G_CALLBACK (stream_removed_cb), self, 0);

  g_signal_connect (content, "removed", (GCallback) content_removed_cb, self);

  return content;
}

static gboolean
simulate_contact_ringing_cb (gpointer p)
{
  ExampleCallChannel *self = p;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  const gchar *peer;
  GHashTable *uu_map = g_hash_table_new (NULL, NULL);
  GArray *empty_au = g_array_sized_new (FALSE, FALSE, sizeof (guint), 0);

  /* ring, ring! */
  self->priv->peer_flags = FUTURE_CALL_MEMBER_FLAG_RINGING;
  g_hash_table_insert (uu_map, GUINT_TO_POINTER (self->priv->handle),
      GUINT_TO_POINTER (self->priv->peer_flags));
  future_svc_channel_type_call_emit_call_members_changed (self,
      uu_map, empty_au);
  g_hash_table_unref (uu_map);
  g_array_free (empty_au, TRUE);


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

  return FALSE;
}

static void
example_call_channel_initiate_outgoing (ExampleCallChannel *self)
{
  g_message ("SIGNALLING: send: new streamed media call");
  example_call_channel_set_state (self,
      FUTURE_CALL_STATE_PENDING_RECEIVER, 0,
      tp_base_connection_get_self_handle (self->priv->conn),
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
      NULL);

  /* After a moment, we're sent an informational message saying it's ringing */
  g_timeout_add_full (G_PRIORITY_DEFAULT,
      self->priv->simulation_delay,
      simulate_contact_ringing_cb, g_object_ref (self),
      g_object_unref);
}

static void
call_ringing (FutureSvcChannelTypeCall *iface,
    DBusGMethodInvocation *context)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);
  GError *error = NULL;

  if (self->priv->locally_requested)
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "Ringing() makes no sense on an outgoing call");
      goto finally;
    }

  if (self->priv->call_state != FUTURE_CALL_STATE_PENDING_RECEIVER)
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "Ringing() makes no sense now that we're not pending receiver");
      goto finally;
    }

  g_message ("SIGNALLING: send: ring, ring!");

  example_call_channel_set_state (self, FUTURE_CALL_STATE_PENDING_RECEIVER,
      self->priv->call_flags | FUTURE_CALL_FLAG_LOCALLY_RINGING,
      tp_base_connection_get_self_handle (self->priv->conn),
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "", NULL);

finally:
  if (error == NULL)
    {
      future_svc_channel_type_call_return_from_ringing (context);
    }
  else
    {
      dbus_g_method_return_error (context, error);
      g_error_free (error);
    }
}

static void
accept_incoming_call (ExampleCallChannel *self)
{
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  GHashTableIter iter;
  gpointer v;

  g_assert (self->priv->call_state == FUTURE_CALL_STATE_PENDING_RECEIVER);

  g_message ("SIGNALLING: send: Accepting incoming call from %s",
      tp_handle_inspect (contact_repo, self->priv->handle));

  example_call_channel_set_state (self,
      FUTURE_CALL_STATE_ACCEPTED, 0,
      tp_base_connection_get_self_handle (self->priv->conn),
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
      NULL);

  g_hash_table_iter_init (&iter, self->priv->contents);

  while (g_hash_table_iter_next (&iter, NULL, &v))
    {
      ExampleCallStream *stream = example_call_content_get_stream (v);
      guint disposition;

      g_object_get (v,
          "disposition", &disposition,
          NULL);

      if (stream == NULL ||
          disposition != FUTURE_CALL_CONTENT_DISPOSITION_INITIAL)
        continue;

      /* we accept the proposed stream direction */
      example_call_stream_accept_proposed_direction (stream);
    }
}

static void
call_accept (FutureSvcChannelTypeCall *iface G_GNUC_UNUSED,
    DBusGMethodInvocation *context)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);

  if (self->priv->locally_requested)
    {
      if (self->priv->call_state == FUTURE_CALL_STATE_PENDING_INITIATOR)
        {
          /* Take the contents we've already added, and make them happen */
          example_call_channel_initiate_outgoing (self);

          future_svc_channel_type_call_return_from_accept (context);
        }
      else if (self->priv->call_state == FUTURE_CALL_STATE_ENDED)
        {
          GError na = { TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
              "This call has already ended" };

          dbus_g_method_return_error (context, &na);
        }
      else
        {
          GError na = { TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
              "This outgoing call has already been started" };

          dbus_g_method_return_error (context, &na);
        }
    }
  else
    {
      if (self->priv->call_state == FUTURE_CALL_STATE_PENDING_RECEIVER)
        {
          accept_incoming_call (self);
          future_svc_channel_type_call_return_from_accept (context);
        }
      else if (self->priv->call_state == FUTURE_CALL_STATE_ENDED)
        {
          GError na = { TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
              "This call has already ended" };

          dbus_g_method_return_error (context, &na);
        }
      else
        {
          GError na = { TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
              "This incoming call has already been accepted" };

          dbus_g_method_return_error (context, &na);
        }
    }
}

static void
call_hangup (FutureSvcChannelTypeCall *iface,
    guint reason,
    const gchar *detailed_reason,
    const gchar *message G_GNUC_UNUSED,
    DBusGMethodInvocation *context)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);

  if (self->priv->call_state == FUTURE_CALL_STATE_ENDED)
    {
      GError na = { TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "This call has already ended" };

      dbus_g_method_return_error (context, &na);
      return;
    }
  else
    {
      example_call_channel_terminate (self,
          tp_base_connection_get_self_handle (self->priv->conn),
          TP_CHANNEL_GROUP_CHANGE_REASON_NONE, reason, detailed_reason);
      future_svc_channel_type_call_return_from_hangup (context);
    }
}

static void
call_add_content (FutureSvcChannelTypeCall *iface,
    const gchar *content_name,
    guint content_type,
    DBusGMethodInvocation *context)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);
  GError *error = NULL;
  gchar *content_path;
  ExampleCallContent *content;

  switch (content_type)
    {
    case TP_MEDIA_STREAM_TYPE_AUDIO:
    case TP_MEDIA_STREAM_TYPE_VIDEO:
      break;

    default:
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "%u is not a supported Media_Stream_Type", content_type);
      goto error;
    }

  content = example_call_channel_add_content (self, content_type, TRUE, FALSE,
      content_name, &error);

  if (content == NULL)
    goto error;

  g_object_get (content,
      "object-path", &content_path,
      NULL);
  future_svc_channel_type_call_return_from_add_content (context,
      content_path);
  g_free (content_path);

  return;

error:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
call_iface_init (gpointer iface,
    gpointer data)
{
  FutureSvcChannelTypeCallClass *klass = iface;

#define IMPLEMENT(x) \
  future_svc_channel_type_call_implement_##x (klass, call_##x)
  IMPLEMENT (ringing);
  IMPLEMENT (hangup);
  IMPLEMENT (accept);
  IMPLEMENT (add_content);
#undef IMPLEMENT
}

static gboolean
simulate_hold (gpointer p)
{
  ExampleCallChannel *self = p;

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_HELD;
  g_message ("SIGNALLING: hold state changed to held");
  tp_svc_channel_interface_hold_emit_hold_state_changed (self,
      self->priv->hold_state, self->priv->hold_state_reason);

  example_call_channel_set_state (self, self->priv->call_state,
      self->priv->call_flags | FUTURE_CALL_FLAG_LOCALLY_HELD,
      tp_base_connection_get_self_handle (self->priv->conn),
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "", NULL);

  return FALSE;
}

static gboolean
simulate_unhold (gpointer p)
{
  ExampleCallChannel *self = p;

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_UNHELD;
  g_message ("SIGNALLING: hold state changed to unheld");
  tp_svc_channel_interface_hold_emit_hold_state_changed (self,
      self->priv->hold_state, self->priv->hold_state_reason);

  example_call_channel_set_state (self, self->priv->call_state,
      self->priv->call_flags & ~FUTURE_CALL_FLAG_LOCALLY_HELD,
      tp_base_connection_get_self_handle (self->priv->conn),
      FUTURE_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "", NULL);

  return FALSE;
}

static gboolean
simulate_inability_to_unhold (gpointer p)
{
  ExampleCallChannel *self = p;

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
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);

  tp_svc_channel_interface_hold_return_from_get_hold_state (context,
      self->priv->hold_state, self->priv->hold_state_reason);
}

static void
hold_request_hold (TpSvcChannelInterfaceHold *iface,
    gboolean hold,
    DBusGMethodInvocation *context)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (iface);
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
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
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
  /* No need to change the call flags - we never change the actual hold state
   * here, only the pending hold state */

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
