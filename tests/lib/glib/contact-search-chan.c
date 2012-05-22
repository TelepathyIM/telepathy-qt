/*
 * contact-search-channel.c - an tp_tests contact search channel
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

#include "contact-search-chan.h"

#include <string.h>

#include <gobject/gvaluecollector.h>

#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/svc-channel.h>
#include <telepathy-glib/svc-properties-interface.h>

static void contact_search_iface_init (gpointer iface, gpointer data);
static void channel_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (TpTestsContactSearchChannel,
    tp_tests_contact_search_channel,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_CONTACT_SEARCH,
      contact_search_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_GROUP,
      tp_group_mixin_iface_init);
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
  PROP_CONTACT_SEARCH_STATE,
  PROP_CONTACT_SEARCH_LIMIT,
  PROP_CONTACT_SEARCH_AVAILABLE_SEARCH_KEYS,
  PROP_CONTACT_SEARCH_SERVER,
  N_PROPS
};

typedef struct
{
  gchar *id;
  gchar *employer;
  GPtrArray *contact_info;
} TpTestsContactSearchContact;

struct _TpTestsContactSearchChannelPrivate
{
  TpBaseConnection *conn;
  gchar *object_path;

  guint contact_search_state;
  guint contact_search_limit;
  gchar **contact_search_available_search_keys;
  gchar *contact_search_server;

  GSList *contact_search_contacts;

  gboolean disposed;
  gboolean closed;
};

static const gchar * tp_tests_contact_search_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_GROUP,
    NULL
};

static void
tp_tests_contact_search_channel_init (TpTestsContactSearchChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL,
      TpTestsContactSearchChannelPrivate);
}

static TpTestsContactSearchContact *
new_contact (const gchar *id, const gchar *employer, const gchar *fn)
{
  TpTestsContactSearchContact *contact = g_new (TpTestsContactSearchContact, 1);
  GPtrArray *contact_info = dbus_g_type_specialized_construct (
        TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST);
  const gchar * const field_values[2] = { fn, NULL };

  contact->id = g_strdup (id);
  contact->employer = g_strdup (employer);

  g_ptr_array_add (contact_info, tp_value_array_build (3,
      G_TYPE_STRING, "fn",
      G_TYPE_STRV, NULL,
      G_TYPE_STRV, field_values,
      G_TYPE_INVALID));

  contact->contact_info = contact_info;

  return contact;
}

static void
free_contact (TpTestsContactSearchContact *contact)
{
  g_free (contact->id);
  g_free (contact->employer);
  g_boxed_free (TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST, contact->contact_info);
  g_free (contact);
}

static void
constructed (GObject *object)
{
  void (*chain_up) (GObject *) =
      ((GObjectClass *) tp_tests_contact_search_channel_parent_class)->constructed;
  TpTestsContactSearchChannel *self = TP_TESTS_CONTACT_SEARCH_CHANNEL (object);
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
      G_STRUCT_OFFSET (TpTestsContactSearchChannel, group),
      contact_repo, self->priv->conn->self_handle);

  self->priv->contact_search_state = TP_CHANNEL_CONTACT_SEARCH_STATE_NOT_STARTED;
  self->priv->contact_search_limit = 0;
  self->priv->contact_search_available_search_keys = g_new0 (gchar *, 2);
  self->priv->contact_search_available_search_keys[0] = g_strdup ("employer");
  self->priv->contact_search_server = g_strdup ("characters.shakespeare.lit");

  self->priv->contact_search_contacts = g_slist_append (self->priv->contact_search_contacts,
      new_contact ("oggis", "Collabora", "Olli Salli"));
  self->priv->contact_search_contacts = g_slist_append (self->priv->contact_search_contacts,
      new_contact ("andrunko", "Collabora", "Andre Moreira Magalhaes"));
  self->priv->contact_search_contacts = g_slist_append (self->priv->contact_search_contacts,
      new_contact ("wjt", "Collabora", "Will Thompson"));

  self->priv->contact_search_contacts = g_slist_append (self->priv->contact_search_contacts,
      new_contact ("foo", "Other Employer", "Foo"));
  self->priv->contact_search_contacts = g_slist_append (self->priv->contact_search_contacts,
      new_contact ("bar", "Other Employer", "Bar"));
}

static void
get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpTestsContactSearchChannel *self = TP_TESTS_CONTACT_SEARCH_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;

    case PROP_CHANNEL_TYPE:
      g_value_set_static_string (value, TP_IFACE_CHANNEL);
      break;

    case PROP_HANDLE_TYPE:
      g_value_set_uint (value, TP_HANDLE_TYPE_NONE);
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
      g_value_set_boxed (value, tp_tests_contact_search_channel_interfaces);
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
              TP_IFACE_CHANNEL_TYPE_CONTACT_SEARCH, "SearchState",
              TP_IFACE_CHANNEL_TYPE_CONTACT_SEARCH, "Limit",
              TP_IFACE_CHANNEL_TYPE_CONTACT_SEARCH, "AvailableSearchKeys",
              TP_IFACE_CHANNEL_TYPE_CONTACT_SEARCH, "Server",
              NULL));
      break;

    case PROP_CONTACT_SEARCH_STATE:
      g_value_set_uint (value, self->priv->contact_search_state);
      g_assert (G_VALUE_HOLDS (value, G_TYPE_UINT));
      break;

    case PROP_CONTACT_SEARCH_LIMIT:
      g_value_set_uint (value, self->priv->contact_search_limit);
      g_assert (G_VALUE_HOLDS (value, G_TYPE_UINT));
      break;

    case PROP_CONTACT_SEARCH_AVAILABLE_SEARCH_KEYS:
      g_value_set_boxed (value, self->priv->contact_search_available_search_keys);
      g_assert (G_VALUE_HOLDS (value, G_TYPE_STRV));
      break;

    case PROP_CONTACT_SEARCH_SERVER:
      g_value_set_string (value, self->priv->contact_search_server);
      g_assert (G_VALUE_HOLDS (value, G_TYPE_STRING));
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
  TpTestsContactSearchChannel *self = TP_TESTS_CONTACT_SEARCH_CHANNEL (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      self->priv->object_path = g_value_dup_string (value);
      break;

    case PROP_CONNECTION:
      self->priv->conn = g_value_get_object (value);
      break;

    case PROP_CHANNEL_TYPE:
    case PROP_HANDLE:
    case PROP_HANDLE_TYPE:
    case PROP_TARGET_ID:
    case PROP_REQUESTED:
    case PROP_INITIATOR_HANDLE:
    case PROP_INITIATOR_ID:
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
  TpTestsContactSearchChannel *self = TP_TESTS_CONTACT_SEARCH_CHANNEL (object);
  GSList *l;

  if (self->priv->disposed)
    {
      return;
    }

  self->priv->disposed = TRUE;

  g_strfreev (self->priv->contact_search_available_search_keys);
  self->priv->contact_search_available_search_keys = NULL;
  g_free (self->priv->contact_search_server);
  self->priv->contact_search_server = NULL;

  for (l = self->priv->contact_search_contacts; l != NULL; l = g_slist_next (l))
    {
      free_contact ((TpTestsContactSearchContact *) l->data);
    }
  g_slist_free (self->priv->contact_search_contacts);

  if (!self->priv->closed)
    {
      self->priv->closed = TRUE;
      tp_svc_channel_emit_closed (self);
    }

  ((GObjectClass *) tp_tests_contact_search_channel_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  TpTestsContactSearchChannel *self = TP_TESTS_CONTACT_SEARCH_CHANNEL (object);

  g_free (self->priv->object_path);

  tp_group_mixin_finalize (object);

  ((GObjectClass *) tp_tests_contact_search_channel_parent_class)->finalize (object);
}

static void
tp_tests_contact_search_channel_class_init (TpTestsContactSearchChannelClass *klass)
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
  static TpDBusPropertiesMixinPropImpl contact_search_props[] = {
      { "SearchState", "search-state", NULL },
      { "Limit", "limit", NULL },
      { "AvailableSearchKeys", "available-search-keys", NULL },
      { "Server", "server", NULL },
      { NULL }
  };
  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
      { TP_IFACE_CHANNEL,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        channel_props,
      },
      { TP_IFACE_CHANNEL_TYPE_CONTACT_SEARCH,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        contact_search_props,
      },
      { NULL }
  };
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  g_type_class_add_private (klass,
      sizeof (TpTestsContactSearchChannelPrivate));

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

  param_spec = g_param_spec_uint ("search-state", "Search state",
      "The search state",
      0, NUM_TP_CHANNEL_CONTACT_SEARCH_STATES, 0,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONTACT_SEARCH_STATE,
      param_spec);

  param_spec = g_param_spec_uint ("limit", "Search limit",
      "The search limit",
      0, G_MAXUINT32, 0,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONTACT_SEARCH_LIMIT,
      param_spec);

  param_spec = g_param_spec_boxed ("available-search-keys", "Available Search Keys",
      "The available search keys",
      G_TYPE_STRV,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONTACT_SEARCH_AVAILABLE_SEARCH_KEYS, param_spec);

  param_spec = g_param_spec_string ("server", "Server",
      "The search server",
      NULL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONTACT_SEARCH_SERVER,
      param_spec);

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (TpTestsContactSearchChannelClass,
        dbus_properties_class));

  tp_group_mixin_class_init (object_class,
      G_STRUCT_OFFSET (TpTestsContactSearchChannelClass, group_class),
      NULL, NULL);
  tp_group_mixin_init_dbus_properties (object_class);
}

static void
channel_close (TpSvcChannel *iface,
    DBusGMethodInvocation *context)
{
  TpTestsContactSearchChannel *self = TP_TESTS_CONTACT_SEARCH_CHANNEL (iface);

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
      TP_IFACE_CHANNEL_TYPE_CONTACT_SEARCH);
}

static void
channel_get_handle (TpSvcChannel *iface,
    DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_handle (context, TP_HANDLE_TYPE_NONE, 0);
}

static void
channel_get_interfaces (TpSvcChannel *iface G_GNUC_UNUSED,
    DBusGMethodInvocation *context)
{
  tp_svc_channel_return_from_get_interfaces (context,
      tp_tests_contact_search_channel_interfaces);
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
change_search_state (TpTestsContactSearchChannel *self,
                     guint state,
                     const gchar *debug_message)
{
  GHashTable *details = g_hash_table_new_full (g_str_hash, g_str_equal,
          NULL, (GDestroyNotify) tp_g_value_slice_free);

  g_hash_table_insert (details, "debug-message", tp_g_value_slice_new_string (debug_message));

  self->priv->contact_search_state = state;
  tp_svc_channel_type_contact_search_emit_search_state_changed (self,
      self->priv->contact_search_state, "", details);

  g_hash_table_destroy (details);
}

static gboolean
validate_terms (TpTestsContactSearchChannel *self,
                GHashTable *terms,
                GError **error)
{
  const gchar * const *asks =
      (const gchar * const *) self->priv->contact_search_available_search_keys;
  GHashTableIter iter;
  gpointer key;

  g_hash_table_iter_init (&iter, terms);

  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      gchar *field = key;

      if (!tp_strv_contains (asks, field))
        {
          g_debug ("%s is not in AvailableSearchKeys", field);
          g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
              "%s is not in AvailableSearchKeys", field);
          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
do_search (TpTestsContactSearchChannel *self,
           GHashTable *terms,
           GError **error)
{
  GHashTable *results = g_hash_table_new (g_str_hash, g_str_equal);
  GHashTableIter iter;
  gchar *key, *value;

  if (!validate_terms (self, terms, error))
    {
      return FALSE;
    }

  g_debug ("Doing search");
  change_search_state (self, TP_CHANNEL_CONTACT_SEARCH_STATE_IN_PROGRESS, "in progress");

  g_hash_table_iter_init (&iter, terms);
  while (g_hash_table_iter_next (&iter, (gpointer *) &key, (gpointer *) &value))
    {
      GSList *l;

      for (l = self->priv->contact_search_contacts; l != NULL; l = g_slist_next (l))
        {
          TpTestsContactSearchContact *contact = (TpTestsContactSearchContact *) l->data;
          if (strcmp (contact->employer, value) == 0)
            {
              g_hash_table_insert (results, contact->id, contact->contact_info);
            }
        }
    }

  tp_svc_channel_type_contact_search_emit_search_result_received (self,
      results);

  change_search_state (self, TP_CHANNEL_CONTACT_SEARCH_STATE_COMPLETED, "completed");

  g_hash_table_destroy (results);

  return TRUE;
}

static void
contact_search_search (TpSvcChannelTypeContactSearch *iface G_GNUC_UNUSED,
                       GHashTable *terms,
                       DBusGMethodInvocation *context)
{
  TpTestsContactSearchChannel *self = TP_TESTS_CONTACT_SEARCH_CHANNEL (iface);
  TpTestsContactSearchChannelPrivate *priv = self->priv;
  GError *error = NULL;

  if (priv->contact_search_state != TP_CHANNEL_CONTACT_SEARCH_STATE_NOT_STARTED)
    {
      g_debug ("Search state is %d", priv->contact_search_state);
      error = g_error_new (TP_ERROR, TP_ERROR_NOT_AVAILABLE,
          "SearchState is %d", priv->contact_search_state);
      goto err;
    }

  if (do_search (self, terms, &error))
    {
      tp_svc_channel_type_contact_search_return_from_search (context);
      return;
    }

err:
  dbus_g_method_return_error (context, error);
  g_error_free (error);
}

static void
contact_search_more (TpSvcChannelTypeContactSearch *iface G_GNUC_UNUSED,
                     DBusGMethodInvocation *context)
{
  tp_svc_channel_type_contact_search_return_from_more (context);
}

static void
contact_search_stop (TpSvcChannelTypeContactSearch *iface,
                     DBusGMethodInvocation *context)
{
  TpTestsContactSearchChannel *self = TP_TESTS_CONTACT_SEARCH_CHANNEL (iface);
  TpTestsContactSearchChannelPrivate *priv = self->priv;

  switch (priv->contact_search_state)
    {
      case TP_CHANNEL_CONTACT_SEARCH_STATE_IN_PROGRESS:
        change_search_state (self,
              TP_CHANNEL_CONTACT_SEARCH_STATE_FAILED, "stopped while in progress");
      case TP_CHANNEL_CONTACT_SEARCH_STATE_COMPLETED:
        tp_svc_channel_type_contact_search_return_from_stop (context);
        break;
      case TP_CHANNEL_CONTACT_SEARCH_STATE_NOT_STARTED:
        {
          GError e = { TP_ERROR, TP_ERROR_NOT_AVAILABLE,
              "Search() hasn't been called yet" };

          g_debug ("%s", e.message);
          dbus_g_method_return_error (context, &e);
          break;
        }
      case TP_CHANNEL_CONTACT_SEARCH_STATE_FAILED:
      case TP_CHANNEL_CONTACT_SEARCH_STATE_MORE_AVAILABLE:
        g_assert_not_reached ();
    }
}

static void
contact_search_iface_init (gpointer iface,
                           gpointer data)
{
  TpSvcChannelTypeContactSearchClass *klass = iface;

#define IMPLEMENT(x) tp_svc_channel_type_contact_search_implement_##x (klass, contact_search_##x)
  IMPLEMENT (search);
  IMPLEMENT (more);
  IMPLEMENT (stop);
#undef IMPLEMENT
}
