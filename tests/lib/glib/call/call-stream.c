/*
 * call-stream.c - a stream in a call.
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

#include "call-stream.h"

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/svc-call.h>

G_DEFINE_TYPE (ExampleCallStream,
    example_call_stream,
    TP_TYPE_BASE_MEDIA_CALL_STREAM)

enum
{
  PROP_SIMULATION_DELAY = 1,
  PROP_LOCALLY_REQUESTED,
  PROP_HANDLE,
  N_PROPS
};

struct _ExampleCallStreamPrivate
{
  guint simulation_delay;
  gboolean locally_requested;
  TpHandle handle;
  guint agreed_delay_id;
};

static void
example_call_stream_init (ExampleCallStream *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALL_STREAM,
      ExampleCallStreamPrivate);
}

static void example_call_stream_receive_direction_request (
    ExampleCallStream *self, gboolean local_send, gboolean remote_send);
static void example_call_stream_change_direction (ExampleCallStream *self,
    gboolean want_to_send, gboolean want_to_receive);

static void
constructed (GObject *object)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (object);
  void (*chain_up) (GObject *) =
      ((GObjectClass *) example_call_stream_parent_class)->constructed;
  static guint count = 0;
  TpBaseConnection *conn;
  TpDBusDaemon *dbus;
  gchar *object_path;
  TpCallStreamEndpoint *endpoint;

  if (chain_up != NULL)
    chain_up (object);

  conn = tp_base_call_stream_get_connection ((TpBaseCallStream *) self);
  dbus = tp_base_connection_get_dbus_daemon (conn);
  object_path = g_strdup_printf ("%s/Endpoint%d",
      tp_base_call_stream_get_object_path ((TpBaseCallStream *) self),
      count++);
  endpoint = tp_call_stream_endpoint_new (dbus, object_path,
      TP_STREAM_TRANSPORT_TYPE_RAW_UDP, FALSE);

  tp_base_media_call_stream_add_endpoint ((TpBaseMediaCallStream *) self,
      endpoint);

  if (self->priv->locally_requested)
    {
      example_call_stream_change_direction (self, TRUE, TRUE);
    }
  else
    {
      example_call_stream_receive_direction_request (self, TRUE, TRUE);
    }

  g_object_unref (endpoint);
  g_free (object_path);
}

static void
get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (object);

  switch (property_id)
    {
    case PROP_SIMULATION_DELAY:
      g_value_set_uint (value, self->priv->simulation_delay);
      break;

    case PROP_LOCALLY_REQUESTED:
      g_value_set_boolean (value, self->priv->locally_requested);
      break;

    case PROP_HANDLE:
      g_value_set_uint (value, self->priv->handle);
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
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (object);

  switch (property_id)
    {
    case PROP_SIMULATION_DELAY:
      self->priv->simulation_delay = g_value_get_uint (value);
      break;

    case PROP_LOCALLY_REQUESTED:
      self->priv->locally_requested = g_value_get_boolean (value);
      break;

    case PROP_HANDLE:
      self->priv->handle = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean stream_request_receiving (TpBaseCallStream *base,
    TpHandle contact,
    gboolean receive,
    GError **error);
static gboolean stream_set_sending (TpBaseCallStream *base,
    gboolean sending,
    GError **error);

static void
finalize (GObject *object)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (object);

  if (self->priv->agreed_delay_id != 0)
    g_source_remove (self->priv->agreed_delay_id);

  G_OBJECT_CLASS (example_call_stream_parent_class)->finalize (object);
}

static void
example_call_stream_class_init (ExampleCallStreamClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  TpBaseCallStreamClass *stream_class = (TpBaseCallStreamClass *) klass;
  GParamSpec *param_spec;

  g_type_class_add_private (klass, sizeof (ExampleCallStreamPrivate));

  object_class->constructed = constructed;
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->finalize = finalize;

  stream_class->request_receiving = stream_request_receiving;
  stream_class->set_sending = stream_set_sending;

  param_spec = g_param_spec_uint ("simulation-delay", "Simulation delay",
      "Delay between simulated network events",
      0, G_MAXUINT32, 1000,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SIMULATION_DELAY,
      param_spec);

  param_spec = g_param_spec_boolean ("locally-requested", "Locally requested?",
      "True if this channel was requested by the local user",
      FALSE,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_LOCALLY_REQUESTED,
      param_spec);

  param_spec = g_param_spec_uint ("handle", "Peer's TpHandle",
      "The handle with which this stream communicates or 0 if not applicable",
      0, G_MAXUINT32, 0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_HANDLE, param_spec);
}

void
example_call_stream_accept_proposed_direction (ExampleCallStream *self)
{
  TpBaseCallStream *base = (TpBaseCallStream *) self;
  TpSendingState state = tp_base_call_stream_get_local_sending_state (base);

  if (state != TP_SENDING_STATE_PENDING_SEND)
    return;

  tp_base_call_stream_update_local_sending_state (base,
      TP_SENDING_STATE_SENDING, 0, TP_CALL_STATE_CHANGE_REASON_UNKNOWN,
      "", "");
}

void
example_call_stream_simulate_contact_agreed_to_send (ExampleCallStream *self)
{
  TpBaseCallStream *base = (TpBaseCallStream *) self;
  TpSendingState state = tp_base_call_stream_get_remote_sending_state (base,
      self->priv->handle);

  if (state != TP_SENDING_STATE_PENDING_SEND)

  g_message ("%s: SIGNALLING: Sending to server: OK, I'll send you media",
      tp_base_call_stream_get_object_path (base));

  tp_base_call_stream_update_remote_sending_state ((TpBaseCallStream *) self,
    self->priv->handle, TP_SENDING_STATE_SENDING, 0,
    TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");
}

static gboolean
simulate_contact_agreed_to_send_cb (gpointer p)
{
  example_call_stream_simulate_contact_agreed_to_send (p);
  return FALSE;
}

static void
example_call_stream_change_direction (ExampleCallStream *self,
    gboolean want_to_send, gboolean want_to_receive)
{
  TpBaseCallStream *base = (TpBaseCallStream *) self;
  TpSendingState local_sending_state =
      tp_base_call_stream_get_local_sending_state (base);
  TpSendingState remote_sending_state =
      tp_base_call_stream_get_remote_sending_state (base, self->priv->handle);

  if (want_to_send)
    {
      if (local_sending_state != TP_SENDING_STATE_SENDING)
        {
          if (local_sending_state == TP_SENDING_STATE_PENDING_SEND)
            {
              g_message ("%s: SIGNALLING: send: I will now send you media",
                  tp_base_call_stream_get_object_path (base));
            }

          g_message ("%s: MEDIA: sending media to peer",
              tp_base_call_stream_get_object_path (base));

          tp_base_call_stream_update_local_sending_state (base,
              TP_SENDING_STATE_SENDING, 0, TP_CALL_STATE_CHANGE_REASON_UNKNOWN,
              "", "");
        }
    }
  else
    {
      if (local_sending_state == TP_SENDING_STATE_SENDING)
        {
          g_message ("%s: SIGNALLING: send: I will no longer send you media",
              tp_base_call_stream_get_object_path (base));
          g_message ("%s: MEDIA: no longer sending media to peer",
              tp_base_call_stream_get_object_path (base));

          tp_base_call_stream_update_local_sending_state (base,
              TP_SENDING_STATE_NONE, 0, TP_CALL_STATE_CHANGE_REASON_UNKNOWN,
              "", "");
        }
      else if (local_sending_state == TP_SENDING_STATE_PENDING_SEND)
        {
          g_message ("%s: SIGNALLING: send: refusing to send you media",
              tp_base_call_stream_get_object_path (base));

          tp_base_call_stream_update_local_sending_state (base,
              TP_SENDING_STATE_NONE, 0, TP_CALL_STATE_CHANGE_REASON_UNKNOWN,
              "", "");
        }
    }

  if (want_to_receive)
    {
      if (remote_sending_state == TP_SENDING_STATE_NONE)
        {
          g_message ("%s: SIGNALLING: send: send me media, please?",
              tp_base_call_stream_get_object_path (base));

          tp_base_call_stream_update_remote_sending_state (
              (TpBaseCallStream *) self,
              self->priv->handle, TP_SENDING_STATE_PENDING_SEND, 0,
              TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");

          if (self->priv->agreed_delay_id == 0)
            {
              self->priv->agreed_delay_id = g_timeout_add (
                  self->priv->simulation_delay,
                  simulate_contact_agreed_to_send_cb, self);
            }
        }
    }
  else
    {
      if (remote_sending_state != TP_SENDING_STATE_NONE)
        {
          g_message ("%s: SIGNALLING: send: Please stop sending me media",
              tp_base_call_stream_get_object_path (base));
          g_message ("%s: MEDIA: suppressing output of stream",
              tp_base_call_stream_get_object_path (base));

          tp_base_call_stream_update_remote_sending_state (
              (TpBaseCallStream *) self,
              self->priv->handle, TP_SENDING_STATE_NONE, 0,
              TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");
        }
    }
}

/* The remote user wants to change the direction of this stream according
 * to @local_send and @remote_send. Shall we let him? */
static void
example_call_stream_receive_direction_request (ExampleCallStream *self,
    gboolean local_send,
    gboolean remote_send)
{
  TpBaseCallStream *base = (TpBaseCallStream *) self;
  TpSendingState local_sending_state =
      tp_base_call_stream_get_local_sending_state (base);
  TpSendingState remote_sending_state =
      tp_base_call_stream_get_remote_sending_state (base, self->priv->handle);

  /* In some protocols, streams cannot be neither sending nor receiving, so
   * if a stream is set to TP_MEDIA_STREAM_DIRECTION_NONE, this is equivalent
   * to removing it. (This is true in XMPP, for instance.)
   *
   * However, for this example we'll emulate a protocol where streams can be
   * directionless.
   */

  if (local_send)
    {
      g_message ("%s: SIGNALLING: send: Please start sending me media",
          tp_base_call_stream_get_object_path (base));

      if (local_sending_state == TP_SENDING_STATE_NONE)
        {
          /* ask the user for permission */
          tp_base_call_stream_update_local_sending_state (base,
              TP_SENDING_STATE_PENDING_SEND, 0,
              TP_CALL_STATE_CHANGE_REASON_UNKNOWN,
              "", "");
        }
      else
        {
          /* nothing to do, we're already sending (or asking the user for
           * permission to do so) on that stream */
        }
    }
  else
    {
      g_message ("%s: SIGNALLING: receive: Please stop sending me media",
          tp_base_call_stream_get_object_path (base));
      g_message ("%s: SIGNALLING: reply: OK!",
          tp_base_call_stream_get_object_path (base));

      if (local_sending_state == TP_SENDING_STATE_SENDING)
        {
          g_message ("%s: MEDIA: no longer sending media to peer",
              tp_base_call_stream_get_object_path (base));

          tp_base_call_stream_update_local_sending_state (base,
              TP_SENDING_STATE_NONE, 0,
              TP_CALL_STATE_CHANGE_REASON_UNKNOWN,
              "", "");
        }
      else if (local_sending_state == TP_SENDING_STATE_PENDING_SEND)
        {
          tp_base_call_stream_update_local_sending_state (base,
              TP_SENDING_STATE_NONE, 0,
              TP_CALL_STATE_CHANGE_REASON_UNKNOWN,
              "", "");
        }
      else
        {
          /* nothing to do, we're not sending on that stream anyway */
        }
    }

  if (remote_send)
    {
      g_message ("%s: SIGNALLING: receive: I will now send you media",
          tp_base_call_stream_get_object_path (base));

      if (remote_sending_state != TP_SENDING_STATE_SENDING)
        {
          tp_base_call_stream_update_remote_sending_state (
              (TpBaseCallStream *) self,
              self->priv->handle, TP_SENDING_STATE_SENDING, 0,
              TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");
        }
    }
  else
    {
      if (remote_sending_state == TP_SENDING_STATE_PENDING_SEND)
        {
          g_message ("%s: SIGNALLING: receive: No, I refuse to send you media",
              tp_base_call_stream_get_object_path (base));

          tp_base_call_stream_update_remote_sending_state (
              (TpBaseCallStream *) self,
              self->priv->handle, TP_SENDING_STATE_NONE, 0,
              TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");
        }
      else if (remote_sending_state == TP_SENDING_STATE_SENDING)
        {
          g_message ("%s: SIGNALLING: receive: I will no longer send media",
              tp_base_call_stream_get_object_path (base));

          tp_base_call_stream_update_remote_sending_state (
              (TpBaseCallStream *) self,
              self->priv->handle, TP_SENDING_STATE_NONE, 0,
              TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");
        }
    }
}

static gboolean
stream_set_sending (TpBaseCallStream *base,
    gboolean sending,
    GError **error)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (base);
  TpSendingState remote_sending_state =
      tp_base_call_stream_get_remote_sending_state (base, self->priv->handle);

  example_call_stream_change_direction (self, sending,
      (remote_sending_state == TP_SENDING_STATE_SENDING));

  return TRUE;
}

static gboolean
stream_request_receiving (TpBaseCallStream *base,
    TpHandle contact,
    gboolean receive,
    GError **error)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (base);
  TpSendingState local_sending_state =
      tp_base_call_stream_get_local_sending_state (base);

  /* This is the only member */
  g_assert (contact == self->priv->handle);

  example_call_stream_change_direction (self,
      (local_sending_state == TP_SENDING_STATE_SENDING),
      receive);

  return TRUE;
}
