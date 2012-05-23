/*
 * media-manager.c - an example channel manager for StreamedMedia calls.
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

#include "media-manager.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/channel-manager.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>
#include <telepathy-glib/interfaces.h>

#include "media-channel.h"

static void channel_manager_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (ExampleCallableMediaManager,
    example_callable_media_manager,
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

struct _ExampleCallableMediaManagerPrivate
{
  TpBaseConnection *conn;
  guint simulation_delay;

  /* Map from reffed ExampleCallableMediaChannel to the same pointer; used as a
   * set.
   */
  GHashTable *channels;

  /* Next channel will be ("MediaChannel%u", next_channel_index) */
  guint next_channel_index;

  gulong status_changed_id;
  gulong available_id;
};

static void
example_callable_media_manager_init (ExampleCallableMediaManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALLABLE_MEDIA_MANAGER,
      ExampleCallableMediaManagerPrivate);

  self->priv->conn = NULL;
  self->priv->channels = g_hash_table_new_full (NULL, NULL, g_object_unref,
      NULL);
  self->priv->status_changed_id = 0;
  self->priv->available_id = 0;
}

static void
example_callable_media_manager_close_all (ExampleCallableMediaManager *self)
{
  if (self->priv->channels != NULL)
    {
      GHashTable *tmp = self->priv->channels;

      self->priv->channels = NULL;

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
  ExampleCallableMediaManager *self = EXAMPLE_CALLABLE_MEDIA_MANAGER (object);

  example_callable_media_manager_close_all (self);
  g_assert (self->priv->channels == NULL);

  ((GObjectClass *) example_callable_media_manager_parent_class)->dispose (
    object);
}

static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  ExampleCallableMediaManager *self = EXAMPLE_CALLABLE_MEDIA_MANAGER (object);

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
  ExampleCallableMediaManager *self = EXAMPLE_CALLABLE_MEDIA_MANAGER (object);

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
                   ExampleCallableMediaManager *self)
{
  switch (status)
    {
    case TP_CONNECTION_STATUS_DISCONNECTED:
        {
          example_callable_media_manager_close_all (self);
        }
      break;

    default:
      break;
    }
}

static ExampleCallableMediaChannel *new_channel (
    ExampleCallableMediaManager *self, TpHandle handle, TpHandle initiator,
    gpointer request_token, gboolean initial_audio, gboolean initial_video);

static gboolean
simulate_incoming_call_cb (gpointer p)
{
  ExampleCallableMediaManager *self = p;
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
              ExampleCallableMediaManager *self)
{
  g_timeout_add_full (G_PRIORITY_DEFAULT, self->priv->simulation_delay,
      simulate_incoming_call_cb, g_object_ref (self), g_object_unref);
}

static void
constructed (GObject *object)
{
  ExampleCallableMediaManager *self = EXAMPLE_CALLABLE_MEDIA_MANAGER (object);
  void (*chain_up) (GObject *) =
      ((GObjectClass *) example_callable_media_manager_parent_class)->constructed;

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
example_callable_media_manager_class_init (
    ExampleCallableMediaManagerClass *klass)
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
      sizeof (ExampleCallableMediaManagerPrivate));
}

static void
example_callable_media_manager_foreach_channel (
    TpChannelManager *iface,
    TpExportableChannelFunc callback,
    gpointer user_data)
{
  ExampleCallableMediaManager *self = EXAMPLE_CALLABLE_MEDIA_MANAGER (iface);
  GHashTableIter iter;
  gpointer chan;

  g_hash_table_iter_init (&iter, self->priv->channels);

  while (g_hash_table_iter_next (&iter, &chan, NULL))
    callback (chan, user_data);
}

static void
channel_closed_cb (ExampleCallableMediaChannel *chan,
                   ExampleCallableMediaManager *self)
{
  tp_channel_manager_emit_channel_closed_for_object (self,
      TP_EXPORTABLE_CHANNEL (chan));

  if (self->priv->channels != NULL)
    g_hash_table_remove (self->priv->channels, chan);
}

static ExampleCallableMediaChannel *
new_channel (ExampleCallableMediaManager *self,
             TpHandle handle,
             TpHandle initiator,
             gpointer request_token,
             gboolean initial_audio,
             gboolean initial_video)
{
  ExampleCallableMediaChannel *chan;
  gchar *object_path;
  GSList *requests = NULL;

  /* FIXME: This could potentially wrap around, but only after 4 billion
   * calls, which is probably plenty. */
  object_path = g_strdup_printf ("%s/MediaChannel%u",
      self->priv->conn->object_path, self->priv->next_channel_index++);

  chan = g_object_new (EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL,
      "connection", self->priv->conn,
      "object-path", object_path,
      "handle", handle,
      "initiator-handle", initiator,
      "requested", (self->priv->conn->self_handle == initiator),
      "simulation-delay", self->priv->simulation_delay,
      "initial-audio", initial_audio,
      "initial-video", initial_video,
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

static const gchar * const fixed_properties[] = {
    TP_PROP_CHANNEL_CHANNEL_TYPE,
    TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
    NULL
};

static const gchar * const allowed_properties[] = {
    TP_PROP_CHANNEL_TARGET_HANDLE,
    TP_PROP_CHANNEL_TARGET_ID,
    NULL
};

static void
example_callable_media_manager_foreach_channel_class (
    TpChannelManager *manager,
    TpChannelManagerChannelClassFunc func,
    gpointer user_data)
{
  GHashTable *table = tp_asv_new (
      TP_PROP_CHANNEL_CHANNEL_TYPE,
          G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
      NULL);

  func (manager, table, allowed_properties, user_data);

  g_hash_table_destroy (table);
}

static gboolean
example_callable_media_manager_request (ExampleCallableMediaManager *self,
                                        gpointer request_token,
                                        GHashTable *request_properties,
                                        gboolean require_new)
{
  TpHandle handle;
  GError *error = NULL;

  if (tp_strdiff (tp_asv_get_string (request_properties,
          TP_PROP_CHANNEL_CHANNEL_TYPE),
      TP_IFACE_CHANNEL_TYPE_STREAMED_MEDIA))
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

  if (tp_channel_manager_asv_has_unknown_properties (request_properties,
        fixed_properties, allowed_properties, &error))
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

  new_channel (self, handle, self->priv->conn->self_handle,
      request_token, FALSE, FALSE);
  return TRUE;

error:
  tp_channel_manager_emit_request_failed (self, request_token,
      error->domain, error->code, error->message);
  g_error_free (error);
  return TRUE;
}

static gboolean
example_callable_media_manager_create_channel (TpChannelManager *manager,
                                               gpointer request_token,
                                               GHashTable *request_properties)
{
    return example_callable_media_manager_request (
        EXAMPLE_CALLABLE_MEDIA_MANAGER (manager),
        request_token, request_properties, TRUE);
}

static gboolean
example_callable_media_manager_ensure_channel (TpChannelManager *manager,
                                               gpointer request_token,
                                               GHashTable *request_properties)
{
    return example_callable_media_manager_request (
        EXAMPLE_CALLABLE_MEDIA_MANAGER (manager),
        request_token, request_properties, FALSE);
}

static void
channel_manager_iface_init (gpointer g_iface,
                            gpointer iface_data G_GNUC_UNUSED)
{
  TpChannelManagerIface *iface = g_iface;

  iface->foreach_channel = example_callable_media_manager_foreach_channel;
  iface->foreach_channel_class =
    example_callable_media_manager_foreach_channel_class;
  iface->create_channel = example_callable_media_manager_create_channel;
  iface->ensure_channel = example_callable_media_manager_ensure_channel;
  /* In this channel manager, RequestChannel is not supported (it's new
   * code so there's no reason to be backwards compatible). The requirements
   * for RequestChannel are somewhat complicated for backwards compatibility
   * reasons: see telepathy-gabble or
   * http://telepathy.freedesktop.org/wiki/Requesting%20StreamedMedia%20channels
   * for the gory details. */
  iface->request_channel = NULL;
}
