/*
 * protocol.c - an example Protocol
 *
 * Copyright © 2007-2010 Collabora Ltd.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "protocol.h"

#include <telepathy-glib/telepathy-glib.h>

#include "conn.h"
#include "im-manager.h"

#include "_gen/param-spec-struct.h"

G_DEFINE_TYPE (ExampleEcho2Protocol,
    example_echo_2_protocol,
    TP_TYPE_BASE_PROTOCOL)

static void
example_echo_2_protocol_init (
    ExampleEcho2Protocol *self)
{
}

static const TpCMParamSpec *
get_parameters (TpBaseProtocol *self)
{
  return example_echo_2_example_params;
}

static TpBaseConnection *
new_connection (TpBaseProtocol *protocol,
    GHashTable *asv,
    GError **error)
{
  ExampleEcho2Connection *conn;
  const gchar *account;
  gchar *protocol_name;

  account = tp_asv_get_string (asv, "account");

  if (account == NULL || account[0] == '\0')
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "The 'account' parameter is required");
      return NULL;
    }

  g_object_get (protocol,
      "name", &protocol_name,
      NULL);

  conn = EXAMPLE_ECHO_2_CONNECTION (
      g_object_new (EXAMPLE_TYPE_ECHO_2_CONNECTION,
        "account", account,
        "protocol", protocol_name,
        NULL));
  g_free (protocol_name);

  return (TpBaseConnection *) conn;
}

gchar *
example_echo_2_protocol_normalize_contact (const gchar *id, GError **error)
{
  if (id[0] == '\0')
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "ID must not be empty");
      return NULL;
    }

  return g_utf8_strdown (id, -1);
}

static gchar *
normalize_contact (TpBaseProtocol *self G_GNUC_UNUSED,
    const gchar *contact,
    GError **error)
{
  return example_echo_2_protocol_normalize_contact (contact, error);
}

static gchar *
identify_account (TpBaseProtocol *self G_GNUC_UNUSED,
    GHashTable *asv,
    GError **error)
{
  const gchar *account = tp_asv_get_string (asv, "account");

  if (account != NULL)
    return g_strdup (account);

  g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
      "'account' parameter not given");
  return NULL;
}

static GStrv
get_interfaces (TpBaseProtocol *self)
{
  return NULL;
}

static void
get_connection_details (TpBaseProtocol *self G_GNUC_UNUSED,
    GStrv *connection_interfaces,
    GPtrArray **requestable_channel_classes,
    gchar **icon_name,
    gchar **english_name,
    gchar **vcard_field)
{
  if (connection_interfaces != NULL)
    {
      *connection_interfaces = g_strdupv (
          (GStrv) example_echo_2_connection_get_possible_interfaces ());
    }

  if (requestable_channel_classes != NULL)
    {
      *requestable_channel_classes = g_ptr_array_new ();
      example_echo_2_im_manager_append_channel_classes (
          *requestable_channel_classes);
    }

  if (icon_name != NULL)
    {
      /* a real protocol would use its own icon name - for this example we
       * borrow the one from ICQ */
      *icon_name = g_strdup ("im-icq");
    }

  if (english_name != NULL)
    {
      /* in a real protocol this would be "ICQ" or
       * "Windows Live Messenger (MSN)" or something */
      *english_name = g_strdup ("Echo II example");
    }

  if (vcard_field != NULL)
    {
      /* in a real protocol this would be "tel" or "x-jabber" or something */
      *vcard_field = g_strdup ("x-telepathy-example");
    }
}

static void
example_echo_2_protocol_class_init (
    ExampleEcho2ProtocolClass *klass)
{
  TpBaseProtocolClass *base_class =
      (TpBaseProtocolClass *) klass;

  base_class->get_parameters = get_parameters;
  base_class->new_connection = new_connection;

  base_class->normalize_contact = normalize_contact;
  base_class->identify_account = identify_account;
  base_class->get_interfaces = get_interfaces;
  base_class->get_connection_details = get_connection_details;
}
