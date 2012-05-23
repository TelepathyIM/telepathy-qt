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

#include "call-manager.h"
#include "conn.h"

G_DEFINE_TYPE (ExampleCallProtocol,
    example_call_protocol,
    TP_TYPE_BASE_PROTOCOL)

static void
example_call_protocol_init (
    ExampleCallProtocol *self)
{
}

gboolean
example_call_protocol_check_contact_id (const gchar *id,
    gchar **normal,
    GError **error)
{
  g_return_val_if_fail (id != NULL, FALSE);

  if (id[0] == '\0')
    {
      g_set_error (error, TP_ERROR, TP_ERROR_INVALID_HANDLE,
          "ID must not be empty");
      return FALSE;
    }

  if (normal != NULL)
    *normal = g_utf8_normalize (id, -1, G_NORMALIZE_ALL_COMPOSE);

  return TRUE;
}

static gboolean
account_param_filter (const TpCMParamSpec *paramspec,
                      GValue *value,
                      GError **error)
{
  const gchar *id = g_value_get_string (value);

  return example_call_protocol_check_contact_id (id, NULL, error);
}

static const TpCMParamSpec example_call_example_params[] = {
  { "account", "s", G_TYPE_STRING,
    TP_CONN_MGR_PARAM_FLAG_REQUIRED | TP_CONN_MGR_PARAM_FLAG_REGISTER,
    NULL,                               /* no default */
    0,                                  /* unused, formerly struct offset */
    account_param_filter,
    NULL,                               /* filter data, unused here */
    NULL },                             /* setter data, now unused */
  { "simulation-delay", "u", G_TYPE_UINT,
    TP_CONN_MGR_PARAM_FLAG_HAS_DEFAULT,
    GUINT_TO_POINTER (1000),            /* default */
    0,                                  /* unused, formerly struct offset */
    NULL,                               /* no filter */
    NULL,                               /* filter data, unused here */
    NULL },                             /* setter data, now unused */
  { NULL }
};

static const TpCMParamSpec *
get_parameters (TpBaseProtocol *self)
{
  return example_call_example_params;
}

static TpBaseConnection *
new_connection (TpBaseProtocol *protocol,
    GHashTable *asv,
    GError **error)
{
  ExampleCallConnection *conn;
  const gchar *account;
  guint sim_delay;

  account = tp_asv_get_string (asv, "account");
  /* telepathy-glib checked this for us */
  g_assert (account != NULL);

  sim_delay = tp_asv_get_uint32 (asv, "simulation-delay", NULL);

  conn = EXAMPLE_CALL_CONNECTION (
      g_object_new (EXAMPLE_TYPE_CALL_CONNECTION,
        "account", account,
        "protocol", tp_base_protocol_get_name (protocol),
        "simulation-delay", sim_delay,
        NULL));

  return (TpBaseConnection *) conn;
}

static gchar *
normalize_contact (TpBaseProtocol *self G_GNUC_UNUSED,
    const gchar *contact,
    GError **error)
{
  gchar *normal;

  if (example_call_protocol_check_contact_id (contact, &normal, error))
    return normal;
  else
    return NULL;
}

static gchar *
identify_account (TpBaseProtocol *self G_GNUC_UNUSED,
    GHashTable *asv,
    GError **error)
{
  const gchar *account = tp_asv_get_string (asv, "account");

  if (account != NULL)
    return normalize_contact (self, account, error);

  g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
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
    GType **channel_managers,
    gchar **icon_name,
    gchar **english_name,
    gchar **vcard_field)
{
  if (connection_interfaces != NULL)
    {
      *connection_interfaces = g_strdupv (
          (GStrv) example_call_connection_get_possible_interfaces ());
    }

  if (channel_managers != NULL)
    {
      GType types[] = { EXAMPLE_TYPE_CALL_MANAGER, G_TYPE_INVALID };

      *channel_managers = g_memdup (types, sizeof (types));
    }

  if (icon_name != NULL)
    *icon_name = g_strdup ("face-smile");

  if (english_name != NULL)
    *english_name = g_strdup ("Example with Call channels");

  if (vcard_field != NULL)
    *vcard_field = g_strdup ("x-telepathy-example");
}

static void
example_call_protocol_class_init (
    ExampleCallProtocolClass *klass)
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
