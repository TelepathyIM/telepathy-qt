/*
 * conn.c - an example connection
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "conn.h"

#include <string.h>

#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>
#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/interfaces.h>

#include "room-manager.h"

G_DEFINE_TYPE (ExampleCSHConnection,
    example_csh_connection,
    CONTACTS_TYPE_CONNECTION)

/* type definition stuff */

enum
{
  PROP_ACCOUNT = 1,
  N_PROPS
};

struct _ExampleCSHConnectionPrivate
{
  gchar *account;
};

static void
example_csh_connection_init (ExampleCSHConnection *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CSH_CONNECTION, ExampleCSHConnectionPrivate);
}

static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *spec)
{
  ExampleCSHConnection *self = EXAMPLE_CSH_CONNECTION (object);

  switch (property_id) {
    case PROP_ACCOUNT:
      g_value_set_string (value, self->priv->account);
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
  ExampleCSHConnection *self = EXAMPLE_CSH_CONNECTION (object);

  switch (property_id) {
    case PROP_ACCOUNT:
      g_free (self->priv->account);
      self->priv->account = g_utf8_strdown (g_value_get_string (value), -1);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
  }
}

static void
finalize (GObject *object)
{
  ExampleCSHConnection *self = EXAMPLE_CSH_CONNECTION (object);

  g_free (self->priv->account);

  G_OBJECT_CLASS (example_csh_connection_parent_class)->finalize (object);
}

static gchar *
get_unique_connection_name (TpBaseConnection *conn)
{
  ExampleCSHConnection *self = EXAMPLE_CSH_CONNECTION (conn);

  return g_strdup (self->priv->account);
}

gchar *
example_csh_normalize_contact (TpHandleRepoIface *repo,
                               const gchar *id,
                               gpointer context,
                               GError **error)
{
  const gchar *at;
  /* For this example, we imagine that global handles look like
   * username@realm and channel-specific handles look like nickname@#chatroom,
   * where username and nickname contain any UTF-8 except "@", and realm
   * and chatroom contain any UTF-8 except "@" and "#".
   *
   * Additionally, we imagine that everything is case-sensitive but is
   * required to be in NFKC.
   */

  if (id[0] == '\0')
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "ID must not be empty");
      return NULL;
    }

  at = strchr (id, '@');

  if (at == NULL || at == id || at[1] == '\0')
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "ID must look like aaa@bbb");
      return NULL;
    }

  if (strchr (at + 1, '@') != NULL)
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "ID cannot contain more than one '@'");
      return NULL;
    }

  if (at[1] == '#' && at[2] == '\0')
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "chatroom name cannot be empty");
      return NULL;
    }

  if (strchr (at + 2, '#') != NULL)
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "realm/chatroom cannot contain '#' except at the beginning");
      return NULL;
    }

  return g_utf8_normalize (id, -1, G_NORMALIZE_ALL_COMPOSE);
}

static gchar *
example_csh_normalize_room (TpHandleRepoIface *repo,
                            const gchar *id,
                            gpointer context,
                            GError **error)
{
  /* See example_csh_normalize_contact(). */

  if (id[0] != '#')
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "Chatroom names in this protocol start with #");
    }

  if (id[1] == '\0')
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "Chatroom name cannot be empty");
      return NULL;
    }

  if (strchr (id, '@') != NULL)
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
          "Chatroom names in this protocol cannot contain '@'");
      return NULL;
    }

  return g_utf8_normalize (id, -1, G_NORMALIZE_ALL_COMPOSE);
}

static void
create_handle_repos (TpBaseConnection *conn,
                     TpHandleRepoIface *repos[NUM_TP_HANDLE_TYPES])
{
  repos[TP_HANDLE_TYPE_CONTACT] = tp_dynamic_handle_repo_new
      (TP_HANDLE_TYPE_CONTACT, example_csh_normalize_contact, NULL);

  repos[TP_HANDLE_TYPE_ROOM] = tp_dynamic_handle_repo_new
      (TP_HANDLE_TYPE_ROOM, example_csh_normalize_room, NULL);
}

static GPtrArray *
create_channel_managers (TpBaseConnection *conn)
{
  GPtrArray *ret = g_ptr_array_sized_new (1);

  g_ptr_array_add (ret, g_object_new (EXAMPLE_TYPE_CSH_ROOM_MANAGER,
        "connection", conn,
        NULL));

  return ret;
}

static gboolean
start_connecting (TpBaseConnection *conn,
                  GError **error)
{
  ExampleCSHConnection *self = EXAMPLE_CSH_CONNECTION (conn);
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
example_csh_connection_class_init (ExampleCSHConnectionClass *klass)
{
  static const gchar *interfaces_always_present[] = {
      TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
      TP_IFACE_CONNECTION_INTERFACE_ALIASING,
      TP_IFACE_CONNECTION_INTERFACE_AVATARS,
      TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
      TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
      TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      NULL };
  TpBaseConnectionClass *base_class =
      (TpBaseConnectionClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->finalize = finalize;
  g_type_class_add_private (klass, sizeof (ExampleCSHConnectionPrivate));

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
}

void
example_csh_connection_set_enable_change_members_detailed (ExampleCSHConnection *self,
                                                           gboolean enable)
{
  TpChannelManagerIter iter;
  TpChannelManager *channel_manager;

  g_return_if_fail (EXAMPLE_IS_CSH_CONNECTION (self));

  tp_base_connection_channel_manager_iter_init (&iter, (TpBaseConnection *) self);

  while (tp_base_connection_channel_manager_iter_next (&iter, &channel_manager))
    {
      if (EXAMPLE_IS_CSH_ROOM_MANAGER (channel_manager))
        {
          example_csh_room_manager_set_enable_change_members_detailed (
            (ExampleCSHRoomManager *) channel_manager, enable);
        }
    }
}

void
example_csh_connection_accept_invitations (ExampleCSHConnection *self)
{
  TpChannelManagerIter iter;
  TpChannelManager *channel_manager;

  g_return_if_fail (EXAMPLE_IS_CSH_CONNECTION (self));

  tp_base_connection_channel_manager_iter_init (&iter, (TpBaseConnection *) self);

  while (tp_base_connection_channel_manager_iter_next (&iter, &channel_manager))
    {
      if (EXAMPLE_IS_CSH_ROOM_MANAGER (channel_manager))
        {
          example_csh_room_manager_accept_invitations (
            (ExampleCSHRoomManager *) channel_manager);
        }
    }
}

void
example_csh_connection_set_use_properties_room (ExampleCSHConnection *self,
                                                gboolean use_properties_room)
{
  TpChannelManagerIter iter;
  TpChannelManager *channel_manager;

  g_return_if_fail (EXAMPLE_IS_CSH_CONNECTION (self));

  tp_base_connection_channel_manager_iter_init (&iter, (TpBaseConnection *) self);

  while (tp_base_connection_channel_manager_iter_next (&iter, &channel_manager))
    {
      if (EXAMPLE_IS_CSH_ROOM_MANAGER (channel_manager))
        {
          example_csh_room_manager_set_use_properties_room (
            (ExampleCSHRoomManager *) channel_manager, use_properties_room);
        }
    }
}
