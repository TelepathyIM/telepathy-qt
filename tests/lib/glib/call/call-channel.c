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
#include <telepathy-glib/svc-call.h>
#include <telepathy-glib/telepathy-glib.h>

#include "call-content.h"
#include "call-stream.h"

static void hold_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (ExampleCallChannel,
    example_call_channel,
    TP_TYPE_BASE_MEDIA_CALL_CHANNEL,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_HOLD,
      hold_iface_init))

enum
{
  PROP_SIMULATION_DELAY = 1,
  N_PROPS
};

struct _ExampleCallChannelPrivate
{
  guint simulation_delay;
  TpBaseConnection *conn;
  TpHandle handle;
  gboolean locally_requested;

  guint hold_state;
  guint hold_state_reason;

  guint next_stream_id;
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
    TpCallState state,
    TpCallFlags flags,
    TpHandle actor,
    TpCallStateChangeReason reason,
    const gchar *error,
    ...)
{
  /* FIXME: TpBaseCallChannel is not that flexible */
  tp_base_call_channel_set_state ((TpBaseCallChannel *) self,
      state, actor, reason, error, "");
}

static void
example_call_channel_init (ExampleCallChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALL_CHANNEL,
      ExampleCallChannelPrivate);

  self->priv->next_stream_id = 1;

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
  TpBaseChannel *base = (TpBaseChannel *) self;
  TpBaseCallChannel *call = (TpBaseCallChannel *) self;

  if (chain_up != NULL)
    chain_up (object);

  self->priv->handle = tp_base_channel_get_target_handle (base);
  self->priv->locally_requested = tp_base_channel_is_requested (base);
  self->priv->conn = tp_base_channel_get_connection (base);

  tp_base_call_channel_update_member_flags (call, self->priv->handle, 0,
      0, TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");

  if (self->priv->locally_requested)
    {
      /* Nobody is locally pending. The remote peer will turn up in
       * remote-pending state when we actually contact them, which is done
       * in example_call_channel_initiate_outgoing. */
      example_call_channel_set_state (self,
          TP_CALL_STATE_PENDING_INITIATOR, 0, 0,
          TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
          NULL);
    }
  else
    {
      /* This is an incoming call, so the self-handle is locally
       * pending, to indicate that we need to answer. */
      example_call_channel_set_state (self,
          TP_CALL_STATE_INITIALISED, 0, self->priv->handle,
          TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
          NULL);
    }

  /* FIXME: should respect initial names */
  if (tp_base_call_channel_has_initial_audio (call, NULL))
    {
      g_message ("Channel initially has an audio stream");
      example_call_channel_add_content (self, TP_MEDIA_STREAM_TYPE_AUDIO,
          self->priv->locally_requested, TRUE, NULL, NULL);
    }

  if (tp_base_call_channel_has_initial_video (call, NULL))
    {
      g_message ("Channel initially has a video stream");
      example_call_channel_add_content (self, TP_MEDIA_STREAM_TYPE_VIDEO,
      self->priv->locally_requested, TRUE, NULL, NULL);
    }

  tp_base_channel_register (base);
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
    case PROP_SIMULATION_DELAY:
      g_value_set_uint (value, self->priv->simulation_delay);
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
    case PROP_SIMULATION_DELAY:
      self->priv->simulation_delay = g_value_get_uint (value);
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
    TpCallStateChangeReason call_reason,
    const gchar *error_name)
{
  TpBaseCallChannel *base = (TpBaseCallChannel *) self;
  TpCallState call_state = tp_base_call_channel_get_state (base);

  if (call_state != TP_CALL_STATE_ENDED)
    {
      GList *contents;

      example_call_channel_set_state (self,
          TP_CALL_STATE_ENDED, 0, actor,
          call_reason, error_name,
          NULL);

      /* FIXME: fd.o #24936 #c20: it's unclear in the spec whether we should
       * remove peers on call termination or not. For now this example does. */
      tp_base_call_channel_remove_member (base, self->priv->handle,
          actor, call_reason, error_name, NULL);

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
       * streams-removed handler) while iterating over it, we have to copy the
       * keys and iterate over those */
      contents = tp_base_call_channel_get_contents (base);
      contents = g_list_copy (contents);
      for (; contents != NULL; contents = g_list_delete_link (contents, contents))
        {
          example_call_content_remove_stream (contents->data);
          tp_base_call_channel_remove_content (base, contents->data,
              0, call_reason, error_name, "");
        }
    }
}

static void
dispose (GObject *object)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (object);

  /* the manager is meant to hold a ref to us until we've closed */
  g_assert (self->priv->closed);

  ((GObjectClass *) example_call_channel_parent_class)->dispose (object);
}

static void
close_channel (TpBaseChannel *base)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (base);

  example_call_channel_terminate (self,
      tp_base_connection_get_self_handle (self->priv->conn),
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
      TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "");

  self->priv->closed = TRUE;

  tp_base_channel_destroyed (base);
}

static void call_accept (TpBaseCallChannel *self);
static TpBaseCallContent * call_add_content (TpBaseCallChannel *self,
      const gchar *name,
      TpMediaStreamType media,
      TpMediaStreamDirection initial_direction,
      GError **error);
static void call_hangup (TpBaseCallChannel *self,
      guint reason,
      const gchar *detailed_reason,
      const gchar *message);

static void
example_call_channel_class_init (ExampleCallChannelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);
  TpBaseCallChannelClass *call_class = (TpBaseCallChannelClass *) klass;
  GParamSpec *param_spec;

  g_type_class_add_private (klass,
      sizeof (ExampleCallChannelPrivate));

  call_class->accept = call_accept;
  call_class->add_content = call_add_content;
  call_class->hangup = call_hangup;

  base_class->target_handle_type = TP_HANDLE_TYPE_CONTACT;
  base_class->interfaces = example_call_channel_interfaces;
  base_class->close = close_channel;

  object_class->constructed = constructed;
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->dispose = dispose;

  param_spec = g_param_spec_uint ("simulation-delay", "Simulation delay",
      "Delay between simulated network events",
      0, G_MAXUINT32, 1000,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SIMULATION_DELAY,
      param_spec);
}

static gboolean
simulate_contact_ended_cb (gpointer p)
{
  ExampleCallChannel *self = p;
  TpBaseCallChannel *base = (TpBaseCallChannel *) self;
  TpCallState call_state = tp_base_call_channel_get_state (base);

  /* if the call has been cancelled while we were waiting for the
   * contact to do so, do nothing! */
  if (call_state == TP_CALL_STATE_ENDED)
    return FALSE;

  g_message ("SIGNALLING: receive: call terminated: <call-terminated/>");

  example_call_channel_terminate (self, self->priv->handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
      TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "");

  return FALSE;
}

static gboolean
simulate_contact_answered_cb (gpointer p)
{
  ExampleCallChannel *self = p;
  TpBaseCallChannel *base = (TpBaseCallChannel *) self;
  TpCallState call_state = tp_base_call_channel_get_state (base);
  GList *contents;
  TpHandleRepoIface *contact_repo;
  const gchar *peer;

  /* if the call has been cancelled while we were waiting for the
   * contact to answer, do nothing! */
  if (call_state == TP_CALL_STATE_ENDED)
    return FALSE;

  /* otherwise, we're waiting for a response from the contact, which now
   * arrives */
  g_assert_cmpuint (call_state, ==, TP_CALL_STATE_INITIALISED);

  g_message ("SIGNALLING: receive: contact answered our call");

  tp_base_call_channel_remote_accept (base);

  contents = tp_base_call_channel_get_contents (base);
  for (; contents != NULL; contents = contents->next)
    {
      ExampleCallStream *stream = example_call_content_get_stream (contents->data);

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
  TpBaseCallChannel *base = (TpBaseCallChannel *) self;
  TpCallState call_state = tp_base_call_channel_get_state (base);

  /* if the call has been cancelled while we were waiting for the
   * contact to answer, do nothing */
  if (call_state == TP_CALL_STATE_ENDED)
    return FALSE;

  /* otherwise, we're waiting for a response from the contact, which now
   * arrives */
  g_assert_cmpuint (call_state, ==, TP_CALL_STATE_INITIALISED);

  g_message ("SIGNALLING: receive: call terminated: <user-is-busy/>");

  example_call_channel_terminate (self, self->priv->handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_BUSY,
      TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED,
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
  TpBaseCallChannel *base = (TpBaseCallChannel *) self;
  GList *contents;
  const gchar *type_str;
  TpHandle creator = self->priv->handle;
  TpCallContentDisposition disposition =
    TP_CALL_CONTENT_DISPOSITION_NONE;
  guint id = self->priv->next_stream_id++;
  ExampleCallContent *content;
  ExampleCallStream *stream;
  gchar *name;
  gchar *path;
  guint i;

  /* an arbitrary limit much less than 2**32 means we don't use ridiculous
   * amounts of memory, and also means @i can't wrap around when we use it to
   * uniquify content names. */
  contents = tp_base_call_channel_get_contents (base);
  if (g_list_length (contents) > MAX_CONTENTS_PER_CALL)
    {
      g_set_error (error, TP_ERROR, TP_ERROR_PERMISSION_DENIED,
          "What are you doing with all those contents anyway?!");
      return NULL;
    }

  type_str = (media_type == TP_MEDIA_STREAM_TYPE_AUDIO ? "audio" : "video");
  if (tp_str_empty (requested_name))
    {
      requested_name = type_str;
    }

  for (i = 0; ; i++)
    {
      GList *l;

      if (i == 0)
        name = g_strdup (requested_name);
      else
        name = g_strdup_printf ("%s (%u)", requested_name, i);

      for (l = contents; l != NULL; l = l->next)
        {
          if (!tp_strdiff (tp_base_call_content_get_name (l->data), name))
            break;
        }

      if (l == NULL)
        {
          /* this name hasn't been used - good enough */
          break;
        }

      g_free (name);
      name = NULL;
    }

  if (initial)
    disposition = TP_CALL_CONTENT_DISPOSITION_INITIAL;

  if (locally_requested)
    {
      g_message ("SIGNALLING: send: new %s stream %s", type_str, name);
      creator = self->priv->conn->self_handle;
    }

  path = g_strdup_printf ("%s/Content%u",
      tp_base_channel_get_object_path ((TpBaseChannel *) self),
      id);
  content = g_object_new (EXAMPLE_TYPE_CALL_CONTENT,
      "connection", self->priv->conn,
      "creator", creator,
      "media-type", media_type,
      "name", name,
      "disposition", disposition,
      "object-path", path,
      NULL);

  tp_base_call_channel_add_content (base, (TpBaseCallContent *) content);
  g_free (path);

  path = g_strdup_printf ("%s/Stream%u",
      tp_base_channel_get_object_path ((TpBaseChannel *) self),
      id);
  stream = g_object_new (EXAMPLE_TYPE_CALL_STREAM,
      "connection", self->priv->conn,
      "handle", self->priv->handle,
      "locally-requested", locally_requested,
      "object-path", path,
      NULL);

  example_call_content_add_stream (content, stream);
  g_free (path);

  g_object_unref (content);
  g_object_unref (stream);

  return content;
}

static gboolean
simulate_contact_ringing_cb (gpointer p)
{
  ExampleCallChannel *self = p;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  const gchar *peer;

  tp_base_call_channel_update_member_flags ((TpBaseCallChannel *) self,
      self->priv->handle, TP_CALL_MEMBER_FLAG_RINGING,
      0, TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");

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
      TP_CALL_STATE_INITIALISED, 0,
      tp_base_connection_get_self_handle (self->priv->conn),
      TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
      NULL);

  /* After a moment, we're sent an informational message saying it's ringing */
  g_timeout_add_full (G_PRIORITY_DEFAULT,
      self->priv->simulation_delay,
      simulate_contact_ringing_cb, g_object_ref (self),
      g_object_unref);
}

static void
accept_incoming_call (ExampleCallChannel *self)
{
  TpBaseCallChannel *base = (TpBaseCallChannel *) self;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  GList *contents;

  g_message ("SIGNALLING: send: Accepting incoming call from %s",
      tp_handle_inspect (contact_repo, self->priv->handle));

  example_call_channel_set_state (self,
      TP_CALL_STATE_ACCEPTED, 0,
      tp_base_connection_get_self_handle (self->priv->conn),
      TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "",
      NULL);

  contents = tp_base_call_channel_get_contents (base);
  for (; contents != NULL; contents = contents->next)
    {
      ExampleCallStream *stream = example_call_content_get_stream (contents->data);
      guint disposition = tp_base_call_content_get_disposition (contents->data);

      if (stream == NULL || disposition != TP_CALL_CONTENT_DISPOSITION_INITIAL)
        continue;

      /* we accept the proposed stream direction */
      example_call_stream_accept_proposed_direction (stream);
    }
}

static void
call_accept (TpBaseCallChannel *base)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (base);

  if (self->priv->locally_requested)
    {
      /* Take the contents we've already added, and make them happen */
      example_call_channel_initiate_outgoing (self);
    }
  else
    {
      accept_incoming_call (self);
    }
}

static void
call_hangup (TpBaseCallChannel *base,
    guint reason,
    const gchar *detailed_reason,
    const gchar *message G_GNUC_UNUSED)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (base);

  example_call_channel_terminate (self,
      tp_base_connection_get_self_handle (self->priv->conn),
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE, reason, detailed_reason);
}

static TpBaseCallContent *
call_add_content (TpBaseCallChannel *base,
    const gchar *content_name,
    guint content_type,
      TpMediaStreamDirection initial_direction,
    GError **error)
{
  ExampleCallChannel *self = EXAMPLE_CALL_CHANNEL (base);

  return (TpBaseCallContent *) example_call_channel_add_content (self,
      content_type, TRUE, FALSE, content_name, error);
}

static gboolean
simulate_hold (gpointer p)
{
  ExampleCallChannel *self = p;
  TpBaseCallChannel *base = (TpBaseCallChannel *) self;
  TpCallState call_state = tp_base_call_channel_get_state (base);
  TpCallFlags call_flags = 0; /* FIXME */

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_HELD;
  g_message ("SIGNALLING: hold state changed to held");
  tp_svc_channel_interface_hold_emit_hold_state_changed (self,
      self->priv->hold_state, self->priv->hold_state_reason);

  example_call_channel_set_state (self, call_state,
      call_flags | TP_CALL_FLAG_LOCALLY_HELD,
      tp_base_connection_get_self_handle (self->priv->conn),
      TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "", NULL);

  return FALSE;
}

static gboolean
simulate_unhold (gpointer p)
{
  ExampleCallChannel *self = p;
  TpBaseCallChannel *base = (TpBaseCallChannel *) self;
  TpCallState call_state = tp_base_call_channel_get_state (base);
  TpCallFlags call_flags = 0; /* FIXME */

  self->priv->hold_state = TP_LOCAL_HOLD_STATE_UNHELD;
  g_message ("SIGNALLING: hold state changed to unheld");
  tp_svc_channel_interface_hold_emit_hold_state_changed (self,
      self->priv->hold_state, self->priv->hold_state_reason);

  example_call_channel_set_state (self, call_state,
      call_flags & ~TP_CALL_FLAG_LOCALLY_HELD,
      tp_base_connection_get_self_handle (self->priv->conn),
      TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED, "", NULL);

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
