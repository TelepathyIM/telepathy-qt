/*
 * call-manager.c - an example channel manager for Call channels.
 *
 * This channel manager emulates a protocol like XMPP Jingle, where you can
 * make several simultaneous calls to the same or different contacts.
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

#include "call-manager.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/channel-manager.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>
#include <telepathy-glib/interfaces.h>

#include "call-channel.h"

static void channel_manager_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (ExampleCallManager,
    example_call_manager,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_MANAGER,
      channel_manager_iface_init))

/* type definition stuff */

enum
{
  PROP_CONNECTION = 1,
  PROP_SIMULATION_DELAY,
  N_PROPS
};

struct _ExampleCallManagerPrivate
{
  TpBaseConnection *conn;
  guint simulation_delay;

  /* Map from reffed ExampleCallChannel to the same pointer; used as a
   * set.
   */
  GHashTable *channels;

  /* Next channel will be ("CallChannel%u", next_channel_index) */
  guint next_channel_index;

  gulong status_changed_id;
  gulong available_id;
};

static void
example_call_manager_init (ExampleCallManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALL_MANAGER,
      ExampleCallManagerPrivate);

  self->priv->conn = NULL;
  self->priv->channels = g_hash_table_new_full (NULL, NULL, g_object_unref,
      NULL);
  self->priv->status_changed_id = 0;
  self->priv->available_id = 0;
}

static void
example_call_manager_close_all (ExampleCallManager *self)
{
  if (self->priv->channels != NULL)
    {
      GHashTable *tmp = self->priv->channels;
      GHashTableIter iter;
      gpointer v;

      self->priv->channels = NULL;

      g_hash_table_iter_init (&iter, tmp);

      while (g_hash_table_iter_next (&iter, NULL, &v))
        tp_base_channel_close (v);

      g_hash_table_unref (tmp);
    }

  if (self->priv->available_id != 0)
    {
      g_signal_handler_disconnect (self->priv->conn,
          self->priv->available_id);
      self->priv->available_id = 0;
    }

  if (self->priv->status_changed_id != 0)
    {
      g_signal_handler_disconnect (self->priv->conn,
          self->priv->status_changed_id);
      self->priv->status_changed_id = 0;
    }
}

static void
dispose (GObject *object)
{
  ExampleCallManager *self = EXAMPLE_CALL_MANAGER (object);

  example_call_manager_close_all (self);
  g_assert (self->priv->channels == NULL);

  ((GObjectClass *) example_call_manager_parent_class)->dispose (
    object);
}

static void
get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ExampleCallManager *self = EXAMPLE_CALL_MANAGER (object);

  switch (property_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->conn);
      break;

    case PROP_SIMULATION_DELAY:
      g_value_set_uint (value, self->priv->simulation_delay);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ExampleCallManager *self = EXAMPLE_CALL_MANAGER (object);

  switch (property_id)
    {
    case PROP_CONNECTION:
      /* We don't ref the connection, because it owns a reference to the
       * channel manager, and it guarantees that the manager's lifetime is
       * less than its lifetime */
      self->priv->conn = g_value_get_object (value);
      break;

    case PROP_SIMULATION_DELAY:
      self->priv->simulation_delay = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
status_changed_cb (TpBaseConnection *conn,
                   guint status,
                   guint reason,
                   ExampleCallManager *self)
{
  switch (status)
    {
    case TP_CONNECTION_STATUS_DISCONNECTED:
        {
          example_call_manager_close_all (self);
        }
      break;

    default:
      break;
    }
}

static ExampleCallChannel *new_channel (ExampleCallManager *self,
    TpHandle handle, TpHandle initiator, gpointer request_token,
    gboolean initial_audio, gboolean initial_video);

static gboolean
simulate_incoming_call_cb (gpointer p)
{
  ExampleCallManager *self = p;
  TpHandleRepoIface *contact_repo;
  TpHandle caller;

  /* do nothing if we've been disconnected while waiting for the contact to
   * call us */
  if (self->priv->available_id == 0)
    return FALSE;

  /* We're called by someone whose ID on the IM service is "caller" */
  contact_repo = tp_base_connection_get_handles (self->priv->conn,
      TP_HANDLE_TYPE_CONTACT);
  caller = tp_handle_ensure (contact_repo, "caller", NULL, NULL);

  new_channel (self, caller, caller, NULL, TRUE, FALSE);

  return FALSE;
}

/* Whenever our presence changes from away to available, and whenever our
 * presence message changes while remaining available, simulate a call from
 * a contact */
static void
available_cb (GObject *conn G_GNUC_UNUSED,
    const gchar *message,
    ExampleCallManager *self)
{
  g_timeout_add_full (G_PRIORITY_DEFAULT, self->priv->simulation_delay,
      simulate_incoming_call_cb, g_object_ref (self), g_object_unref);
}

static void
constructed (GObject *object)
{
  ExampleCallManager *self = EXAMPLE_CALL_MANAGER (object);
  void (*chain_up) (GObject *) =
      ((GObjectClass *) example_call_manager_parent_class)->constructed;

  if (chain_up != NULL)
    {
      chain_up (object);
    }

  self->priv->status_changed_id = g_signal_connect (self->priv->conn,
      "status-changed", (GCallback) status_changed_cb, self);

  self->priv->available_id = g_signal_connect (self->priv->conn,
      "available", (GCallback) available_cb, self);
}

static void
example_call_manager_class_init (ExampleCallManagerClass *klass)
{
  GParamSpec *param_spec;
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->constructed = constructed;
  object_class->dispose = dispose;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  param_spec = g_param_spec_object ("connection", "Connection object",
      "The connection that owns this channel manager",
      TP_TYPE_BASE_CONNECTION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_uint ("simulation-delay", "Simulation delay",
      "Delay between simulated network events",
      0, G_MAXUINT32, 1000,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SIMULATION_DELAY,
      param_spec);

  g_type_class_add_private (klass,
      sizeof (ExampleCallManagerPrivate));
}

static void
example_call_manager_foreach_channel (TpChannelManager *iface,
    TpExportableChannelFunc callback,
    gpointer user_data)
{
  ExampleCallManager *self = EXAMPLE_CALL_MANAGER (iface);
  GHashTableIter iter;
  gpointer chan;

  g_hash_table_iter_init (&iter, self->priv->channels);

  while (g_hash_table_iter_next (&iter, &chan, NULL))
    callback (chan, user_data);
}

static void
channel_closed_cb (ExampleCallChannel *chan,
    ExampleCallManager *self)
{
  tp_channel_manager_emit_channel_closed_for_object (self,
      TP_EXPORTABLE_CHANNEL (chan));

  if (self->priv->channels != NULL)
    g_hash_table_remove (self->priv->channels, chan);
}

static ExampleCallChannel *
new_channel (ExampleCallManager *self,
    TpHandle handle,
    TpHandle initiator,
    gpointer request_token,
    gboolean initial_audio,
    gboolean initial_video)
{
  ExampleCallChannel *chan;
  gchar *object_path;
  GSList *requests = NULL;

  /* FIXME: This could potentially wrap around, but only after 4 billion
   * calls, which is probably plenty. */
  object_path = g_strdup_printf ("%s/CallChannel%u",
      self->priv->conn->object_path, self->priv->next_channel_index++);

  chan = g_object_new (EXAMPLE_TYPE_CALL_CHANNEL,
      "connection", self->priv->conn,
      "object-path", object_path,
      "handle", handle,
      "initiator-handle", initiator,
      "requested", (self->priv->conn->self_handle == initiator),
      "simulation-delay", self->priv->simulation_delay,
      "initial-audio", initial_audio,
      "initial-video", initial_video,
      "mutable-contents", TRUE,
      NULL);

  g_free (object_path);

  g_signal_connect (chan, "closed", G_CALLBACK (channel_closed_cb), self);

  g_hash_table_insert (self->priv->channels, chan, chan);

  if (request_token != NULL)
    requests = g_slist_prepend (requests, request_token);

  tp_channel_manager_emit_new_channel (self, TP_EXPORTABLE_CHANNEL (chan),
      requests);
  g_slist_free (requests);

  return chan;
}

static const gchar * const audio_fixed_properties[] = {
    TP_PROP_CHANNEL_CHANNEL_TYPE,
    TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
    TP_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO,
    NULL
};

static const gchar * const video_fixed_properties[] = {
    TP_PROP_CHANNEL_CHANNEL_TYPE,
    TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
    TP_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO,
    NULL
};

static const gchar * const audio_allowed_properties[] = {
    TP_PROP_CHANNEL_TARGET_HANDLE,
    TP_PROP_CHANNEL_TARGET_ID,
    TP_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO,
    NULL
};

static const gchar * const video_allowed_properties[] = {
    TP_PROP_CHANNEL_TARGET_HANDLE,
    TP_PROP_CHANNEL_TARGET_ID,
    TP_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO,
    NULL
};

static void
example_call_manager_type_foreach_channel_class (GType type,
    TpChannelManagerTypeChannelClassFunc func,
    gpointer user_data)
{
  GHashTable *table = tp_asv_new (
      TP_PROP_CHANNEL_CHANNEL_TYPE,
          G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_CALL,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
    TP_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO, G_TYPE_BOOLEAN, TRUE,
      NULL);

  func (type, table, audio_allowed_properties, user_data);

  g_hash_table_remove (table, TP_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO);
  tp_asv_set_boolean (table, TP_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO,
      TRUE);

  func (type, table, video_allowed_properties, user_data);

  g_hash_table_unref (table);
}

static gboolean
example_call_manager_request (ExampleCallManager *self,
    gpointer request_token,
    GHashTable *request_properties,
    gboolean require_new)
{
  TpHandle handle;
  GError *error = NULL;
  gboolean initial_audio, initial_video;

  if (tp_strdiff (tp_asv_get_string (request_properties,
          TP_PROP_CHANNEL_CHANNEL_TYPE),
      TP_IFACE_CHANNEL_TYPE_CALL))
    {
      return FALSE;
    }

  if (tp_asv_get_uint32 (request_properties,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, NULL) != TP_HANDLE_TYPE_CONTACT)
    {
      return FALSE;
    }

  handle = tp_asv_get_uint32 (request_properties,
      TP_PROP_CHANNEL_TARGET_HANDLE, NULL);
  g_assert (handle != 0);

  initial_audio = tp_asv_get_boolean (request_properties,
      TP_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO, NULL);
  initial_video = tp_asv_get_boolean (request_properties,
        TP_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO, NULL);

  if (!initial_audio && !initial_video)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED,
          "Call channels must initially have either audio or video content");
      goto error;
    }

  /* the set of (fixed | allowed) properties is the same for audio and video,
   * so we only need to check with one set */
  if (tp_channel_manager_asv_has_unknown_properties (request_properties,
        audio_fixed_properties, audio_allowed_properties, &error))
    {
      goto error;
    }

  if (handle == self->priv->conn->self_handle)
    {
      /* In protocols with a concept of multiple "resources" signed in to
       * one account (XMPP, and possibly MSN) it is technically possible to
       * call yourself - e.g. if you're signed in on two PCs, you can call one
       * from the other. For simplicity, this example simulates a protocol
       * where this is not the case.
       */
      g_set_error (&error, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED,
          "In this protocol, you can't call yourself");
      goto error;
    }

  if (!require_new)
    {
      /* see if we're already calling that handle */
      GHashTableIter iter;
      gpointer chan;

      g_hash_table_iter_init (&iter, self->priv->channels);

      while (g_hash_table_iter_next (&iter, &chan, NULL))
        {
          guint its_handle;

          g_object_get (chan,
              "handle", &its_handle,
              NULL);

          if (its_handle == handle)
            {
              tp_channel_manager_emit_request_already_satisfied (self,
                  request_token, TP_EXPORTABLE_CHANNEL (chan));
              return TRUE;
            }
        }
    }

  new_channel (self, handle, self->priv->conn->self_handle, request_token,
      initial_audio, initial_video);
  return TRUE;

error:
  tp_channel_manager_emit_request_failed (self, request_token,
      error->domain, error->code, error->message);
  g_error_free (error);
  return TRUE;
}

static gboolean
example_call_manager_create_channel (TpChannelManager *manager,
    gpointer request_token,
    GHashTable *request_properties)
{
    return example_call_manager_request (
        EXAMPLE_CALL_MANAGER (manager),
        request_token, request_properties, TRUE);
}

static gboolean
example_call_manager_ensure_channel (TpChannelManager *manager,
    gpointer request_token,
    GHashTable *request_properties)
{
    return example_call_manager_request (
        EXAMPLE_CALL_MANAGER (manager),
        request_token, request_properties, FALSE);
}

static void
channel_manager_iface_init (gpointer g_iface,
    gpointer iface_data G_GNUC_UNUSED)
{
  TpChannelManagerIface *iface = g_iface;

  iface->foreach_channel = example_call_manager_foreach_channel;
  iface->type_foreach_channel_class =
    example_call_manager_type_foreach_channel_class;
  iface->create_channel = example_call_manager_create_channel;
  iface->ensure_channel = example_call_manager_ensure_channel;
  /* In this channel manager, RequestChannel is not supported; Call is not
   * designed to work with the old RequestChannel API. */
  iface->request_channel = NULL;
}
