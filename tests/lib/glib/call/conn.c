/*
 * conn.c - an example connection
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

#include "conn.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/handle-repo-static.h>

#include "call-manager.h"
#include "protocol.h"

G_DEFINE_TYPE_WITH_CODE (ExampleCallConnection,
    example_call_connection,
    TP_TYPE_BASE_CONNECTION,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACTS,
      tp_contacts_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_PRESENCE,
      tp_presence_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      tp_presence_mixin_simple_presence_iface_init))

enum
{
  PROP_ACCOUNT = 1,
  PROP_SIMULATION_DELAY,
  N_PROPS
};

enum
{
  SIGNAL_AVAILABLE,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

struct _ExampleCallConnectionPrivate
{
  gchar *account;
  guint simulation_delay;
  gboolean away;
  gchar *presence_message;
};

static void
example_call_connection_init (ExampleCallConnection *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALL_CONNECTION,
      ExampleCallConnectionPrivate);
  self->priv->away = FALSE;
  self->priv->presence_message = g_strdup ("");
}

static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *spec)
{
  ExampleCallConnection *self = EXAMPLE_CALL_CONNECTION (object);

  switch (property_id)
    {
    case PROP_ACCOUNT:
      g_value_set_string (value, self->priv->account);
      break;

    case PROP_SIMULATION_DELAY:
      g_value_set_uint (value, self->priv->simulation_delay);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
    }
}

static void
set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *spec)
{
  ExampleCallConnection *self = EXAMPLE_CALL_CONNECTION (object);

  switch (property_id)
    {
    case PROP_ACCOUNT:
      g_free (self->priv->account);
      self->priv->account = g_value_dup_string (value);
      break;

    case PROP_SIMULATION_DELAY:
      self->priv->simulation_delay = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
    }
}

static void
finalize (GObject *object)
{
  ExampleCallConnection *self = EXAMPLE_CALL_CONNECTION (object);

  tp_contacts_mixin_finalize (object);
  g_free (self->priv->account);
  g_free (self->priv->presence_message);

  G_OBJECT_CLASS (example_call_connection_parent_class)->finalize (object);
}

static gchar *
get_unique_connection_name (TpBaseConnection *conn)
{
  ExampleCallConnection *self = EXAMPLE_CALL_CONNECTION (conn);

  return g_strdup_printf ("%s@%p", self->priv->account, self);
}

gchar *
example_call_normalize_contact (TpHandleRepoIface *repo,
    const gchar *id,
    gpointer context,
    GError **error)
{
  gchar *normal = NULL;

  if (example_call_protocol_check_contact_id (id, &normal, error))
    return normal;
  else
    return NULL;
}

static void
create_handle_repos (TpBaseConnection *conn,
    TpHandleRepoIface *repos[NUM_TP_HANDLE_TYPES])
{
  repos[TP_HANDLE_TYPE_CONTACT] = tp_dynamic_handle_repo_new
      (TP_HANDLE_TYPE_CONTACT, example_call_normalize_contact, NULL);
}

static GPtrArray *
create_channel_managers (TpBaseConnection *conn)
{
  ExampleCallConnection *self = EXAMPLE_CALL_CONNECTION (conn);
  GPtrArray *ret = g_ptr_array_sized_new (1);

  g_ptr_array_add (ret,
      g_object_new (EXAMPLE_TYPE_CALL_MANAGER,
        "connection", conn,
        "simulation-delay", self->priv->simulation_delay,
        NULL));

  return ret;
}

static gboolean
start_connecting (TpBaseConnection *conn,
    GError **error)
{
  ExampleCallConnection *self = EXAMPLE_CALL_CONNECTION (conn);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (conn,
      TP_HANDLE_TYPE_CONTACT);

  /* In a real connection manager we'd ask the underlying implementation to
   * start connecting, then go to state CONNECTED when finished, but here
   * we can do it immediately. */

  conn->self_handle = tp_handle_ensure (contact_repo, self->priv->account,
      NULL, error);

  if (conn->self_handle == 0)
    return FALSE;

  tp_base_connection_change_status (conn, TP_CONNECTION_STATUS_CONNECTED,
      TP_CONNECTION_STATUS_REASON_REQUESTED);

  return TRUE;
}

static void
shut_down (TpBaseConnection *conn)
{
  /* In a real connection manager we'd ask the underlying implementation to
   * start shutting down, then call this function when finished, but here
   * we can do it immediately. */
  tp_base_connection_finish_shutdown (conn);
}

static void
constructed (GObject *object)
{
  TpBaseConnection *base = TP_BASE_CONNECTION (object);
  void (*chain_up) (GObject *) =
    G_OBJECT_CLASS (example_call_connection_parent_class)->constructed;

  if (chain_up != NULL)
    chain_up (object);

  tp_contacts_mixin_init (object,
      G_STRUCT_OFFSET (ExampleCallConnection, contacts_mixin));
  tp_base_connection_register_with_contacts_mixin (base);

  tp_presence_mixin_init (object,
      G_STRUCT_OFFSET (ExampleCallConnection, presence_mixin));
  tp_presence_mixin_simple_presence_register_with_contacts_mixin (object);
}

static gboolean
status_available (GObject *object,
    guint index_)
{
  TpBaseConnection *base = TP_BASE_CONNECTION (object);

  if (base->status != TP_CONNECTION_STATUS_CONNECTED)
    return FALSE;

  return TRUE;
}

static GHashTable *
get_contact_statuses (GObject *object,
    const GArray *contacts,
    GError **error)
{
  ExampleCallConnection *self =
    EXAMPLE_CALL_CONNECTION (object);
  TpBaseConnection *base = TP_BASE_CONNECTION (object);
  guint i;
  GHashTable *result = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) tp_presence_status_free);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle contact = g_array_index (contacts, guint, i);
      ExampleCallPresence presence;
      GHashTable *parameters;

      parameters = g_hash_table_new_full (g_str_hash,
          g_str_equal, NULL, (GDestroyNotify) tp_g_value_slice_free);

      /* we know our own status from the connection; for this example CM,
       * everyone else's status is assumed to be "available" */
      if (contact == base->self_handle)
        {
          presence = (self->priv->away ? EXAMPLE_CALL_PRESENCE_AWAY
              : EXAMPLE_CALL_PRESENCE_AVAILABLE);

          if (self->priv->presence_message[0] != '\0')
            g_hash_table_insert (parameters, "message",
                tp_g_value_slice_new_string (self->priv->presence_message));
        }
      else
        {
          presence = EXAMPLE_CALL_PRESENCE_AVAILABLE;
        }

      g_hash_table_insert (result, GUINT_TO_POINTER (contact),
          tp_presence_status_new (presence, parameters));
      g_hash_table_unref (parameters);
    }

  return result;
}

static gboolean
set_own_status (GObject *object,
    const TpPresenceStatus *status,
    GError **error)
{
  ExampleCallConnection *self =
    EXAMPLE_CALL_CONNECTION (object);
  TpBaseConnection *base = TP_BASE_CONNECTION (object);
  GHashTable *presences;
  const gchar *message = "";

  if (status->optional_arguments != NULL)
    {
      GValue *v = g_hash_table_lookup (status->optional_arguments, "message");

      if (v != NULL && G_VALUE_HOLDS_STRING (v))
        {
          message = g_value_get_string (v);

          if (message == NULL)
            message = "";
        }
    }

  if (status->index == EXAMPLE_CALL_PRESENCE_AWAY)
    {
      if (self->priv->away && !tp_strdiff (message,
            self->priv->presence_message))
        return TRUE;

      self->priv->away = TRUE;
    }
  else
    {
      if (!self->priv->away && !tp_strdiff (message,
            self->priv->presence_message))
        return TRUE;

      self->priv->away = FALSE;
    }

  g_free (self->priv->presence_message);
  self->priv->presence_message = g_strdup (message);

  presences = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, NULL);
  g_hash_table_insert (presences, GUINT_TO_POINTER (base->self_handle),
      (gpointer) status);
  tp_presence_mixin_emit_presence_update (object, presences);
  g_hash_table_unref (presences);

  if (!self->priv->away)
    {
      g_signal_emit (self, signals[SIGNAL_AVAILABLE], 0, message);
    }

  return TRUE;
}

static const TpPresenceStatusOptionalArgumentSpec can_have_message[] = {
      { "message", "s", NULL, NULL },
      { NULL }
};

/* Must be kept in sync with ExampleCallPresence enum in header */
static const TpPresenceStatusSpec presence_statuses[] = {
      { "offline", TP_CONNECTION_PRESENCE_TYPE_OFFLINE, FALSE, NULL },
      { "unknown", TP_CONNECTION_PRESENCE_TYPE_UNKNOWN, FALSE, NULL },
      { "error", TP_CONNECTION_PRESENCE_TYPE_ERROR, FALSE, NULL },
      { "away", TP_CONNECTION_PRESENCE_TYPE_AWAY, TRUE, can_have_message },
      { "available", TP_CONNECTION_PRESENCE_TYPE_AVAILABLE, TRUE,
        can_have_message },
      { NULL }
};

static const gchar *interfaces_always_present[] = {
    TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
    TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
    TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
    TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
    NULL };

const gchar * const *
example_call_connection_get_possible_interfaces (void)
{
  /* in this example CM we don't have any extra interfaces that are sometimes,
   * but not always, present */
  return interfaces_always_present;
}

static void
example_call_connection_class_init (
    ExampleCallConnectionClass *klass)
{
  TpBaseConnectionClass *base_class = (TpBaseConnectionClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->constructed = constructed;
  object_class->finalize = finalize;
  g_type_class_add_private (klass,
      sizeof (ExampleCallConnectionPrivate));

  base_class->create_handle_repos = create_handle_repos;
  base_class->get_unique_connection_name = get_unique_connection_name;
  base_class->create_channel_managers = create_channel_managers;
  base_class->start_connecting = start_connecting;
  base_class->shut_down = shut_down;
  base_class->interfaces_always_present = interfaces_always_present;

  param_spec = g_param_spec_string ("account", "Account name",
      "The username of this user", NULL,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ACCOUNT, param_spec);

  param_spec = g_param_spec_uint ("simulation-delay", "Simulation delay",
      "Delay between simulated network events",
      0, G_MAXUINT32, 1000,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SIMULATION_DELAY,
      param_spec);

  /* Used in the call manager, to simulate an incoming call when we become
   * available */
  signals[SIGNAL_AVAILABLE] = g_signal_new ("available",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_VOID__STRING,
      G_TYPE_NONE, 1, G_TYPE_STRING);

  tp_contacts_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCallConnectionClass, contacts_mixin));
  tp_presence_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCallConnectionClass, presence_mixin),
      status_available, get_contact_statuses, set_own_status,
      presence_statuses);
  tp_presence_mixin_simple_presence_init_dbus_properties (object_class);
}
