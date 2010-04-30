/*
 * contacts-conn.c - connection with contact info
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */
#include "contacts-conn.h"

#include <dbus/dbus-glib.h>

#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/util.h>

#include "debug.h"

static void init_aliasing (gpointer, gpointer);
static void init_avatars (gpointer, gpointer);
static void init_location (gpointer, gpointer);
static void init_contact_caps (gpointer, gpointer);
static void init_contact_info (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (ContactsConnection,
    contacts_connection,
    SIMPLE_TYPE_CONNECTION,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_ALIASING,
      init_aliasing);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_AVATARS,
      init_avatars);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACTS,
      tp_contacts_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_PRESENCE,
      tp_presence_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      tp_presence_mixin_simple_presence_iface_init)
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_LOCATION,
      init_location)
    G_IMPLEMENT_INTERFACE (
      TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_CAPABILITIES,
      init_contact_caps)
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_INFO,
      init_contact_info)
    );

/* type definition stuff */

enum
{
  N_SIGNALS
};

struct _ContactsConnectionPrivate
{
  /* TpHandle => gchar * */
  GHashTable *aliases;
  /* TpHandle => gchar * */
  GHashTable *avatar_tokens;
  /* TpHandle => ContactsConnectionPresenceStatusIndex */
  GHashTable *presence_statuses;
  /* TpHandle => gchar * */
  GHashTable *presence_messages;
  /* TpHandle => GHashTable * */
  GHashTable *locations;
  /* TpHandle => GPtrArray * */
  GHashTable *capabilities;
  /* TpHandle => GPtrArray * */
  GHashTable *infos;
};

static void
free_rcc_list (GPtrArray *rccs)
{
  g_boxed_free (TP_ARRAY_TYPE_REQUESTABLE_CHANNEL_CLASS_LIST, rccs);
}

static void
contacts_connection_init (ContactsConnection *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, CONTACTS_TYPE_CONNECTION,
      ContactsConnectionPrivate);
  self->priv->aliases = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, g_free);
  self->priv->avatar_tokens = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, NULL, g_free);
  self->priv->presence_statuses = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, NULL, NULL);
  self->priv->presence_messages = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, NULL, g_free);
  self->priv->locations = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) g_hash_table_unref);
  self->priv->capabilities = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, NULL, (GDestroyNotify) free_rcc_list);
  self->priv->infos = dbus_g_type_specialized_construct (
      TP_HASH_TYPE_CONTACT_INFO_MAP);
}

static void
finalize (GObject *object)
{
  ContactsConnection *self = CONTACTS_CONNECTION (object);

  tp_contacts_mixin_finalize (object);
  g_hash_table_destroy (self->priv->aliases);
  g_hash_table_destroy (self->priv->avatar_tokens);
  g_hash_table_destroy (self->priv->presence_statuses);
  g_hash_table_destroy (self->priv->presence_messages);
  g_hash_table_destroy (self->priv->locations);
  g_hash_table_destroy (self->priv->capabilities);
  g_boxed_free (TP_HASH_TYPE_CONTACT_INFO_MAP, self->priv->infos);

  G_OBJECT_CLASS (contacts_connection_parent_class)->finalize (object);
}

static void
aliasing_fill_contact_attributes (GObject *object,
                                  const GArray *contacts,
                                  GHashTable *attributes)
{
  guint i;
  ContactsConnection *self = CONTACTS_CONNECTION (object);
  TpBaseConnection *base = TP_BASE_CONNECTION (object);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle handle = g_array_index (contacts, guint, i);
      const gchar *alias = g_hash_table_lookup (self->priv->aliases,
          GUINT_TO_POINTER (handle));

      if (alias == NULL)
        {
          alias = tp_handle_inspect (contact_repo, handle);
        }

      tp_contacts_mixin_set_contact_attribute (attributes, handle,
          TP_IFACE_CONNECTION_INTERFACE_ALIASING "/alias",
          tp_g_value_slice_new_string (alias));
    }
}

static void
avatars_fill_contact_attributes (GObject *object,
                                 const GArray *contacts,
                                 GHashTable *attributes)
{
  guint i;
  ContactsConnection *self = CONTACTS_CONNECTION (object);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle handle = g_array_index (contacts, guint, i);
      const gchar *token = g_hash_table_lookup (self->priv->avatar_tokens,
          GUINT_TO_POINTER (handle));

      if (token != NULL)
        {
          tp_contacts_mixin_set_contact_attribute (attributes, handle,
              TP_IFACE_CONNECTION_INTERFACE_AVATARS "/token",
              tp_g_value_slice_new_string (token));
        }
    }
}

static void
location_fill_contact_attributes (GObject *object,
    const GArray *contacts,
    GHashTable *attributes)
{
  guint i;
  ContactsConnection *self = CONTACTS_CONNECTION (object);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle handle = g_array_index (contacts, guint, i);
      GHashTable *location = g_hash_table_lookup (self->priv->locations,
          GUINT_TO_POINTER (handle));

      if (location != NULL)
        {
          tp_contacts_mixin_set_contact_attribute (attributes, handle,
              TP_IFACE_CONNECTION_INTERFACE_LOCATION "/location",
              tp_g_value_slice_new_boxed (TP_HASH_TYPE_LOCATION, location));
        }
    }
}

static void
contact_caps_fill_contact_attributes (GObject *object,
    const GArray *contacts,
    GHashTable *attributes)
{
  guint i;
  ContactsConnection *self = CONTACTS_CONNECTION (object);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle handle = g_array_index (contacts, guint, i);
      GPtrArray *caps = g_hash_table_lookup (self->priv->capabilities,
          GUINT_TO_POINTER (handle));

      if (caps != NULL)
        {
          tp_contacts_mixin_set_contact_attribute (attributes, handle,
              TP_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES "/capabilities",
              tp_g_value_slice_new_boxed (
                TP_ARRAY_TYPE_REQUESTABLE_CHANNEL_CLASS_LIST, caps));
        }
    }
}

static void
contact_info_fill_contact_attributes (GObject *object,
    const GArray *contacts,
    GHashTable *attributes)
{
  guint i;
  ContactsConnection *self = CONTACTS_CONNECTION (object);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle handle = g_array_index (contacts, guint, i);
      GPtrArray *info = g_hash_table_lookup (self->priv->infos,
          GUINT_TO_POINTER (handle));

      if (info != NULL)
        {
          tp_contacts_mixin_set_contact_attribute (attributes, handle,
              TP_IFACE_CONNECTION_INTERFACE_CONTACT_INFO "/info",
              tp_g_value_slice_new_boxed (TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST,
                  info));
        }
    }
}

static void
constructed (GObject *object)
{
  TpBaseConnection *base = TP_BASE_CONNECTION (object);
  void (*parent_impl) (GObject *) =
    G_OBJECT_CLASS (contacts_connection_parent_class)->constructed;

  if (parent_impl != NULL)
    parent_impl (object);

  tp_contacts_mixin_init (object,
      G_STRUCT_OFFSET (ContactsConnection, contacts_mixin));
  tp_base_connection_register_with_contacts_mixin (base);
  tp_contacts_mixin_add_contact_attributes_iface (object,
      TP_IFACE_CONNECTION_INTERFACE_ALIASING,
      aliasing_fill_contact_attributes);
  tp_contacts_mixin_add_contact_attributes_iface (object,
      TP_IFACE_CONNECTION_INTERFACE_AVATARS,
      avatars_fill_contact_attributes);
  tp_contacts_mixin_add_contact_attributes_iface (object,
      TP_IFACE_CONNECTION_INTERFACE_LOCATION,
      location_fill_contact_attributes);
  tp_contacts_mixin_add_contact_attributes_iface (object,
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES,
      contact_caps_fill_contact_attributes);
  tp_contacts_mixin_add_contact_attributes_iface (object,
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_INFO,
      contact_info_fill_contact_attributes);

  tp_presence_mixin_init (object,
      G_STRUCT_OFFSET (ContactsConnection, presence_mixin));
  tp_presence_mixin_simple_presence_register_with_contacts_mixin (object);
}

static const TpPresenceStatusOptionalArgumentSpec can_have_message[] = {
      { "message", "s", NULL, NULL },
      { NULL }
};

/* Must match ContactsConnectionPresenceStatusIndex in the .h */
static const TpPresenceStatusSpec my_statuses[] = {
      { "available", TP_CONNECTION_PRESENCE_TYPE_AVAILABLE, TRUE,
        can_have_message },
      { "busy", TP_CONNECTION_PRESENCE_TYPE_BUSY, TRUE, can_have_message },
      { "away", TP_CONNECTION_PRESENCE_TYPE_AWAY, TRUE, can_have_message },
      { "offline", TP_CONNECTION_PRESENCE_TYPE_OFFLINE, FALSE, NULL },
      { "unknown", TP_CONNECTION_PRESENCE_TYPE_UNKNOWN, FALSE, NULL },
      { "error", TP_CONNECTION_PRESENCE_TYPE_ERROR, FALSE, NULL },
      { NULL }
};

static gboolean
my_status_available (GObject *object,
                     guint index)
{
  TpBaseConnection *base = TP_BASE_CONNECTION (object);

  if (base->status != TP_CONNECTION_STATUS_CONNECTED)
    return FALSE;

  return TRUE;
}

static GHashTable *
my_get_contact_statuses (GObject *object,
                         const GArray *contacts,
                         GError **error)
{
  ContactsConnection *self = CONTACTS_CONNECTION (object);
  GHashTable *result = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) tp_presence_status_free);
  guint i;

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle handle = g_array_index (contacts, TpHandle, i);
      gpointer key = GUINT_TO_POINTER (handle);
      ContactsConnectionPresenceStatusIndex index;
      const gchar *presence_message;
      GHashTable *parameters;

      index = GPOINTER_TO_UINT (g_hash_table_lookup (
            self->priv->presence_statuses, key));
      presence_message = g_hash_table_lookup (
          self->priv->presence_messages, key);

      parameters = g_hash_table_new_full (g_str_hash,
          g_str_equal, NULL, (GDestroyNotify) tp_g_value_slice_free);

      if (presence_message != NULL)
        g_hash_table_insert (parameters, "message",
            tp_g_value_slice_new_string (presence_message));

      g_hash_table_insert (result, key,
          tp_presence_status_new (index, parameters));
      g_hash_table_destroy (parameters);
    }

  return result;
}

static gboolean
my_set_own_status (GObject *object,
                   const TpPresenceStatus *status,
                   GError **error)
{
  TpBaseConnection *base_conn = TP_BASE_CONNECTION (object);
  ContactsConnectionPresenceStatusIndex index = status->index;
  const gchar *message = "";

  if (status->optional_arguments != NULL)
    {
      message = g_hash_table_lookup (status->optional_arguments, "message");

      if (message == NULL)
        message = "";
    }

  contacts_connection_change_presences (CONTACTS_CONNECTION (object),
      1, &(base_conn->self_handle), &index, &message);

  return TRUE;
}

static void
contacts_connection_class_init (ContactsConnectionClass *klass)
{
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;
  static const gchar *interfaces_always_present[] = {
      TP_IFACE_CONNECTION_INTERFACE_ALIASING,
      TP_IFACE_CONNECTION_INTERFACE_AVATARS,
      TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
      TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_LOCATION,
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES,
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_INFO,
      TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
      NULL };

  object_class->constructed = constructed;
  object_class->finalize = finalize;
  g_type_class_add_private (klass, sizeof (ContactsConnectionPrivate));

  base_class->interfaces_always_present = interfaces_always_present;

  tp_contacts_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ContactsConnectionClass, contacts_mixin));

  tp_presence_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ContactsConnectionClass, presence_mixin),
      my_status_available, my_get_contact_statuses,
      my_set_own_status, my_statuses);

  tp_presence_mixin_simple_presence_init_dbus_properties (object_class);
}

void
contacts_connection_change_aliases (ContactsConnection *self,
                                    guint n,
                                    const TpHandle *handles,
                                    const gchar * const *aliases)
{
  GPtrArray *structs = g_ptr_array_sized_new (n);
  guint i;

  for (i = 0; i < n; i++)
    {
      GValueArray *pair = g_value_array_new (2);

      DEBUG ("contact#%u -> %s", handles[i], aliases[i]);

      g_hash_table_insert (self->priv->aliases,
          GUINT_TO_POINTER (handles[i]), g_strdup (aliases[i]));

      g_value_array_append (pair, NULL);
      g_value_init (pair->values + 0, G_TYPE_UINT);
      g_value_set_uint (pair->values + 0, handles[i]);

      g_value_array_append (pair, NULL);
      g_value_init (pair->values + 1, G_TYPE_STRING);
      g_value_set_string (pair->values + 1, aliases[i]);

      g_ptr_array_add (structs, pair);
    }

  tp_svc_connection_interface_aliasing_emit_aliases_changed (self,
      structs);

  g_ptr_array_foreach (structs, (GFunc) g_value_array_free, NULL);
  g_ptr_array_free (structs, TRUE);
}

void
contacts_connection_change_presences (
    ContactsConnection *self,
    guint n,
    const TpHandle *handles,
    const ContactsConnectionPresenceStatusIndex *indexes,
    const gchar * const *messages)
{
  GHashTable *presences = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) tp_presence_status_free);
  guint i;

  for (i = 0; i < n; i++)
    {
      GHashTable *parameters;
      gpointer key = GUINT_TO_POINTER (handles[i]);

      DEBUG ("contact#%u -> %s \"%s\"", handles[i],
          my_statuses[indexes[i]].name, messages[i]);

      g_hash_table_insert (self->priv->presence_statuses, key,
          GUINT_TO_POINTER (indexes[i]));
      g_hash_table_insert (self->priv->presence_messages, key,
          g_strdup (messages[i]));

      parameters = g_hash_table_new_full (g_str_hash,
          g_str_equal, NULL, (GDestroyNotify) tp_g_value_slice_free);

      if (messages[i] != NULL && messages[i][0] != '\0')
        g_hash_table_insert (parameters, "message",
            tp_g_value_slice_new_string (messages[i]));

      g_hash_table_insert (presences, key, tp_presence_status_new (indexes[i],
            parameters));
      g_hash_table_destroy (parameters);
    }

  tp_presence_mixin_emit_presence_update ((GObject *) self,
      presences);
  g_hash_table_destroy (presences);
}

void
contacts_connection_change_avatar_tokens (ContactsConnection *self,
                                          guint n,
                                          const TpHandle *handles,
                                          const gchar * const *tokens)
{
  guint i;

  for (i = 0; i < n; i++)
    {
      DEBUG ("contact#%u -> %s", handles[i], tokens[i]);
      g_hash_table_insert (self->priv->avatar_tokens,
          GUINT_TO_POINTER (handles[i]), g_strdup (tokens[i]));
      tp_svc_connection_interface_avatars_emit_avatar_updated (self,
          handles[i], tokens[i]);
    }
}

void
contacts_connection_change_locations (ContactsConnection *self,
    guint n,
    const TpHandle *handles,
    GHashTable **locations)
{
  guint i;

  for (i = 0; i < n; i++)
    {
      DEBUG ("contact#%u ->", handles[i]);
      tp_asv_dump (locations[i]);
      g_hash_table_insert (self->priv->locations,
          GUINT_TO_POINTER (handles[i]), g_hash_table_ref (locations[i]));

      tp_svc_connection_interface_location_emit_location_updated (self,
          handles[i], locations[i]);
    }
}

void
contacts_connection_change_capabilities (ContactsConnection *self,
    GHashTable *capabilities)
{
  GHashTableIter iter;
  gpointer handle, caps;

  g_hash_table_iter_init (&iter, capabilities);
  while (g_hash_table_iter_next (&iter, &handle, &caps))
    {
      g_hash_table_insert (self->priv->capabilities,
          handle,
          g_boxed_copy (TP_ARRAY_TYPE_REQUESTABLE_CHANNEL_CLASS_LIST,
            caps));
    }

  tp_svc_connection_interface_contact_capabilities_emit_contact_capabilities_changed (
      self, capabilities);
}

void
contacts_connection_change_infos (ContactsConnection *self,
    guint n,
    const TpHandle *handles,
    GPtrArray **infos)
{
  guint i;

  for (i = 0; i < n; i++)
    {
      DEBUG ("contact#%u ->", handles[i]);
      g_hash_table_insert (self->priv->infos,
          GUINT_TO_POINTER (handles[i]), g_ptr_array_ref (infos[i]));

      tp_svc_connection_interface_contact_info_emit_contact_info_changed (self,
          handles[i], infos[i]);
    }
}

static void
my_get_alias_flags (TpSvcConnectionInterfaceAliasing *aliasing,
                    DBusGMethodInvocation *context)
{
  TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);

  TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);
  tp_svc_connection_interface_aliasing_return_from_get_alias_flags (context,
      0);
}

static void
my_get_aliases (TpSvcConnectionInterfaceAliasing *aliasing,
                const GArray *contacts,
                DBusGMethodInvocation *context)
{
  ContactsConnection *self = CONTACTS_CONNECTION (aliasing);
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
      TpHandle handle = g_array_index (contacts, TpHandle, i);
      const gchar *alias = g_hash_table_lookup (self->priv->aliases,
          GUINT_TO_POINTER (handle));

      if (alias == NULL)
        g_hash_table_insert (result, GUINT_TO_POINTER (handle),
            (gchar *) tp_handle_inspect (contact_repo, handle));
      else
        g_hash_table_insert (result, GUINT_TO_POINTER (handle),
            (gchar *) alias);
    }

  tp_svc_connection_interface_aliasing_return_from_get_aliases (context,
      result);
  g_hash_table_destroy (result);
}

static void
my_request_aliases (TpSvcConnectionInterfaceAliasing *aliasing,
                    const GArray *contacts,
                    DBusGMethodInvocation *context)
{
  ContactsConnection *self = CONTACTS_CONNECTION (aliasing);
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
      TpHandle handle = g_array_index (contacts, TpHandle, i);
      const gchar *alias = g_hash_table_lookup (self->priv->aliases,
          GUINT_TO_POINTER (handle));

      if (alias == NULL)
        g_ptr_array_add (result,
            (gchar *) tp_handle_inspect (contact_repo, handle));
      else
        g_ptr_array_add (result, (gchar *) alias);
    }

  g_ptr_array_add (result, NULL);
  strings = (gchar **) g_ptr_array_free (result, FALSE);
  tp_svc_connection_interface_aliasing_return_from_request_aliases (context,
      (const gchar **) strings);
  g_free (strings);
}

static void
init_aliasing (gpointer g_iface,
               gpointer iface_data)
{
  TpSvcConnectionInterfaceAliasingClass *klass = g_iface;

#define IMPLEMENT(x) tp_svc_connection_interface_aliasing_implement_##x (\
    klass, my_##x)
  IMPLEMENT(get_alias_flags);
  IMPLEMENT(request_aliases);
  IMPLEMENT(get_aliases);
  /* IMPLEMENT(set_aliases); */
#undef IMPLEMENT
}

static void
my_get_avatar_tokens (TpSvcConnectionInterfaceAvatars *avatars,
                      const GArray *contacts,
                      DBusGMethodInvocation *context)
{
  ContactsConnection *self = CONTACTS_CONNECTION (avatars);
  TpBaseConnection *base = TP_BASE_CONNECTION (avatars);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;
  GHashTable *result;
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
      TpHandle handle = g_array_index (contacts, TpHandle, i);
      const gchar *token = g_hash_table_lookup (self->priv->avatar_tokens,
          GUINT_TO_POINTER (handle));

      if (token == NULL)
        {
          /* we're expected to do a round-trip to the server to find out
           * their token, so we have to give some sort of result. Assume
           * no avatar, here */
          token = "";
          g_hash_table_insert (self->priv->avatar_tokens,
              GUINT_TO_POINTER (handle), g_strdup (token));
          tp_svc_connection_interface_avatars_emit_avatar_updated (self,
              handle, token);
        }

      g_hash_table_insert (result, GUINT_TO_POINTER (handle),
          (gchar *) token);
    }

  tp_svc_connection_interface_avatars_return_from_get_known_avatar_tokens (
      context, result);
  g_hash_table_destroy (result);
}

static void
my_get_known_avatar_tokens (TpSvcConnectionInterfaceAvatars *avatars,
                            const GArray *contacts,
                            DBusGMethodInvocation *context)
{
  ContactsConnection *self = CONTACTS_CONNECTION (avatars);
  TpBaseConnection *base = TP_BASE_CONNECTION (avatars);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;
  GHashTable *result;
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
      TpHandle handle = g_array_index (contacts, TpHandle, i);
      const gchar *token = g_hash_table_lookup (self->priv->avatar_tokens,
          GUINT_TO_POINTER (handle));

      g_hash_table_insert (result, GUINT_TO_POINTER (handle),
          (gchar *) (token != NULL ? token : ""));
    }

  tp_svc_connection_interface_avatars_return_from_get_known_avatar_tokens (
      context, result);
  g_hash_table_destroy (result);
}

static void
init_avatars (gpointer g_iface,
              gpointer iface_data)
{
  TpSvcConnectionInterfaceAvatarsClass *klass = g_iface;

#define IMPLEMENT(x) tp_svc_connection_interface_avatars_implement_##x (\
    klass, my_##x)
  /* IMPLEMENT(get_avatar_requirements); */
  IMPLEMENT(get_avatar_tokens);
  IMPLEMENT(get_known_avatar_tokens);
  /* IMPLEMENT(request_avatar); */
  /* IMPLEMENT(request_avatars); */
  /* IMPLEMENT(set_avatar); */
  /* IMPLEMENT(clear_avatar); */
#undef IMPLEMENT
}

static void
my_get_locations (TpSvcConnectionInterfaceLocation *avatars,
    const GArray *contacts,
    DBusGMethodInvocation *context)
{
  ContactsConnection *self = CONTACTS_CONNECTION (avatars);
  TpBaseConnection *base = TP_BASE_CONNECTION (avatars);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;
  GHashTable *result;
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
      TpHandle handle = g_array_index (contacts, TpHandle, i);
      GHashTable *location = g_hash_table_lookup (self->priv->locations,
          GUINT_TO_POINTER (handle));

      if (location != NULL)
        {
          g_hash_table_insert (result, GUINT_TO_POINTER (handle), location);
        }
    }

  tp_svc_connection_interface_location_return_from_get_locations (
      context, result);
  g_hash_table_destroy (result);
}

static void
init_location (gpointer g_iface,
    gpointer iface_data)
{
  TpSvcConnectionInterfaceLocationClass *klass = g_iface;

#define IMPLEMENT(x) tp_svc_connection_interface_location_implement_##x (\
    klass, my_##x)
  IMPLEMENT(get_locations);
#undef IMPLEMENT
}

static void
my_get_contact_capabilities (TpSvcConnectionInterfaceContactCapabilities *obj,
    const GArray *contacts,
    DBusGMethodInvocation *context)
{
  ContactsConnection *self = CONTACTS_CONNECTION (obj);
  TpBaseConnection *base = TP_BASE_CONNECTION (obj);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;
  GHashTable *result;
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
      TpHandle handle = g_array_index (contacts, TpHandle, i);
      GPtrArray *arr = g_hash_table_lookup (self->priv->capabilities,
          GUINT_TO_POINTER (handle));

      if (arr != NULL)
        {
          g_hash_table_insert (result, GUINT_TO_POINTER (handle), arr);
        }
    }

  tp_svc_connection_interface_contact_capabilities_return_from_get_contact_capabilities (
      context, result);

  g_hash_table_destroy (result);
}

static void
init_contact_caps (gpointer g_iface,
    gpointer iface_data)
{
  TpSvcConnectionInterfaceContactCapabilitiesClass *klass = g_iface;

#define IMPLEMENT(x) tp_svc_connection_interface_contact_capabilities_implement_##x (\
    klass, my_##x)
  IMPLEMENT(get_contact_capabilities);
#undef IMPLEMENT
}

static void
my_refresh_contact_info (TpSvcConnectionInterfaceContactInfo *obj,
    const GArray *contacts,
    DBusGMethodInvocation *context)
{
  ContactsConnection *self = CONTACTS_CONNECTION (obj);
  TpBaseConnection *base = TP_BASE_CONNECTION (obj);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;
  guint i;

  TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

  if (!tp_handles_are_valid (contact_repo, contacts, FALSE, &error))
    {
      dbus_g_method_return_error (context, error);
      g_error_free (error);
      return;
    }

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle handle = g_array_index (contacts, guint, i);
      GPtrArray *arr = g_hash_table_lookup (self->priv->infos,
          GUINT_TO_POINTER (handle));
      if (arr != NULL)
        {
          const gchar * const field_values[2] = {
              "http://telepathy.freedesktop.org", NULL
          };

          g_ptr_array_add (arr, tp_value_array_build (3,
                      G_TYPE_STRING, "url",
                      G_TYPE_STRV, NULL,
                      G_TYPE_STRV, field_values,
                      G_TYPE_INVALID));

          tp_svc_connection_interface_contact_info_emit_contact_info_changed (self,
                  handle, arr);
        }
    }

  tp_svc_connection_interface_contact_info_return_from_refresh_contact_info (
      context);
}

static void
init_contact_info (gpointer g_iface,
    gpointer iface_data)
{
  TpSvcConnectionInterfaceContactInfoClass *klass = g_iface;

#define IMPLEMENT(x) tp_svc_connection_interface_contact_info_implement_##x (\
    klass, my_##x)
  IMPLEMENT(refresh_contact_info);
#undef IMPLEMENT
}

/* =============== Legacy version (no Contacts interface) ================= */

G_DEFINE_TYPE (LegacyContactsConnection, legacy_contacts_connection,
    CONTACTS_TYPE_CONNECTION);

static void
legacy_contacts_connection_init (LegacyContactsConnection *self)
{
}

static void
legacy_contacts_connection_class_init (LegacyContactsConnectionClass *klass)
{
  /* Leave Contacts out of the interfaces we say are present, so clients
   * won't use it */
  static const gchar *interfaces_always_present[] = {
      TP_IFACE_CONNECTION_INTERFACE_ALIASING,
      TP_IFACE_CONNECTION_INTERFACE_AVATARS,
      TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_LOCATION,
      TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
      NULL };
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;

  base_class->interfaces_always_present = interfaces_always_present;
}

/* =============== No Requests and no ContactCapabilities ================= */

G_DEFINE_TYPE (NoRequestsConnection, no_requests_connection,
    CONTACTS_TYPE_CONNECTION);

static void
no_requests_connection_init (NoRequestsConnection *self)
{
}

static void
no_requests_connection_class_init (NoRequestsConnectionClass *klass)
{
  static const gchar *interfaces_always_present[] = {
      TP_IFACE_CONNECTION_INTERFACE_ALIASING,
      TP_IFACE_CONNECTION_INTERFACE_AVATARS,
      TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
      TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_LOCATION,
      NULL };
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;

  base_class->interfaces_always_present = interfaces_always_present;
}
