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

#include "extensions/extensions.h"

static void stream_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (ExampleCallStream,
    example_call_stream,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (FUTURE_TYPE_SVC_CALL_STREAM, stream_iface_init))

enum
{
  PROP_OBJECT_PATH = 1,
  PROP_CONNECTION,
  PROP_HANDLE,
  PROP_SIMULATION_DELAY,
  PROP_LOCALLY_REQUESTED,
  PROP_SENDERS,
  N_PROPS
};

enum
{
  SIGNAL_REMOVED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

struct _ExampleCallStreamPrivate
{
  gchar *object_path;
  TpBaseConnection *conn;
  TpHandle handle;
  FutureSendingState local_sending_state;
  FutureSendingState remote_sending_state;

  guint simulation_delay;

  guint connected_event_id;

  gboolean locally_requested;
  gboolean removed;
};

static void
example_call_stream_init (ExampleCallStream *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALL_STREAM,
      ExampleCallStreamPrivate);

  /* start off directionless */
  self->priv->local_sending_state = FUTURE_SENDING_STATE_NONE;
  self->priv->remote_sending_state = FUTURE_SENDING_STATE_NONE;
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

  if (chain_up != NULL)
    chain_up (object);

  tp_dbus_daemon_register_object (
      tp_base_connection_get_dbus_daemon (self->priv->conn),
      self->priv->object_path, self);

  if (self->priv->locally_requested)
    {
      example_call_stream_change_direction (self, TRUE, TRUE);
    }
  else
    {
      example_call_stream_receive_direction_request (self, TRUE, TRUE);
    }

  if (self->priv->handle != 0)
    {
      TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
          self->priv->conn, TP_HANDLE_TYPE_CONTACT);

      tp_handle_ref (contact_repo, self->priv->handle);
    }
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
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;

    case PROP_HANDLE:
      g_value_set_uint (value, self->priv->handle);
      break;

    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->conn);
      break;

    case PROP_SIMULATION_DELAY:
      g_value_set_uint (value, self->priv->simulation_delay);
      break;

    case PROP_LOCALLY_REQUESTED:
      g_value_set_boolean (value, self->priv->locally_requested);
      break;

    case PROP_SENDERS:
        {
          GHashTable *senders = g_hash_table_new (NULL, NULL);

          g_hash_table_insert (senders, GUINT_TO_POINTER (self->priv->handle),
              GUINT_TO_POINTER (self->priv->remote_sending_state));

          g_hash_table_insert (senders,
              GUINT_TO_POINTER (tp_base_connection_get_self_handle (
                  self->priv->conn)),
              GUINT_TO_POINTER (self->priv->local_sending_state));

          g_value_take_boxed (value, senders);
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
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_assert (self->priv->object_path == NULL);   /* construct-only */
      self->priv->object_path = g_value_dup_string (value);
      break;

    case PROP_HANDLE:
      self->priv->handle = g_value_get_uint (value);
      break;

    case PROP_CONNECTION:
      g_assert (self->priv->conn == NULL);
      self->priv->conn = g_value_dup_object (value);
      break;

    case PROP_SIMULATION_DELAY:
      self->priv->simulation_delay = g_value_get_uint (value);
      break;

    case PROP_LOCALLY_REQUESTED:
      self->priv->locally_requested = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
dispose (GObject *object)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (object);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
      self->priv->conn, TP_HANDLE_TYPE_CONTACT);

  example_call_stream_close (self);

  if (self->priv->handle != 0)
    {
      tp_handle_unref (contact_repo, self->priv->handle);
      self->priv->handle = 0;
    }

  if (self->priv->conn != NULL)
    {
      g_object_unref (self->priv->conn);
      self->priv->conn = NULL;
    }

  ((GObjectClass *) example_call_stream_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (object);
  void (*chain_up) (GObject *) =
    ((GObjectClass *) example_call_stream_parent_class)->finalize;

  g_free (self->priv->object_path);

  if (chain_up != NULL)
    chain_up (object);
}

static void
example_call_stream_class_init (ExampleCallStreamClass *klass)
{
  static TpDBusPropertiesMixinPropImpl stream_props[] = {
      { "Senders", "senders", NULL },
      { NULL }
  };
  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
      { FUTURE_IFACE_CALL_STREAM,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        stream_props,
      },
      { NULL }
  };
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  g_type_class_add_private (klass,
      sizeof (ExampleCallStreamPrivate));

  object_class->constructed = constructed;
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->dispose = dispose;
  object_class->finalize = finalize;

  param_spec = g_param_spec_string ("object-path", "D-Bus object path",
      "The D-Bus object path used for this object on the bus.", NULL,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_OBJECT_PATH, param_spec);

  param_spec = g_param_spec_object ("connection", "TpBaseConnection",
      "Connection that (indirectly) owns this stream",
      TP_TYPE_BASE_CONNECTION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_uint ("handle", "Peer's TpHandle",
      "The handle with which this stream communicates or 0 if not applicable",
      0, G_MAXUINT32, 0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_HANDLE, param_spec);

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

  param_spec = g_param_spec_boxed ("senders", "Senders",
      "Map from contact handles to their sending states",
      FUTURE_HASH_TYPE_CONTACT_SENDING_STATE_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SENDERS, param_spec);

  signals[SIGNAL_REMOVED] = g_signal_new ("removed",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCallStreamClass,
        dbus_properties_class));
}

void
example_call_stream_close (ExampleCallStream *self)
{
  if (!self->priv->removed)
    {
      self->priv->removed = TRUE;

      g_message ("%s: Sending to server: Closing stream",
          self->priv->object_path);

      if (self->priv->connected_event_id != 0)
        {
          g_source_remove (self->priv->connected_event_id);
        }

      /* this has to come last, because the MediaChannel may unref us in
       * response to the removed signal */
      g_signal_emit (self, signals[SIGNAL_REMOVED], 0);
    }
}

void
example_call_stream_accept_proposed_direction (ExampleCallStream *self)
{
  GHashTable *updated_senders;
  GArray *removed_senders;

  if (self->priv->removed ||
      self->priv->local_sending_state != FUTURE_SENDING_STATE_PENDING_SEND)
    return;

  g_message ("%s: SIGNALLING: Sending to server: OK, I'll send you media",
      self->priv->object_path);

  self->priv->local_sending_state = FUTURE_SENDING_STATE_SENDING;

  updated_senders = g_hash_table_new (NULL, NULL);
  removed_senders = g_array_sized_new (FALSE, FALSE, sizeof (guint), 0);
  g_hash_table_insert (updated_senders,
      GUINT_TO_POINTER (tp_base_connection_get_self_handle (self->priv->conn)),
      GUINT_TO_POINTER (FUTURE_SENDING_STATE_SENDING));
  future_svc_call_stream_emit_senders_changed (self, updated_senders,
      removed_senders);
  g_hash_table_unref (updated_senders);
  g_array_free (removed_senders, TRUE);
}

void
example_call_stream_simulate_contact_agreed_to_send (ExampleCallStream *self)
{
  GHashTable *updated_senders;
  GArray *removed_senders;

  if (self->priv->removed ||
      self->priv->remote_sending_state != FUTURE_SENDING_STATE_PENDING_SEND)
    return;

  g_message ("%s: SIGNALLING: received: OK, I'll send you media",
      self->priv->object_path);

  self->priv->remote_sending_state = FUTURE_SENDING_STATE_SENDING;

  updated_senders = g_hash_table_new (NULL, NULL);
  removed_senders = g_array_sized_new (FALSE, FALSE, sizeof (guint), 0);
  g_hash_table_insert (updated_senders, GUINT_TO_POINTER (self->priv->handle),
      GUINT_TO_POINTER (FUTURE_SENDING_STATE_SENDING));
  future_svc_call_stream_emit_senders_changed (self, updated_senders,
      removed_senders);
  g_hash_table_unref (updated_senders);
  g_array_free (removed_senders, TRUE);
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
  GHashTable *updated_senders = g_hash_table_new (NULL, NULL);

  if (want_to_send)
    {
      if (self->priv->local_sending_state != FUTURE_SENDING_STATE_SENDING)
        {
          if (self->priv->local_sending_state ==
              FUTURE_SENDING_STATE_PENDING_SEND)
            {
              g_message ("%s: SIGNALLING: send: I will now send you media",
                  self->priv->object_path);
            }

          g_message ("%s: MEDIA: sending media to peer",
              self->priv->object_path);
          self->priv->local_sending_state = FUTURE_SENDING_STATE_SENDING;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (tp_base_connection_get_self_handle (
                  self->priv->conn)),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_SENDING));
        }
    }
  else
    {
      if (self->priv->local_sending_state == FUTURE_SENDING_STATE_SENDING)
        {
          g_message ("%s: SIGNALLING: send: I will no longer send you media",
              self->priv->object_path);
          g_message ("%s: MEDIA: no longer sending media to peer",
              self->priv->object_path);
          self->priv->local_sending_state = FUTURE_SENDING_STATE_NONE;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (tp_base_connection_get_self_handle (
                  self->priv->conn)),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_NONE));
        }
      else if (self->priv->local_sending_state ==
          FUTURE_SENDING_STATE_PENDING_SEND)
        {
          g_message ("%s: SIGNALLING: send: refusing to send you media",
              self->priv->object_path);
          self->priv->local_sending_state = FUTURE_SENDING_STATE_NONE;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (tp_base_connection_get_self_handle (
                  self->priv->conn)),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_NONE));
        }
    }

  if (want_to_receive)
    {
      if (self->priv->remote_sending_state == FUTURE_SENDING_STATE_NONE)
        {
          g_message ("%s: SIGNALLING: send: send me media, please?",
              self->priv->object_path);
          self->priv->remote_sending_state = FUTURE_SENDING_STATE_PENDING_SEND;
          g_timeout_add_full (G_PRIORITY_DEFAULT, self->priv->simulation_delay,
              simulate_contact_agreed_to_send_cb, g_object_ref (self),
              g_object_unref);

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (self->priv->handle),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_PENDING_SEND));
        }
    }
  else
    {
      if (self->priv->remote_sending_state != FUTURE_SENDING_STATE_NONE)
        {
          g_message ("%s: SIGNALLING: send: Please stop sending me media",
              self->priv->object_path);
          g_message ("%s: MEDIA: suppressing output of stream",
              self->priv->object_path);
          self->priv->remote_sending_state = FUTURE_SENDING_STATE_NONE;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (self->priv->handle),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_NONE));
        }
    }

  if (g_hash_table_size (updated_senders) != 0)
    {
      GArray *removed_senders = g_array_sized_new (FALSE, FALSE,
          sizeof (guint), 0);

      future_svc_call_stream_emit_senders_changed (self, updated_senders,
          removed_senders);

      g_array_free (removed_senders, TRUE);
    }

  g_hash_table_unref (updated_senders);
}

/* The remote user wants to change the direction of this stream according
 * to @local_send and @remote_send. Shall we let him? */
static void
example_call_stream_receive_direction_request (ExampleCallStream *self,
    gboolean local_send,
    gboolean remote_send)
{
  GHashTable *updated_senders = g_hash_table_new (NULL, NULL);

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
          self->priv->object_path);

      if (self->priv->local_sending_state == FUTURE_SENDING_STATE_NONE)
        {
          /* ask the user for permission */
          self->priv->local_sending_state = FUTURE_SENDING_STATE_PENDING_SEND;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (tp_base_connection_get_self_handle (
                  self->priv->conn)),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_PENDING_SEND));
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
          self->priv->object_path);
      g_message ("%s: SIGNALLING: reply: OK!",
          self->priv->object_path);

      if (self->priv->local_sending_state == FUTURE_SENDING_STATE_SENDING)
        {
          g_message ("%s: MEDIA: no longer sending media to peer",
              self->priv->object_path);
          self->priv->local_sending_state = FUTURE_SENDING_STATE_NONE;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (tp_base_connection_get_self_handle (
                  self->priv->conn)),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_NONE));
        }
      else if (self->priv->local_sending_state ==
          FUTURE_SENDING_STATE_PENDING_SEND)
        {
          self->priv->local_sending_state = FUTURE_SENDING_STATE_NONE;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (tp_base_connection_get_self_handle (
                  self->priv->conn)),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_NONE));
        }
      else
        {
          /* nothing to do, we're not sending on that stream anyway */
        }
    }

  if (remote_send)
    {
      g_message ("%s: SIGNALLING: receive: I will now send you media",
          self->priv->object_path);

      if (self->priv->remote_sending_state != FUTURE_SENDING_STATE_SENDING)
        {
          self->priv->remote_sending_state = FUTURE_SENDING_STATE_SENDING;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (self->priv->handle),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_SENDING));
        }
    }
  else
    {
      if (self->priv->remote_sending_state ==
          FUTURE_SENDING_STATE_PENDING_SEND)
        {
          g_message ("%s: SIGNALLING: receive: No, I refuse to send you media",
              self->priv->object_path);
          self->priv->remote_sending_state = FUTURE_SENDING_STATE_NONE;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (self->priv->handle),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_NONE));
        }
      else if (self->priv->remote_sending_state ==
          FUTURE_SENDING_STATE_SENDING)
        {
          g_message ("%s: SIGNALLING: receive: I will no longer send media",
              self->priv->object_path);
          self->priv->remote_sending_state = FUTURE_SENDING_STATE_NONE;

          g_hash_table_insert (updated_senders,
              GUINT_TO_POINTER (self->priv->handle),
              GUINT_TO_POINTER (FUTURE_SENDING_STATE_NONE));
        }
    }

  if (g_hash_table_size (updated_senders) != 0)
    {
      GArray *removed_senders = g_array_sized_new (FALSE, FALSE,
          sizeof (guint), 0);

      future_svc_call_stream_emit_senders_changed (self, updated_senders,
          removed_senders);

      g_array_free (removed_senders, TRUE);
    }

  g_hash_table_unref (updated_senders);
}

static void
stream_set_sending (FutureSvcCallStream *iface G_GNUC_UNUSED,
    gboolean sending,
    DBusGMethodInvocation *context)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (iface);

  example_call_stream_change_direction (self, sending,
      (self->priv->remote_sending_state == FUTURE_SENDING_STATE_SENDING));

  future_svc_call_stream_return_from_set_sending (context);
}

static void
stream_request_receiving (FutureSvcCallStream *iface,
    TpHandle contact,
    gboolean receive,
    DBusGMethodInvocation *context)
{
  ExampleCallStream *self = EXAMPLE_CALL_STREAM (iface);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles
      (self->priv->conn, TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;

  if (!tp_handle_is_valid (contact_repo, contact, &error))
    {
      goto finally;
    }

  if (contact != self->priv->handle)
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "Can't receive from contact #%u: this stream only contains #%u",
          contact, self->priv->handle);
      goto finally;
    }

  example_call_stream_change_direction (self,
      (self->priv->local_sending_state == FUTURE_SENDING_STATE_SENDING),
      receive);

finally:
  if (error == NULL)
    {
      future_svc_call_stream_return_from_request_receiving (context);
    }
  else
    {
      dbus_g_method_return_error (context, error);
      g_error_free (error);
    }
}

static void
stream_iface_init (gpointer iface,
    gpointer data)
{
  FutureSvcCallStreamClass *klass = iface;

#define IMPLEMENT(x) \
  future_svc_call_stream_implement_##x (klass, stream_##x)
  IMPLEMENT (set_sending);
  IMPLEMENT (request_receiving);
#undef IMPLEMENT
}
