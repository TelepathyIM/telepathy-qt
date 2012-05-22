/*
 * conn.c - connection implementing Conn.I.Addressing
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2011 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "conn.h"

#include <dbus/dbus-glib-lowlevel.h>

#include <telepathy-glib/connection.h>
#include <telepathy-glib/contacts-mixin.h>
#include <telepathy-glib/enums.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>

#include "extensions/extensions.h"

#include <string.h>

static void addressing_fill_contact_attributes (GObject *obj,
    const GArray *contacts,
    GHashTable *attributes_hash);
static void addressing_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (TpTestsAddressingConnection,
    tp_tests_addressing_connection,
    TP_TESTS_TYPE_CONTACTS_CONNECTION,
    G_IMPLEMENT_INTERFACE (FUTURE_TYPE_SVC_CONNECTION_INTERFACE_ADDRESSING,
      addressing_iface_init);
    );

struct _TpTestsAddressingConnectionPrivate
{
};

static const gchar *addressable_vcard_fields[] = {"x-addr", NULL};
static const gchar *addressable_uri_schemes[] = {"addr", NULL};

static const char *assumed_interfaces[] = {
  TP_IFACE_CONNECTION,
  FUTURE_IFACE_CONNECTION_INTERFACE_ADDRESSING,
  NULL
};

static void
constructed (GObject *object)
{
  TpTestsAddressingConnection *self = TP_TESTS_ADDRESSING_CONNECTION (object);
  void (*parent_impl) (GObject *) =
      G_OBJECT_CLASS (tp_tests_addressing_connection_parent_class)->constructed;

  if (parent_impl != NULL)
    parent_impl (object);

  tp_contacts_mixin_add_contact_attributes_iface (G_OBJECT (self),
      FUTURE_IFACE_CONNECTION_INTERFACE_ADDRESSING,
      addressing_fill_contact_attributes);
}

static void
tp_tests_addressing_connection_init (TpTestsAddressingConnection *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TP_TESTS_TYPE_ADDRESSING_CONNECTION, TpTestsAddressingConnectionPrivate);
}

static void
finalize (GObject *object)
{
  G_OBJECT_CLASS (tp_tests_addressing_connection_parent_class)->finalize (object);
}

static void
tp_tests_addressing_connection_class_init (TpTestsAddressingConnectionClass *klass)
{
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;
  static const gchar *interfaces_always_present[] = {
      TP_IFACE_CONNECTION_INTERFACE_ALIASING,
      TP_IFACE_CONNECTION_INTERFACE_AVATARS,
      TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_LIST,
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS,
      TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_LOCATION,
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES,
      TP_IFACE_CONNECTION_INTERFACE_CONTACT_INFO,
      TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
      FUTURE_IFACE_CONNECTION_INTERFACE_ADDRESSING,
      NULL };

  object_class->constructed = constructed;
  object_class->finalize = finalize;
  g_type_class_add_private (klass, sizeof (TpTestsAddressingConnectionPrivate));

  base_class->interfaces_always_present = interfaces_always_present;
}

static gchar **
uris_for_handle (TpHandleRepoIface *contact_repo,
    TpHandle contact)
{
  GPtrArray *uris = g_ptr_array_new ();
  const gchar * const *scheme;

  for (scheme = addressable_uri_schemes; *scheme != NULL; scheme++)
    {
      const gchar *identifier = tp_handle_inspect (contact_repo, contact);
      gchar *uri = g_strdup_printf ("%s:%s", *scheme, identifier);

      if (uri != NULL)
        {
          g_ptr_array_add (uris, uri);
        }
    }

  g_ptr_array_add (uris, NULL);
  return (gchar **) g_ptr_array_free (uris, FALSE);
}

static GHashTable *
vcard_addresses_for_handle (TpHandleRepoIface *contact_repo,
    TpHandle contact)
{
  GHashTable *addresses = g_hash_table_new_full (g_str_hash, g_str_equal,
      NULL, (GDestroyNotify) g_free);
  const gchar * const *field;

  for (field = addressable_vcard_fields; *field != NULL; field++)
    {
      const gchar *identifier = tp_handle_inspect (contact_repo, contact);

      if (identifier != NULL)
        {
          g_hash_table_insert (addresses, (gpointer) *field, g_strdup (identifier));
        }
    }

  return addresses;
}

static void
addressing_fill_contact_attributes (GObject *obj,
    const GArray *contacts,
    GHashTable *attributes_hash)
{
  guint i;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
      (TpBaseConnection *) obj, TP_HANDLE_TYPE_CONTACT);

  for (i = 0; i < contacts->len; i++)
    {
      TpHandle contact = g_array_index (contacts, TpHandle, i);
      gchar **uris = uris_for_handle (contact_repo, contact);
      GHashTable *addresses = vcard_addresses_for_handle (contact_repo, contact);

      tp_contacts_mixin_set_contact_attribute (attributes_hash,
          contact, FUTURE_IFACE_CONNECTION_INTERFACE_ADDRESSING"/uris",
          tp_g_value_slice_new_take_boxed (G_TYPE_STRV, uris));

      tp_contacts_mixin_set_contact_attribute (attributes_hash,
          contact, FUTURE_IFACE_CONNECTION_INTERFACE_ADDRESSING"/addresses",
          tp_g_value_slice_new_take_boxed (TP_HASH_TYPE_STRING_STRING_MAP, addresses));
    }
}

static gchar *
uri_to_id (const gchar *uri,
    GError **error)
{
  gchar *scheme;
  gchar *normalized_id = NULL;

  g_return_val_if_fail (uri != NULL, NULL);

  scheme = g_uri_parse_scheme (uri);

  if (scheme == NULL)
    {
      g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "'%s' is not a valid URI", uri);
      goto OUT;
    }
  else if (g_ascii_strcasecmp (scheme, "addr") == 0)
    {
      normalized_id = g_strdup (uri + strlen (scheme) + 1); /* Strip the scheme */
    }
  else
    {
      g_set_error (error, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED,
          "'%s' URI scheme is not supported by this protocol",
          scheme);
      goto OUT;
    }

OUT:
  g_free (scheme);

  return normalized_id;
}

static TpHandle
ensure_handle_from_uri (TpHandleRepoIface *repo,
    const gchar *uri,
    GError **error)
{
  TpHandle handle;
  gchar *id = uri_to_id (uri, error);

  if (id == NULL)
    return 0;

  handle = tp_handle_ensure (repo, id, NULL, error);

  g_free (id);

  return handle;
}

static void
addressing_get_contacts_by_uri (FutureSvcConnectionInterfaceAddressing *iface,
    const gchar **uris,
    const gchar **interfaces,
    DBusGMethodInvocation *context)
{
  const gchar **uri;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
      (TpBaseConnection *) iface, TP_HANDLE_TYPE_CONTACT);
  GHashTable *attributes;
  GHashTable *requested = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  GArray *handles = g_array_sized_new (TRUE, TRUE, sizeof (TpHandle),
      g_strv_length ((gchar **) uris));
  gchar *sender = dbus_g_method_get_sender (context);

  for (uri = uris; *uri != NULL; uri++)
    {
      TpHandle h = ensure_handle_from_uri (contact_repo, *uri, NULL);

      if (h == 0)
        continue;

      g_hash_table_insert (requested, g_strdup (*uri), GUINT_TO_POINTER (h));
      g_array_append_val (handles, h);
    }

  attributes = tp_contacts_mixin_get_contact_attributes (G_OBJECT (iface), handles,
      interfaces, assumed_interfaces, sender);

  future_svc_connection_interface_addressing_return_from_get_contacts_by_uri (
      context, requested, attributes);

  g_hash_table_unref (requested);
  g_hash_table_unref (attributes);
  g_free (sender);
}

static gchar *
vcard_address_to_id (const gchar *vcard_field,
    const gchar *vcard_address,
    GError **error)
{
  gchar *normalized_id = NULL;

  g_return_val_if_fail (vcard_field != NULL, NULL);
  g_return_val_if_fail (vcard_address != NULL, NULL);

  if (g_ascii_strcasecmp (vcard_field, "x-addr") == 0)
    {
      normalized_id = g_strdup (vcard_address);
    }
  else
    {
      g_set_error (error, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED,
          "'%s' vCard field is not supported by this protocol", vcard_field);
    }

  return normalized_id;
}

static TpHandle
ensure_handle_from_vcard_address (TpHandleRepoIface *repo,
    const gchar *vcard_field,
    const gchar *vcard_address,
    GError **error)
{
  TpHandle handle;
  gchar *normalized_id;

  normalized_id = vcard_address_to_id (vcard_field, vcard_address, error);
  if (normalized_id == NULL)
    return 0;

  handle = tp_handle_ensure (repo, normalized_id, NULL, error);

  g_free (normalized_id);

  return handle;
}

static void
addressing_get_contacts_by_vcard_field (FutureSvcConnectionInterfaceAddressing *iface,
    const gchar *field,
    const gchar **addresses,
    const gchar **interfaces,
    DBusGMethodInvocation *context)
{
  const gchar **address;
  TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
      (TpBaseConnection *) iface, TP_HANDLE_TYPE_CONTACT);
  GHashTable *attributes;
  GHashTable *requested = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  GArray *handles = g_array_sized_new (TRUE, TRUE, sizeof (TpHandle),
      g_strv_length ((gchar **) addresses));
  gchar *sender = dbus_g_method_get_sender (context);

  for (address = addresses; *address != NULL; address++)
    {
      TpHandle h = ensure_handle_from_vcard_address (contact_repo, field,
          *address, NULL);

      if (h == 0)
        continue;

      g_hash_table_insert (requested, g_strdup (*address), GUINT_TO_POINTER (h));
      g_array_append_val (handles, h);
    }

  attributes = tp_contacts_mixin_get_contact_attributes (G_OBJECT (iface), handles,
      interfaces, assumed_interfaces, sender);

  future_svc_connection_interface_addressing_return_from_get_contacts_by_vcard_field (
      context, requested, attributes);

  g_hash_table_unref (requested);
  g_hash_table_unref (attributes);
  g_free (sender);
}

static void
addressing_iface_init (gpointer g_iface, gpointer iface_data)
{
#define IMPLEMENT(x) \
  future_svc_connection_interface_addressing_implement_##x (\
      g_iface, addressing_##x)

  IMPLEMENT(get_contacts_by_uri);
  IMPLEMENT(get_contacts_by_vcard_field);
#undef IMPLEMENT
}
