/*
 * simple-channel-dispatch-operation.c - a simple channel dispatch operation
 * service.
 *
 * Copyright © 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "simple-channel-dispatch-operation.h"

#include <telepathy-glib/channel.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/defs.h>
#include <telepathy-glib/enums.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/svc-channel-dispatch-operation.h>

static void channel_dispatch_operation_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (TpTestsSimpleChannelDispatchOperation,
    tp_tests_simple_channel_dispatch_operation,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_DISPATCH_OPERATION,
        channel_dispatch_operation_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
        tp_dbus_properties_mixin_iface_init)
    )

/* TP_IFACE_CHANNEL_DISPATCH_OPERATION is implied */
static const char *CHANNEL_DISPATCH_OPERATION_INTERFACES[] = { NULL };

static const gchar *CHANNEL_DISPATCH_OPERATION_POSSIBLE_HANDLERS[] = {
    TP_CLIENT_BUS_NAME_BASE ".Badger", NULL, };

enum
{
  PROP_0,
  PROP_INTERFACES,
  PROP_CONNECTION,
  PROP_ACCOUNT,
  PROP_CHANNELS,
  PROP_POSSIBLE_HANDLERS,
};

struct _SimpleChannelDispatchOperationPrivate
{
  gchar *conn_path;
  gchar *account_path;
  /* Array of TpChannel */
  GPtrArray *channels;
};

static void
tp_tests_simple_channel_dispatch_operation_handle_with (
    TpSvcChannelDispatchOperation *iface,
    const gchar *handler,
    DBusGMethodInvocation *context)
{
  if (!tp_strdiff (handler, "FAIL"))
    {
      GError error = { TP_ERROR, TP_ERROR_INVALID_ARGUMENT, "Nope" };

      dbus_g_method_return_error (context, &error);
      return;
    }

  dbus_g_method_return (context);
}

static void
tp_tests_simple_channel_dispatch_operation_claim (
    TpSvcChannelDispatchOperation *iface,
    DBusGMethodInvocation *context)
{
  dbus_g_method_return (context);
}

static void
tp_tests_simple_channel_dispatch_operation_handle_with_time (
    TpSvcChannelDispatchOperation *iface,
    const gchar *handler,
    gint64 user_action_timestamp,
    DBusGMethodInvocation *context)
{
  dbus_g_method_return (context);
}

static void
channel_dispatch_operation_iface_init (gpointer klass,
    gpointer unused G_GNUC_UNUSED)
{
#define IMPLEMENT(x) tp_svc_channel_dispatch_operation_implement_##x (\
  klass, tp_tests_simple_channel_dispatch_operation_##x)
  IMPLEMENT(handle_with);
  IMPLEMENT(claim);
  IMPLEMENT(handle_with_time);
#undef IMPLEMENT
}


static void
tp_tests_simple_channel_dispatch_operation_init (TpTestsSimpleChannelDispatchOperation *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TP_TESTS_TYPE_SIMPLE_CHANNEL_DISPATCH_OPERATION,
      TpTestsSimpleChannelDispatchOperationPrivate);

  self->priv->channels = g_ptr_array_new_with_free_func (
      (GDestroyNotify) g_object_unref);
}

static void
tp_tests_simple_channel_dispatch_operation_get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *spec)
{
  TpTestsSimpleChannelDispatchOperation *self =
    TP_TESTS_SIMPLE_CHANNEL_DISPATCH_OPERATION (object);

  switch (property_id) {
    case PROP_INTERFACES:
      g_value_set_boxed (value, CHANNEL_DISPATCH_OPERATION_INTERFACES);
      break;

    case PROP_ACCOUNT:
      g_value_set_boxed (value, self->priv->account_path);
      break;

    case PROP_CONNECTION:
      g_value_set_boxed (value, self->priv->conn_path);
      break;

    case PROP_CHANNELS:
      {
        GPtrArray *arr = g_ptr_array_new ();
        guint i;

        for (i = 0; i < self->priv->channels->len; i++)
          {
            TpChannel *channel = g_ptr_array_index (self->priv->channels, i);
            GValue props_value = G_VALUE_INIT;
            GVariant *props_variant;

            /* Yay, double conversion! But this is for tests corner case */
            props_variant = tp_channel_dup_immutable_properties (channel);
            dbus_g_value_parse_g_variant (props_variant, &props_value);

            g_ptr_array_add (arr,
                tp_value_array_build (2,
                  DBUS_TYPE_G_OBJECT_PATH, tp_proxy_get_object_path (channel),
                  TP_HASH_TYPE_STRING_VARIANT_MAP,
                      g_value_get_boxed (&props_value),
                  G_TYPE_INVALID));

            g_variant_unref (props_variant);
            g_value_unset (&props_value);
          }

        g_value_take_boxed (value, arr);
      }
      break;

    case PROP_POSSIBLE_HANDLERS:
      g_value_set_boxed (value, CHANNEL_DISPATCH_OPERATION_POSSIBLE_HANDLERS);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
      break;
  }
}

static void
tp_tests_simple_channel_dispatch_operation_finalize (GObject *object)
{
  TpTestsSimpleChannelDispatchOperation *self =
    TP_TESTS_SIMPLE_CHANNEL_DISPATCH_OPERATION (object);
  void (*finalize) (GObject *) =
    G_OBJECT_CLASS (tp_tests_simple_channel_dispatch_operation_parent_class)->finalize;

  g_free (self->priv->conn_path);
  g_free (self->priv->account_path);
  g_ptr_array_free (self->priv->channels, TRUE);

  if (finalize != NULL)
    finalize (object);
}

/**
  * This class currently only provides the minimum for
  * tp_channel_dispatch_operation_prepare to succeed. This turns out to be only a working
  * Properties.GetAll().
  */
static void
tp_tests_simple_channel_dispatch_operation_class_init (TpTestsSimpleChannelDispatchOperationClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  static TpDBusPropertiesMixinPropImpl a_props[] = {
        { "Interfaces", "interfaces", NULL },
        { "Connection", "connection", NULL },
        { "Account", "account", NULL },
        { "Channels", "channels", NULL },
        { "PossibleHandlers", "possible-handlers", NULL },
        { NULL }
  };

  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
        { TP_IFACE_CHANNEL_DISPATCH_OPERATION,
          tp_dbus_properties_mixin_getter_gobject_properties,
          NULL,
          a_props
        },
        { NULL },
  };

  g_type_class_add_private (klass, sizeof (TpTestsSimpleChannelDispatchOperationPrivate));
  object_class->get_property = tp_tests_simple_channel_dispatch_operation_get_property;
  object_class->finalize = tp_tests_simple_channel_dispatch_operation_finalize;

  param_spec = g_param_spec_boxed ("interfaces", "Extra D-Bus interfaces",
      "In this case we only implement ChannelDispatchOperation, so none.",
      G_TYPE_STRV,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INTERFACES, param_spec);

  param_spec = g_param_spec_boxed ("connection", "connection path",
      "Connection path",
      DBUS_TYPE_G_OBJECT_PATH,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_boxed ("account", "account path",
      "Account path",
      DBUS_TYPE_G_OBJECT_PATH,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ACCOUNT, param_spec);

  param_spec = g_param_spec_boxed ("channels", "channel paths",
      "Channel paths",
      TP_ARRAY_TYPE_CHANNEL_DETAILS_LIST,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CHANNELS, param_spec);

  param_spec = g_param_spec_boxed ("possible-handlers", "possible handlers",
      "possible handles",
      G_TYPE_STRV,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_POSSIBLE_HANDLERS,
      param_spec);

  klass->dbus_props_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (TpTestsSimpleChannelDispatchOperationClass, dbus_props_class));
}

void
tp_tests_simple_channel_dispatch_operation_set_conn_path (
    TpTestsSimpleChannelDispatchOperation *self,
    const gchar *conn_path)
{
  self->priv->conn_path = g_strdup (conn_path);
}

void
tp_tests_simple_channel_dispatch_operation_add_channel (
    TpTestsSimpleChannelDispatchOperation *self,
    TpChannel *chan)
{
  g_ptr_array_add (self->priv->channels, g_object_ref (chan));
}

void
tp_tests_simple_channel_dispatch_operation_lost_channel (
    TpTestsSimpleChannelDispatchOperation *self,
    TpChannel *chan)
{
  const gchar *path = tp_proxy_get_object_path (chan);

  g_ptr_array_remove (self->priv->channels, chan);

  tp_svc_channel_dispatch_operation_emit_channel_lost (self, path,
      TP_ERROR_STR_NOT_AVAILABLE, "Badger");

  if (self->priv->channels->len == 0)
    {
      /* We removed the last channel; fire Finished */
      tp_svc_channel_dispatch_operation_emit_finished (self);
    }
}

void
tp_tests_simple_channel_dispatch_operation_set_account_path (
    TpTestsSimpleChannelDispatchOperation *self,
    const gchar *account_path)
{
  self->priv->account_path = g_strdup (account_path);
}
