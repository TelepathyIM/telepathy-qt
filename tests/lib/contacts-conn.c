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

#include "tests/lib/debug.h"

static void init_aliasing (gpointer, gpointer);
static void init_avatars (gpointer, gpointer);
static void init_capabilities (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (ContactsConnection,
    contacts_connection,
    SIMPLE_TYPE_CONNECTION,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_ALIASING,
      init_aliasing);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_AVATARS,
      init_avatars);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_CAPABILITIES,
      init_capabilities);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACTS,
      tp_contacts_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_PRESENCE,
      tp_presence_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      tp_presence_mixin_simple_presence_iface_init)
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
};

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
      GValue *val = tp_g_value_slice_new (G_TYPE_STRING);
      TpHandle handle = g_array_index (contacts, guint, i);
      const gchar *alias = g_hash_table_lookup (self->priv->aliases,
          GUINT_TO_POINTER (handle));

      if (alias == NULL)
        {
          alias = tp_handle_inspect (contact_repo, handle);
        }

        g_value_set_string (val, alias);
        tp_contacts_mixin_set_contact_attribute (attributes, handle,
            TP_IFACE_CONNECTION_INTERFACE_ALIASING "/alias", val);
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
          GValue *val = tp_g_value_slice_new (G_TYPE_STRING);

          g_value_set_string (val, token);
          tp_contacts_mixin_set_contact_attribute (attributes, handle,
              TP_IFACE_CONNECTION_INTERFACE_AVATARS "/token", val);
        }
    }
}

static void
get_contact_capabilities (ContactsConnection *self,
                          guint handle,
                          GPtrArray *arr)
{
  GValue monster = {0, };
  GHashTable *fixed_properties;
  GValue *channel_type_value;
  GValue *target_handle_type_value;
  gchar *text_allowed_properties[] =
      {
        TP_IFACE_CHANNEL ".TargetHandle",
        NULL
      };

  g_assert (handle != 0);

  g_value_init (&monster, TP_STRUCT_TYPE_REQUESTABLE_CHANNEL_CLASS);
  g_value_take_boxed (&monster,
      dbus_g_type_specialized_construct (
        TP_STRUCT_TYPE_REQUESTABLE_CHANNEL_CLASS));

  fixed_properties = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
      (GDestroyNotify) tp_g_value_slice_free);

  channel_type_value = tp_g_value_slice_new (G_TYPE_STRING);
  g_value_set_static_string (channel_type_value, TP_IFACE_CHANNEL_TYPE_TEXT);
  g_hash_table_insert (fixed_properties, TP_IFACE_CHANNEL ".ChannelType",
      channel_type_value);

  target_handle_type_value = tp_g_value_slice_new (G_TYPE_UINT);
  g_value_set_uint (target_handle_type_value, TP_HANDLE_TYPE_CONTACT);
  g_hash_table_insert (fixed_properties, TP_IFACE_CHANNEL ".TargetHandleType",
      target_handle_type_value);

  dbus_g_type_struct_set (&monster,
      0, fixed_properties,
      1, text_allowed_properties,
      G_MAXUINT);

  g_hash_table_destroy (fixed_properties);

  g_ptr_array_add (arr, g_value_get_boxed (&monster));
}

static void
free_capabilities_array (GPtrArray *arr)
{
  guint i;

  for (i = 0; i < arr->len; i++)
    {
      g_boxed_free (TP_STRUCT_TYPE_CAPABILITY_CHANGE,
          g_ptr_array_index (arr, i));
    }

  g_ptr_array_free (arr, TRUE);
}

static void
capabilities_fill_contact_attributes (GObject *object,
                                      const GArray *contacts,
                                      GHashTable *attributes)
{
  ContactsConnection *self = CONTACTS_CONNECTION (object);
  guint i;

  for (i = 0; i < contacts->len; i++)
    {
      GPtrArray *arr = g_ptr_array_new ();
      TpHandle handle = g_array_index (contacts, guint, i);
      GValue *val =  tp_g_value_slice_new (
              TP_ARRAY_TYPE_REQUESTABLE_CHANNEL_CLASS_LIST);

      get_contact_capabilities (self, handle, arr);
      g_value_take_boxed (val, arr);
      tp_contacts_mixin_set_contact_attribute (attributes, handle,
          TP_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES "/capabilities", val);
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
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES,
      capabilities_fill_contact_attributes);

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
      { "offline", TP_CONNECTION_PRESENCE_TYPE_AVAILABLE, TRUE, NULL },
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
        {
          GValue *value = tp_g_value_slice_new (G_TYPE_STRING);

          g_value_set_string (value, presence_message);
          g_hash_table_insert (parameters, "message", value);
        }

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
  g_set_error (error, TP_ERRORS, TP_ERROR_NOT_IMPLEMENTED,
      "Not implemented");
  return FALSE;
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
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES,
      TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
      TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
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
        {
          GValue *value = tp_g_value_slice_new (G_TYPE_STRING);

          g_value_set_string (value, messages[i]);
          g_hash_table_insert (parameters, "message", value);
        }

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
my_get_contact_capabilities (TpSvcConnectionInterfaceContactCapabilities *caps,
                             const GArray *handles,
                             DBusGMethodInvocation *context)
{
  ContactsConnection *self = CONTACTS_CONNECTION (caps);
  TpBaseConnection *base = TP_BASE_CONNECTION (caps);
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
      TP_HANDLE_TYPE_CONTACT);
  GError *error = NULL;
  GHashTable *ret;
  guint i;

  TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

  if (!tp_handles_are_valid (contact_repo, handles, FALSE, &error))
    {
      dbus_g_method_return_error (context, error);
      g_error_free (error);
      return;
    }

  ret = g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) free_capabilities_array);

  for (i = 0; i < handles->len; i++)
    {
      GPtrArray *val = g_ptr_array_new ();
      TpHandle handle = g_array_index (handles, guint, i);
      get_contact_capabilities (self, handle, val);
      g_hash_table_insert (ret, GUINT_TO_POINTER (handle), val);
    }

  tp_svc_connection_interface_contact_capabilities_return_from_get_contact_capabilities
      (context, ret);

  g_hash_table_destroy (ret);
}

static void
init_capabilities (gpointer g_iface,
                   gpointer iface_data)
{
  TpSvcConnectionInterfaceContactCapabilitiesClass *klass = g_iface;

#define IMPLEMENT(x) tp_svc_connection_interface_contact_capabilities_implement_##x (\
    klass, my_##x)
  IMPLEMENT(get_contact_capabilities);
  /* IMPLEMENT(update_capabilities); */
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
      NULL };
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;

  base_class->interfaces_always_present = interfaces_always_present;
}
