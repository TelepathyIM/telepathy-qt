/*
 * simple-client.c - a simple client
 *
 * Copyright © 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "simple-client.h"

#include <string.h>

#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/errors.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/util.h>

#include "util.h"

G_DEFINE_TYPE (TpTestsSimpleClient, tp_tests_simple_client, TP_TYPE_BASE_CLIENT)

static void
simple_observe_channels (
    TpBaseClient *client,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    TpChannelDispatchOperation *dispatch_operation,
    GList *requests,
    TpObserveChannelsContext *context)
{
  TpTestsSimpleClient *self = TP_TESTS_SIMPLE_CLIENT (client);
  GHashTable *info;
  gboolean fail;
  GList *l;

  /* Fail if caller set the fake "FAIL" info */
  g_object_get (context, "observer-info", &info, NULL);
  fail = tp_asv_get_boolean (info, "FAIL", NULL);
  g_hash_table_unref (info);

  if (self->observe_ctx != NULL)
    {
      g_object_unref (self->observe_ctx);
      self->observe_ctx = NULL;
    }

  if (fail)
    {
      GError error = { TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "No observation for you!" };

      tp_observe_channels_context_fail (context, &error);
      return;
    }

  g_assert (TP_IS_ACCOUNT (account));
  g_assert (tp_proxy_is_prepared (account, TP_ACCOUNT_FEATURE_CORE));

  g_assert (TP_IS_CONNECTION (connection));
  g_assert (tp_proxy_is_prepared (connection, TP_CONNECTION_FEATURE_CORE));

  g_assert_cmpuint (g_list_length (channels), >, 0);
  for (l = channels; l != NULL; l = g_list_next (l))
    {
      TpChannel *channel = l->data;

      g_assert (TP_IS_CHANNEL (channel));
      g_assert (tp_proxy_is_prepared (channel, TP_CHANNEL_FEATURE_CORE) ||
          tp_proxy_get_invalidated (channel) != NULL);
    }

  if (dispatch_operation != NULL)
    g_assert (TP_IS_CHANNEL_DISPATCH_OPERATION (dispatch_operation));

  for (l = requests; l != NULL; l = g_list_next (l))
    {
      TpChannelRequest *request = l->data;

      g_assert (TP_IS_CHANNEL_REQUEST (request));
    }

  self->observe_ctx = g_object_ref (context);
  tp_observe_channels_context_accept (context);
}

static void
simple_add_dispatch_operation (
    TpBaseClient *client,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    TpChannelDispatchOperation *dispatch_operation,
    TpAddDispatchOperationContext *context)
{
  TpTestsSimpleClient *self = TP_TESTS_SIMPLE_CLIENT (client);
  GList *l;

  g_assert (TP_IS_ACCOUNT (account));
  g_assert (tp_proxy_is_prepared (account, TP_ACCOUNT_FEATURE_CORE));

  g_assert (TP_IS_CONNECTION (connection));
  g_assert (tp_proxy_is_prepared (connection, TP_CONNECTION_FEATURE_CORE));

  g_assert (TP_IS_CHANNEL_DISPATCH_OPERATION (dispatch_operation));
  g_assert (tp_proxy_is_prepared (dispatch_operation,
        TP_CHANNEL_DISPATCH_OPERATION_FEATURE_CORE) ||
      tp_proxy_get_invalidated (dispatch_operation) != NULL);

  if (self->add_dispatch_ctx != NULL)
    {
      g_object_unref (self->add_dispatch_ctx);
      self->add_dispatch_ctx = NULL;
    }

  g_assert_cmpuint (g_list_length (channels), >, 0);
  for (l = channels; l != NULL; l = g_list_next (l))
    {
      TpChannel *channel = l->data;

      g_assert (TP_IS_CHANNEL (channel));
      g_assert (tp_proxy_is_prepared (channel, TP_CHANNEL_FEATURE_CORE) ||
          tp_proxy_get_invalidated (channel) != NULL);
    }

  self->add_dispatch_ctx = g_object_ref (context);
  tp_add_dispatch_operation_context_accept (context);
}

static void
simple_handle_channels (TpBaseClient *client,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    GList *requests_satisfied,
    gint64 user_action_time,
    TpHandleChannelsContext *context)
{
  TpTestsSimpleClient *self = TP_TESTS_SIMPLE_CLIENT (client);
  GList *l;

  if (self->handle_channels_ctx != NULL)
    {
      g_object_unref (self->handle_channels_ctx);
      self->handle_channels_ctx = NULL;
    }

  g_assert (TP_IS_ACCOUNT (account));
  g_assert (tp_proxy_is_prepared (account, TP_ACCOUNT_FEATURE_CORE));

  g_assert (TP_IS_CONNECTION (connection));
  g_assert (tp_proxy_is_prepared (connection, TP_CONNECTION_FEATURE_CORE));

  g_assert_cmpuint (g_list_length (channels), >, 0);
  for (l = channels; l != NULL; l = g_list_next (l))
    {
      TpChannel *channel = l->data;

      g_assert (TP_IS_CHANNEL (channel));
      g_assert (tp_proxy_is_prepared (channel, TP_CHANNEL_FEATURE_CORE) ||
          tp_proxy_get_invalidated (channel) != NULL);
    }

  for (l = requests_satisfied; l != NULL; l = g_list_next (l))
    {
      TpChannelRequest *request = l->data;

      g_assert (TP_IS_CHANNEL_REQUEST (request));
    }

  self->handle_channels_ctx = g_object_ref (context);
  tp_handle_channels_context_accept (context);
}

static void
tp_tests_simple_client_init (TpTestsSimpleClient *self)
{
}

static void
tp_tests_simple_client_dispose (GObject *object)
{
  TpTestsSimpleClient *self = TP_TESTS_SIMPLE_CLIENT (object);
  void (*dispose) (GObject *) =
    G_OBJECT_CLASS (tp_tests_simple_client_parent_class)->dispose;

  if (self->observe_ctx != NULL)
    {
      g_object_unref (self->observe_ctx);
      self->observe_ctx = NULL;
    }

  if (self->add_dispatch_ctx != NULL)
    {
      g_object_unref (self->add_dispatch_ctx);
      self->add_dispatch_ctx = NULL;
    }

  if (self->handle_channels_ctx != NULL)
    {
      g_object_unref (self->handle_channels_ctx);
      self->handle_channels_ctx = NULL;
    }

  if (dispose != NULL)
    dispose (object);
}

static void
tp_tests_simple_client_class_init (TpTestsSimpleClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  TpBaseClientClass *base_class = TP_BASE_CLIENT_CLASS (klass);

  object_class->dispose = tp_tests_simple_client_dispose;

  tp_base_client_implement_observe_channels (base_class,
      simple_observe_channels);

  tp_base_client_implement_add_dispatch_operation (base_class,
      simple_add_dispatch_operation);

  tp_base_client_implement_handle_channels (base_class,
      simple_handle_channels);
}

TpTestsSimpleClient *
tp_tests_simple_client_new (TpDBusDaemon *dbus_daemon,
    const gchar *name,
    gboolean uniquify_name)
{
  return tp_tests_object_new_static_class (TP_TESTS_TYPE_SIMPLE_CLIENT,
      "dbus-daemon", dbus_daemon,
      "name", name,
      "uniquify-name", uniquify_name,
      NULL);
}
