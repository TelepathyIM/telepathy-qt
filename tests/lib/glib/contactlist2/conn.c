/*
 * conn.c - an example connection
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "conn.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/handle-repo-static.h>

#include "contact-list.h"
#include "protocol.h"

static void init_aliasing (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (ExampleContactListConnection,
    example_contact_list_connection,
    TP_TYPE_BASE_CONNECTION,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_ALIASING,
      init_aliasing);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACTS,
      tp_contacts_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_LIST,
      tp_base_contact_list_mixin_list_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_GROUPS,
      tp_base_contact_list_mixin_groups_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_BLOCKING,
      tp_base_contact_list_mixin_blocking_iface_init);
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

struct _ExampleContactListConnectionPrivate
{
  gchar *account;
  guint simulation_delay;
  ExampleContactList *contact_list;
  gboolean away;
};

static void
example_contact_list_connection_init (ExampleContactListConnection *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CONTACT_LIST_CONNECTION,
      ExampleContactListConnectionPrivate);
}

static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *spec)
{
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (object);

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
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (object);

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
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (object);

  tp_contacts_mixin_finalize (object);
  g_free (self->priv->account);

  G_OBJECT_CLASS (example_contact_list_connection_parent_class)->finalize (
      object);
}

static gchar *
get_unique_connection_name (TpBaseConnection *conn)
{
  ExampleContactListConnection *self = EXAMPLE_CONTACT_LIST_CONNECTION (conn);

  return g_strdup_printf ("%s@%p", self->priv->account, self);
}

gchar *
example_contact_list_normalize_contact (TpHandleRepoIface *repo,
                                        const gchar *id,
                                        gpointer context,
                                        GError **error)
{
  gchar *normal = NULL;

  if (example_contact_list_protocol_check_contact_id (id, &normal, error))
    return normal;
  else
    return NULL;
}

static void
create_handle_repos (TpBaseConnection *conn,
                     TpHandleRepoIface *repos[NUM_TP_HANDLE_TYPES])
{
  repos[TP_HANDLE_TYPE_CONTACT] = tp_dynamic_handle_repo_new
      (TP_HANDLE_TYPE_CONTACT, example_contact_list_normalize_contact, NULL);
}

static void
alias_updated_cb (ExampleContactList *contact_list,
                  TpHandle contact,
                  ExampleContactListConnection *self)
{
  GPtrArray *aliases;
  GValueArray *pair;

  pair = g_value_array_new (2);
  g_value_array_append (pair, NULL);
  g_value_array_append (pair, NULL);
  g_value_init (pair->values + 0, G_TYPE_UINT);
  g_value_init (pair->values + 1, G_TYPE_STRING);
  g_value_set_uint (pair->values + 0, contact);
  g_value_set_string (pair->values + 1,
      example_contact_list_get_alias (contact_list, contact));

  aliases = g_ptr_array_sized_new (1);
  g_ptr_array_add (aliases, pair);

  tp_svc_connection_interface_aliasing_emit_aliases_changed (self, aliases);

  g_ptr_array_free (aliases, TRUE);
  g_value_array_free (pair);
}

static void
presence_updated_cb (ExampleContactList *contact_list,
                     TpHandle contact,
                     ExampleContactListConnection *self)
{
  TpBaseConnection *base = (TpBaseConnection *) self;
  TpPresenceStatus *status;

  /* we ignore the presence indicated by the contact list for our own handle */
  if (contact == base->self_handle)
    return;

  status = tp_presence_status_new (
      example_contact_list_get_presence (contact_list, contact),
      NULL);
  tp_presence_mixin_emit_one_presence_update ((GObject *) self,
      contact, status);
  tp_presence_status_free (status);
}

static GPtrArray *
create_channel_managers (TpBaseConnection *conn)
{
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (conn);
  GPtrArray *ret = g_ptr_array_sized_new (1);

  self->priv->contact_list = EXAMPLE_CONTACT_LIST (g_object_new (
          EXAMPLE_TYPE_CONTACT_LIST,
          "connection", conn,
          "simulation-delay", self->priv->simulation_delay,
          NULL));

  g_signal_connect (self->priv->contact_list, "alias-updated",
      G_CALLBACK (alias_updated_cb), self);
  g_signal_connect (self->priv->contact_list, "presence-updated",
      G_CALLBACK (presence_updated_cb), self);

  g_ptr_array_add (ret, self->priv->contact_list);

  return ret;
}

static gboolean
start_connecting (TpBaseConnection *conn,
                  GError **error)
{
  ExampleContactListConnection *self = EXAMPLE_CONTACT_LIST_CONNECTION (conn);
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
aliasing_fill_contact_attributes (GObject *object,
                                  const GArray *contacts,
                                  GHashTable *attributes)
{
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (object);
  guint i;

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle contact = g_array_index (contacts, guint, i);

      tp_contacts_mixin_set_contact_attribute (attributes, contact,
          TP_TOKEN_CONNECTION_INTERFACE_ALIASING_ALIAS,
          tp_g_value_slice_new_string (
            example_contact_list_get_alias (self->priv->contact_list,
              contact)));
    }
}

static void
constructed (GObject *object)
{
  TpBaseConnection *base = TP_BASE_CONNECTION (object);
  void (*chain_up) (GObject *) =
    G_OBJECT_CLASS (example_contact_list_connection_parent_class)->constructed;

  if (chain_up != NULL)
    chain_up (object);

  tp_contacts_mixin_init (object,
      G_STRUCT_OFFSET (ExampleContactListConnection, contacts_mixin));

  tp_base_connection_register_with_contacts_mixin (base);
  tp_base_contact_list_mixin_register_with_contacts_mixin (base);

  tp_contacts_mixin_add_contact_attributes_iface (object,
      TP_IFACE_CONNECTION_INTERFACE_ALIASING,
      aliasing_fill_contact_attributes);

  tp_presence_mixin_init (object,
      G_STRUCT_OFFSET (ExampleContactListConnection, presence_mixin));
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
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (object);
  TpBaseConnection *base = TP_BASE_CONNECTION (object);
  guint i;
  GHashTable *result = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) tp_presence_status_free);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle contact = g_array_index (contacts, guint, i);
      ExampleContactListPresence presence;
      GHashTable *parameters;

      /* we get our own status from the connection, and everyone else's status
       * from the contact lists */
      if (contact == base->self_handle)
        {
          presence = (self->priv->away ? EXAMPLE_CONTACT_LIST_PRESENCE_AWAY
              : EXAMPLE_CONTACT_LIST_PRESENCE_AVAILABLE);
        }
      else
        {
          presence = example_contact_list_get_presence (
              self->priv->contact_list, contact);
        }

      parameters = g_hash_table_new_full (g_str_hash,
          g_str_equal, NULL, (GDestroyNotify) tp_g_value_slice_free);
      g_hash_table_insert (result, GUINT_TO_POINTER (contact),
          tp_presence_status_new (presence, parameters));
      g_hash_table_destroy (parameters);
    }

  return result;
}

static gboolean
set_own_status (GObject *object,
                const TpPresenceStatus *status,
                GError **error)
{
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (object);
  TpBaseConnection *base = TP_BASE_CONNECTION (object);
  GHashTable *presences;

  if (status->index == EXAMPLE_CONTACT_LIST_PRESENCE_AWAY)
    {
      if (self->priv->away)
        return TRUE;

      self->priv->away = TRUE;
    }
  else
    {
      if (!self->priv->away)
        return TRUE;

      self->priv->away = FALSE;
    }

  presences = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, NULL);
  g_hash_table_insert (presences, GUINT_TO_POINTER (base->self_handle),
      (gpointer) status);
  tp_presence_mixin_emit_presence_update (object, presences);
  g_hash_table_destroy (presences);
  return TRUE;
}

static const gchar *interfaces_always_present[] = {
    TP_IFACE_CONNECTION_INTERFACE_ALIASING,
    TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
    TP_IFACE_CONNECTION_INTERFACE_CONTACT_LIST,
    TP_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS,
    TP_IFACE_CONNECTION_INTERFACE_CONTACT_BLOCKING,
    TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
    TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
    TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
    NULL };

const gchar * const *
example_contact_list_connection_get_possible_interfaces (void)
{
  /* in this example CM we don't have any extra interfaces that are sometimes,
   * but not always, present */
  return interfaces_always_present;
}

static void
example_contact_list_connection_class_init (
    ExampleContactListConnectionClass *klass)
{
  TpBaseConnectionClass *base_class = (TpBaseConnectionClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->constructed = constructed;
  object_class->finalize = finalize;
  g_type_class_add_private (klass,
      sizeof (ExampleContactListConnectionPrivate));

  base_class->create_handle_repos = create_handle_repos;
  base_class->get_unique_connection_name = get_unique_connection_name;
  base_class->create_channel_managers = create_channel_managers;
  base_class->start_connecting = start_connecting;
  base_class->shut_down = shut_down;
  base_class->interfaces_always_present = interfaces_always_present;

  param_spec = g_param_spec_string ("account", "Account name",
      "The username of this user", NULL,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_ACCOUNT, param_spec);

  param_spec = g_param_spec_uint ("simulation-delay", "Simulation delay",
      "Delay between simulated network events",
      0, G_MAXUINT32, 1000,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SIMULATION_DELAY,
      param_spec);

  tp_contacts_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleContactListConnectionClass, contacts_mixin));

  tp_presence_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleContactListConnectionClass, presence_mixin),
      status_available, get_contact_statuses, set_own_status,
      example_contact_list_presence_statuses ());
  tp_presence_mixin_simple_presence_init_dbus_properties (object_class);

  tp_base_contact_list_mixin_class_init (base_class);
}

static void
get_alias_flags (TpSvcConnectionInterfaceAliasing *aliasing,
                 DBusGMethodInvocation *context)
{
  TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);

  TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);
  tp_svc_connection_interface_aliasing_return_from_get_alias_flags (context,
      TP_CONNECTION_ALIAS_FLAG_USER_SET);
}

static void
get_aliases (TpSvcConnectionInterfaceAliasing *aliasing,
             const GArray *contacts,
             DBusGMethodInvocation *context)
{
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (aliasing);
  TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GHashTable *result;
  GError *error = NULL;
  guint i;

  TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

  if (!tp_handles_are_valid (contact_repo, contacts, FALSE, &error))
    {
      dbus_g_method_return_error (context, error);
      g_error_free (error);
      return;
    }

  result = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle contact = g_array_index (contacts, TpHandle, i);
      const gchar *alias = example_contact_list_get_alias (
          self->priv->contact_list, contact);

      g_hash_table_insert (result, GUINT_TO_POINTER (contact),
          (gchar *) alias);
    }

  tp_svc_connection_interface_aliasing_return_from_get_aliases (context,
      result);
  g_hash_table_destroy (result);
}

static void
request_aliases (TpSvcConnectionInterfaceAliasing *aliasing,
                 const GArray *contacts,
                 DBusGMethodInvocation *context)
{
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (aliasing);
  TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GPtrArray *result;
  gchar **strings;
  GError *error = NULL;
  guint i;

  TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

  if (!tp_handles_are_valid (contact_repo, contacts, FALSE, &error))
    {
      dbus_g_method_return_error (context, error);
      g_error_free (error);
      return;
    }

  result = g_ptr_array_sized_new (contacts->len + 1);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle contact = g_array_index (contacts, TpHandle, i);
      const gchar *alias = example_contact_list_get_alias (
          self->priv->contact_list, contact);

      g_ptr_array_add (result, (gchar *) alias);
    }

  g_ptr_array_add (result, NULL);
  strings = (gchar **) g_ptr_array_free (result, FALSE);
  tp_svc_connection_interface_aliasing_return_from_request_aliases (context,
      (const gchar **) strings);
  g_free (strings);
}

static void
set_aliases (TpSvcConnectionInterfaceAliasing *aliasing,
             GHashTable *aliases,
             DBusGMethodInvocation *context)
{
  ExampleContactListConnection *self =
    EXAMPLE_CONTACT_LIST_CONNECTION (aliasing);
  TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, aliases);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      GError *error = NULL;

      if (!tp_handle_is_valid (contact_repo, GPOINTER_TO_UINT (key),
            &error))
        {
          dbus_g_method_return_error (context, error);
          g_error_free (error);
          return;
        }
    }

  g_hash_table_iter_init (&iter, aliases);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      example_contact_list_set_alias (self->priv->contact_list,
          GPOINTER_TO_UINT (key), value);
    }

  tp_svc_connection_interface_aliasing_return_from_set_aliases (context);
}

static void
init_aliasing (gpointer iface,
               gpointer iface_data G_GNUC_UNUSED)
{
  TpSvcConnectionInterfaceAliasingClass *klass = iface;

#define IMPLEMENT(x) tp_svc_connection_interface_aliasing_implement_##x (\
    klass, x)
  IMPLEMENT(get_alias_flags);
  IMPLEMENT(request_aliases);
  IMPLEMENT(get_aliases);
  IMPLEMENT(set_aliases);
#undef IMPLEMENT
}
